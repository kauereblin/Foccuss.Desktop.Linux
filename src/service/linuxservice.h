#ifndef LINUXSERVICE_H
#define LINUXSERVICE_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <memory>

class AppMonitor;
class Database;

class LinuxService : public QObject
{
    Q_OBJECT
    
public:
    explicit LinuxService(Database* database, QObject *parent = nullptr);
    ~LinuxService();
    
    bool initialize();
    bool installService();
    bool uninstallService();
    bool startService();
    bool stopService();
    bool isServiceInstalled() const;
    bool isServiceRunning() const;
    
    QString getServiceName() const;
    QString getServiceDisplayName() const;
    
private:
    bool createSystemdServiceFile();
    bool removeSystemdServiceFile();
    bool runSystemctl(const QString& command, const QString& arg);
    bool enableAutostart();
    bool disableAutostart();
    
    // Service name and status
    QString m_serviceName;
    QString m_serviceDisplayName;
    QString m_serviceFilePath;
    
    // Service components
    Database* m_database;
    AppMonitor* m_appMonitor;
    QProcess m_systemctlProcess;
    
    // Check if we're the service instance
    bool m_isServiceInstance;
};

#endif // LINUXSERVICE_H 