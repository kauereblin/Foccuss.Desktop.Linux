#pragma once
#ifndef APPMONITOR_H
#define APPMONITOR_H

#include "../../include/Common.h"

class AppModel;
class Database;

class AppMonitor : public QObject
{
    Q_OBJECT

public:
    explicit AppMonitor(Database* database, QObject *parent = nullptr);
    ~AppMonitor();
    
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
signals:
    void blockedAppLaunched(Window targetWindow, const QString& appPath, const QString& appName);
    
private slots:
    void checkRunningApps();
    
private:
    void initializeX11();
    void cleanupX11();
    Window findWindowByPid(pid_t pid);
    QString getWindowName(Window window);
    QString getProcessPath(pid_t pid);
    
    Database* m_database;
    QTimer m_monitorTimer;
    bool m_isMonitoring;
    
    // X11 display connection
    Display* m_display;
    
    // Cache previously detected processes to avoid repeatedly signaling
    QSet<Window> m_windowCache;
};

#endif // APPMONITOR_H 