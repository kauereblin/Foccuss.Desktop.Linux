#include "linuxservice.h"
#include "../core/appmonitor.h"
#include "../data/database.h"

#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QProcess>
#include <QFileInfo>
#include <QDateTime>

static QString s_logFilePath;

void logToFile(const QString& message) {
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

LinuxService::LinuxService(Database* database, QObject *parent)
    : QObject(parent),
      m_serviceName("foccuss"),
      m_serviceDisplayName("Foccuss Application Blocker"),
      m_database(database),
      m_appMonitor(nullptr),
      m_isServiceInstance(false)
{
    m_serviceFilePath = QString("/etc/systemd/user/%1.service").arg(m_serviceName);
}

LinuxService::~LinuxService()
{
    if (m_appMonitor) {
        m_appMonitor->stopMonitoring();
        delete m_appMonitor;
    }
}

bool LinuxService::initialize()
{
    if (!m_database || !m_database->isInitialized()) {
        logToFile("Database not initialized");
        return false;
    }
    
    m_appMonitor = new AppMonitor(m_database, this);
    if (!m_appMonitor) {
        logToFile("Failed to create AppMonitor");
        return false;
    }
    
    logToFile("AppMonitor created successfully");
    
    return true;
}

bool LinuxService::installService()
{
    if (createSystemdServiceFile()) {
        return runSystemctl("enable", "--user " + m_serviceName);
    }
    return false;
}

bool LinuxService::uninstallService()
{
    if (runSystemctl("disable", "--user " + m_serviceName)) {
        return removeSystemdServiceFile();
    }
    return false;
}

bool LinuxService::startService()
{
    return runSystemctl("start", "--user " + m_serviceName);
}

bool LinuxService::stopService()
{
    return runSystemctl("stop", "--user " + m_serviceName);
}

bool LinuxService::isServiceInstalled() const
{
    QFileInfo fileInfo(m_serviceFilePath);
    return fileInfo.exists();
}

bool LinuxService::isServiceRunning() const
{
    QProcess process;
    process.start("systemctl", QStringList() << "--user" << "is-active" << m_serviceName);
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    return output.trimmed() == "active";
}

QString LinuxService::getServiceName() const
{
    return m_serviceName;
}

QString LinuxService::getServiceDisplayName() const
{
    return m_serviceDisplayName;
}

bool LinuxService::createSystemdServiceFile()
{
    QString appPath = QCoreApplication::applicationFilePath();
    appPath = QDir::toNativeSeparators(appPath);
    
    // Create systemd user directory if it doesn't exist
    QDir systemdUserDir(QDir::homePath() + "/.config/systemd/user");
    if (!systemdUserDir.exists()) {
        systemdUserDir.mkpath(".");
    }
    
    // User systemd service file path
    QString userServicePath = systemdUserDir.filePath(m_serviceName + ".service");
    
    QFile serviceFile(userServicePath);
    if (!serviceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        logToFile("Failed to create service file: " + userServicePath);
        return false;
    }
    
    QTextStream out(&serviceFile);
    out << "[Unit]\n";
    out << "Description=" << m_serviceDisplayName << "\n";
    out << "After=graphical-session.target\n\n";
    
    out << "[Service]\n";
    out << "Type=simple\n";
    out << "ExecStart=" << appPath << " --service\n";
    out << "Restart=on-failure\n";
    out << "RestartSec=10\n";
    out << "Environment=DISPLAY=:0\n\n";
    
    out << "[Install]\n";
    out << "WantedBy=graphical-session.target\n";
    
    serviceFile.close();
    
    logToFile("Created service file: " + userServicePath);
    return true;
}

bool LinuxService::removeSystemdServiceFile()
{
    // User systemd service file path
    QString userServicePath = QDir::homePath() + "/.config/systemd/user/" + m_serviceName + ".service";
    
    QFile serviceFile(userServicePath);
    if (serviceFile.exists()) {
        if (!serviceFile.remove()) {
            logToFile("Failed to remove service file: " + userServicePath);
            return false;
        }
        
        logToFile("Removed service file: " + userServicePath);
        return true;
    }
    
    return true; // File doesn't exist, so nothing to remove
}

bool LinuxService::runSystemctl(const QString& command, const QString& arg)
{
    QProcess process;
    QString cmdStr = "systemctl " + command + " " + arg;
    logToFile("Running: " + cmdStr);
    
    QStringList args = arg.split(" ", Qt::SkipEmptyParts);
    args.prepend(command);
    
    process.start("systemctl", args);
    
    if (!process.waitForFinished(5000)) {
        logToFile("systemctl command timed out");
        return false;
    }
    
    int exitCode = process.exitCode();
    if (exitCode != 0) {
        QString error = process.readAllStandardError();
        logToFile("systemctl command failed with code " + QString::number(exitCode) + ": " + error);
        return false;
    }
    
    return true;
}

bool LinuxService::enableAutostart()
{
    // Create autostart directory if it doesn't exist
    QDir autostartDir(QDir::homePath() + "/.config/autostart");
    if (!autostartDir.exists()) {
        autostartDir.mkpath(".");
    }
    
    // Create desktop file
    QString desktopFilePath = autostartDir.filePath(m_serviceName + ".desktop");
    QFile desktopFile(desktopFilePath);
    
    if (!desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        logToFile("Failed to create autostart desktop file");
        return false;
    }
    
    QString appPath = QCoreApplication::applicationFilePath();
    
    QTextStream out(&desktopFile);
    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Name=" << m_serviceDisplayName << "\n";
    out << "Exec=" << appPath << " --service\n";
    out << "Terminal=false\n";
    out << "Hidden=false\n";
    out << "X-GNOME-Autostart-enabled=true\n";
    
    desktopFile.close();
    
    return true;
}

bool LinuxService::disableAutostart()
{
    QString desktopFilePath = QDir::homePath() + "/.config/autostart/" + m_serviceName + ".desktop";
    QFile desktopFile(desktopFilePath);
    
    if (desktopFile.exists()) {
        return desktopFile.remove();
    }
    
    return true; // File didn't exist, so nothing to do
} 