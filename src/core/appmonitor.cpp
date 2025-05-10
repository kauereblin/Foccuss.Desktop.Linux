#include "appmonitor.h"
#include "../data/database.h"
#include "../data/appmodel.h"

#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QProcess>

static QString s_logFilePath;

void logToFile_(const QString& message)
{
    if (s_logFilePath.isEmpty()) {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir appDataDir(appDataPath);
        if (!appDataDir.exists()) {
            appDataDir.mkpath(".");
        }
        s_logFilePath = appDataDir.filePath("foccuss_service.log");
    }

    QFile logFile(s_logFilePath);
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") 
            << " - " << message << "\n";
        logFile.close();
    }
}

AppMonitor::AppMonitor(Database* database, QObject *parent)
    : QObject(parent),
      m_database(database),
      m_isMonitoring(false)
{
    m_monitorTimer.setInterval(1000);
    connect(&m_monitorTimer, &QTimer::timeout, this, &AppMonitor::checkRunningApps);
}

void AppMonitor::startMonitoring()
{
    if (!m_isMonitoring && m_database && m_database->isInitialized()) {
        m_monitorTimer.start();
        m_isMonitoring = true;
        m_processPathCache.clear();
        
        QTimer::singleShot(0, this, &AppMonitor::checkRunningApps);
    } else {
        if (m_isMonitoring) {
            logToFile_("Monitoring already active");
        } else if (!m_database) {
            logToFile_("Cannot start monitoring - database is null");
        } else if (!m_database->isInitialized()) {
            logToFile_("Cannot start monitoring - database not initialized");
        }
    }
}

void AppMonitor::stopMonitoring()
{
    if (m_isMonitoring) {
        m_monitorTimer.stop();
        m_isMonitoring = false;
        m_processPathCache.clear();
    }
}

bool AppMonitor::isMonitoring() const
{
    return m_isMonitoring;
}

void AppMonitor::checkRunningApps()
{
    if (!m_database || !m_database->isInitialized())
        return;

    if (!m_database->isBlockingActive() || !m_database->isBlockingNow())
    {
        logToFile_("!m_database->isBlockingActive() || !m_database->isBlockingNow()");
        return;
    }

    // Get list of running processes using ps command
    QProcess process;
    process.start("ps", QStringList() << "-e" << "-o" << "pid,comm");
    if (!process.waitForFinished()) {
        logToFile_("Failed to get process list");
        return;
    }
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QSet<QString> currentProcessPaths;
    
    // Skip the header line
    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        
        if (parts.size() >= 2) {
            QString pid = parts[0];
            QString processName = parts[1];
            
            // Skip system processes
            if (processName.startsWith("kworker") || 
                processName.startsWith("systemd") || 
                processName == "foccuss") {
                continue;
            }
            
            // Try to get the full path of the process
            QString processPath;
            QProcess pathProcess;
            pathProcess.start("readlink", QStringList() << "-f" << "/proc/" + pid + "/exe");
            if (pathProcess.waitForFinished()) {
                processPath = pathProcess.readAllStandardOutput().trimmed();
                
                if (!processPath.isEmpty() && m_database->isAppBlocked(processPath)) {
                    currentProcessPaths.insert(processPath);
                    
                    if (!m_processPathCache.contains(processPath)) {
                        m_processPathCache.insert(processPath);
                        
                        // We've found a blocked app that's running
                        emit blockedAppLaunched(processPath, processName);
                        
                        // Try to kill the process
                        QProcess killProcess;
                        killProcess.start("kill", QStringList() << pid);
                        
                        QThread::msleep(100);
                    }
                }
            }
        }
    }
    
    // Remove processes that are no longer running from our cache
    QSet<QString> pathsToRemove = m_processPathCache;
    pathsToRemove.subtract(currentProcessPaths);
    
    if (!pathsToRemove.empty()) {
        for (const auto& path : pathsToRemove) {
            m_processPathCache.remove(path);
        }
    }
}