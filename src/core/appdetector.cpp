#include "appdetector.h"
#include "../data/appmodel.h"

AppDetector::AppDetector(QObject *parent) 
    : QObject(parent)
{
    refreshInstalledApps();
}

QList<std::shared_ptr<AppModel>> AppDetector::getInstalledApps() const
{
    return m_installedApps;
}

void AppDetector::refreshInstalledApps()
{
    m_installedApps.clear();
    
    findAppsInDesktopFiles();
    findSnapApps();
    findFlatpakApps();
    
    std::sort(m_installedApps.begin(), m_installedApps.end(), 
              [](const std::shared_ptr<AppModel>& a, const std::shared_ptr<AppModel>& b) {
                  return a->getName().toLower() < b->getName().toLower();
              });
    
    emit installedAppsChanged();
}

void AppDetector::findAppsInDesktopFiles()
{
    // Common locations for desktop files
    QStringList desktopPaths = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        QDir::homePath() + "/.local/share/applications"
    };
    
    for (const QString& path : desktopPaths) {
        QDir dir(path);
        if (dir.exists()) {
            QStringList filters;
            filters << "*.desktop";
            QStringList files = dir.entryList(filters, QDir::Files);
            
            for (const QString& file : files) {
                processDesktopFile(dir.filePath(file));
            }
        }
    }
}

void AppDetector::findSnapApps()
{
    QProcess process;
    process.start("snap", {"list"});
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        // Skip header line
        for (int i = 1; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;
            
            QStringList parts = line.split(QRegularExpression("\\s+"));
            if (parts.size() >= 2) {
                QString appName = parts[0];
                QString appPath = QString("/snap/bin/%1").arg(appName);
                
                if (QFile::exists(appPath)) {
                    m_installedApps.append(std::make_shared<AppModel>(appPath, appName, false));
                }
            }
        }
    }
}

void AppDetector::findFlatpakApps()
{
    QProcess process;
    process.start("flatpak", {"list", "--app", "--columns=application,name"});
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        for (const QString& line : lines) {
            QStringList parts = line.split('\t');
            if (parts.size() >= 2) {
                QString appId = parts[0];
                QString appName = parts[1];
                QString appPath = QString("flatpak run %1").arg(appId);
                
                m_installedApps.append(std::make_shared<AppModel>(appPath, appName, false));
            }
        }
    }
}

void AppDetector::processDesktopFile(const QString& filePath)
{
    QSettings desktopFile(filePath, QSettings::IniFormat);
    
    // Check if it's a valid desktop entry
    if (!desktopFile.contains("Desktop Entry/Type") || 
        desktopFile.value("Desktop Entry/Type").toString() != "Application") {
        return;
    }
    
    // Skip hidden entries and system utilities
    if (desktopFile.value("Desktop Entry/Hidden", false).toBool() ||
        desktopFile.value("Desktop Entry/NoDisplay", false).toBool() ||
        desktopFile.value("Desktop Entry/Categories").toString().contains("System")) {
        return;
    }
    
    QString execPath = getExecutablePath(filePath);
    if (execPath.isEmpty()) {
        return;
    }
    
    // Skip system utilities and development tools
    QString categories = desktopFile.value("Desktop Entry/Categories").toString().toLower();
    if (categories.contains("system") || 
        categories.contains("utility") || 
        categories.contains("development") ||
        categories.contains("settings")) {
        return;
    }
    
    QString appName = getAppName(filePath);
    if (appName.isEmpty()) {
        appName = QFileInfo(execPath).fileName();
    }
    
    m_installedApps.append(std::make_shared<AppModel>(execPath, appName, false));
}

QString AppDetector::getExecutablePath(const QString& desktopFile)
{
    QSettings settings(desktopFile, QSettings::IniFormat);
    
    QString exec = settings.value("Desktop Entry/Exec").toString();
    if (exec.isEmpty()) {
        return QString();
    }
    
    // Remove arguments and % parameters
    exec = exec.split(" ").first();
    exec = exec.split("%").first();
    
    // If it's a full path, return it
    if (exec.startsWith("/")) {
        return exec;
    }
    
    // Try to find the executable in PATH
    QProcess process;
    process.start("which", {exec});
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        return process.readAllStandardOutput().trimmed();
    }
    
    return QString();
}

QString AppDetector::getAppName(const QString& desktopFile)
{
    QSettings settings(desktopFile, QSettings::IniFormat);
    
    // Try to get the localized name first
    QString lang = QLocale::system().name();
    QString name = settings.value(QString("Desktop Entry/Name[%1]").arg(lang)).toString();
    
    if (name.isEmpty()) {
        name = settings.value("Desktop Entry/Name").toString();
    }
    
    return name;
}

QString AppDetector::getAppIcon(const QString& desktopFile)
{
    QSettings settings(desktopFile, QSettings::IniFormat);
    
    QString icon = settings.value("Desktop Entry/Icon").toString();
    if (icon.isEmpty()) {
        return QString();
    }
    
    // If it's a full path, return it
    if (icon.startsWith("/")) {
        return icon;
    }
    
    // Try to find the icon in standard locations
    QStringList iconPaths = {
        "/usr/share/icons",
        "/usr/share/pixmaps",
        QDir::homePath() + "/.local/share/icons"
    };
    
    for (const QString& path : iconPaths) {
        QDir dir(path);
        if (dir.exists()) {
            QStringList filters;
            filters << icon + ".*";
            QStringList files = dir.entryList(filters, QDir::Files);
            
            if (!files.isEmpty()) {
                return dir.filePath(files.first());
            }
        }
    }
    
    return QString();
} 