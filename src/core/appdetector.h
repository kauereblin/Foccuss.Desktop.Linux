#ifndef APPDETECTOR_H
#define APPDETECTOR_H

#include <QObject>
#include <QList>
#include <QSettings>
#include <QString>
#include <memory>

class AppModel;

class AppDetector : public QObject
{
    Q_OBJECT

public:
    explicit AppDetector(QObject *parent = nullptr);
    QList<std::shared_ptr<AppModel>> getInstalledApps() const;
    QList<std::shared_ptr<AppModel>> getRunningApps() const;
    void refreshInstalledApps();
    
signals:
    void installedAppsChanged();
    
private:
    void findDesktopApplications();
    void findExecutablesInPath();
    QString findExecutableInDirectory(const QString& directory, const QString& appName);
    
    QList<std::shared_ptr<AppModel>> m_installedApps;
};

#endif // APPDETECTOR_H 