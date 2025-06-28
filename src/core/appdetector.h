#pragma once
#ifndef APPDETECTOR_H
#define APPDETECTOR_H

#include "../../include/Common.h"

class AppModel;

class AppDetector : public QObject
{
    Q_OBJECT
    
public:
    explicit AppDetector(QObject *parent = nullptr);
    
    QList<std::shared_ptr<AppModel>> getInstalledApps() const;
    
public slots:
    void refreshInstalledApps();
    
signals:
    void installedAppsChanged();
    
private:
    void findAppsInDesktopFiles();
    void findSnapApps();
    void findFlatpakApps();
    void processDesktopFile(const QString& filePath);
    QString getExecutablePath(const QString& desktopFile);
    QString getAppName(const QString& desktopFile);
    QString getAppIcon(const QString& desktopFile);
    
    QList<std::shared_ptr<AppModel>> m_installedApps;
};

#endif // APPDETECTOR_H 