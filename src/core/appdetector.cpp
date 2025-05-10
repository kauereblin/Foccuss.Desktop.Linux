#include "appdetector.h"
#include "../data/appmodel.h"

#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <QProcess>
#include <QSettings>
#include <QDirIterator>

AppDetector::AppDetector(QObject *parent) 
    : QObject(parent)
{
    refreshInstalledApps();
}

QList<std::shared_ptr<AppModel>> AppDetector::getInstalledApps() const
{
    return m_installedApps;
}

QList<std::shared_ptr<AppModel>> AppDetector::getRunningApps() const
{
    QList<std::shared_ptr<AppModel>> runningApps;
    
    // Get list of running processes using ps command
    QProcess process;
    process.start("ps", QStringList() << "-e" << "-o" << "pid,comm");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
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
            }
            
            if (!processPath.isEmpty()) {
                runningApps.append(std::make_shared<AppModel>(processPath, processName, false));
            }
        }
    }
    
    return runningApps;
}

void AppDetector::refreshInstalledApps()
{
    m_installedApps.clear();
    
    findDesktopApplications();
    findExecutablesInPath();
    
    std::sort(m_installedApps.begin(), m_installedApps.end(), 
              [](const std::shared_ptr<AppModel>& a, const std::shared_ptr<AppModel>& b) {
                  return a->getName().toLower() < b->getName().toLower();
              });
    
    emit installedAppsChanged();
}

void AppDetector::findDesktopApplications()
{
    // Standard locations for .desktop files
    QStringList desktopPaths = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        QDir::homePath() + "/.local/share/applications"
    };
    
    QSet<QString> processedApps;
    
    for (const QString& desktopPath : desktopPaths) {
        QDir dir(desktopPath);
        if (!dir.exists())
            continue;
            
        QStringList desktopFiles = dir.entryList(QStringList() << "*.desktop", QDir::Files);
        
        for (const QString& desktopFile : desktopFiles) {
            QString filePath = dir.filePath(desktopFile);
            QSettings desktop(filePath, QSettings::IniFormat);
            desktop.beginGroup("Desktop Entry");
            
            QString name = desktop.value("Name").toString();
            QString exec = desktop.value("Exec").toString();
            bool noDisplay = desktop.value("NoDisplay", false).toBool();
            QString type = desktop.value("Type").toString();
            
            desktop.endGroup();
            
            // Skip entries that shouldn't be displayed or aren't applications
            if (noDisplay || type != "Application" || name.isEmpty() || exec.isEmpty())
                continue;
                
            // Clean up the exec string to get the actual command
            exec = exec.split(" ").first();
            if (exec.startsWith("\"") && exec.endsWith("\""))
                exec = exec.mid(1, exec.length() - 2);
                
            // Handle full paths vs command names
            QString execPath = exec;
            if (!QFileInfo(execPath).exists()) {
                // Try to find the executable in PATH
                QProcess process;
                process.start("which", QStringList() << exec);
                if (process.waitForFinished() && process.exitCode() == 0) {
                    execPath = process.readAllStandardOutput().trimmed();
                }
            }
            
            if (!execPath.isEmpty() && QFileInfo(execPath).exists() && !processedApps.contains(execPath)) {
                m_installedApps.append(std::make_shared<AppModel>(execPath, name, false));
                processedApps.insert(execPath);
            }
        }
    }
}

void AppDetector::findExecutablesInPath()
{
    // Get directories from PATH environment variable
    QString pathEnv = qgetenv("PATH");
    QStringList pathDirs = pathEnv.split(":", Qt::SkipEmptyParts);
    
    QSet<QString> processedPaths;
    
    for (const QString& dir : pathDirs) {
        QDir pathDir(dir);
        if (!pathDir.exists())
            continue;
            
        QFileInfoList files = pathDir.entryInfoList(QDir::Files | QDir::Executable);
        
        for (const QFileInfo& fileInfo : files) {
            QString filePath = fileInfo.absoluteFilePath();
            
            // Skip if we've already processed this path
            if (processedPaths.contains(filePath))
                continue;
                
            // Skip system binaries and scripts
            if (fileInfo.fileName().startsWith(".") ||
                filePath.contains("/sbin/") ||
                filePath.contains("/bin/") ||
                fileInfo.fileName() == "foccuss")
                continue;
                
            m_installedApps.append(std::make_shared<AppModel>(filePath, fileInfo.fileName(), false));
            processedPaths.insert(filePath);
        }
    }
}

QString AppDetector::findExecutableInDirectory(const QString& directory, const QString& appName)
{
    QDir dir(directory);
    if (!dir.exists())
        return QString();
    
    QString possibleExeName = appName.split(" ").first();
    if (dir.exists(possibleExeName))
        return dir.filePath(possibleExeName);
    
    QStringList exeFiles = dir.entryList(QDir::Files | QDir::Executable);
    if (!exeFiles.isEmpty())
        return dir.filePath(exeFiles.first());
    
    QStringList commonSubdirs = {"bin", "usr/bin", "opt"};
    for (const QString& subdir : commonSubdirs) {
        QDir subdirectory(dir.filePath(subdir));
        if (subdirectory.exists()) {
            exeFiles = subdirectory.entryList(QDir::Files | QDir::Executable);
            if (!exeFiles.isEmpty())
                return subdirectory.filePath(exeFiles.first());
        }
    }
    
    return QString();
} 