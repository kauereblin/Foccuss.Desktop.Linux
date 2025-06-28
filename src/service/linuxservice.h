#pragma once
#ifndef LINUXSERVICE_H
#define LINUXSERVICE_H

#include "../../include/Common.h"

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
    bool enableService();
    bool disableService();
    void serviceWorkerThread();
    
    // Service name and status
    QString m_serviceName;
    QString m_serviceDisplayName;
    
    // Service components
    Database* m_database;
    AppMonitor* m_appMonitor;
    
    // Control event
    bool m_running;
};

#endif // LINUXSERVICE_H 