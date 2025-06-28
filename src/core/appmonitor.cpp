#include "appmonitor.h"
#include "../data/database.h"
#include "../data/appmodel.h"

static QString s_logFilePath;

void logToFileAM(const QString& message)
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
      m_isMonitoring(false),
      m_display(nullptr)
{
    m_monitorTimer.setInterval(1000);
    connect(&m_monitorTimer, &QTimer::timeout, this, &AppMonitor::checkRunningApps);
    initializeX11();
}

AppMonitor::~AppMonitor()
{
    cleanupX11();
}

void AppMonitor::initializeX11()
{
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        logToFileAM("Failed to open X11 display");
    }
}

void AppMonitor::cleanupX11()
{
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

void AppMonitor::startMonitoring()
{
    if (!m_isMonitoring && m_database && m_database->isInitialized()) {
        m_monitorTimer.start();
        m_isMonitoring = true;
        m_windowCache.clear();
        
        QTimer::singleShot(0, this, &AppMonitor::checkRunningApps);
    } else {
        if (m_isMonitoring) {
            logToFileAM("Monitoring already active");
        } else if (!m_database) {
            logToFileAM("Cannot start monitoring - database is null");
        } else if (!m_database->isInitialized()) {
            logToFileAM("Cannot start monitoring - database not initialized");
        }
    }
}

void AppMonitor::stopMonitoring()
{
    if (m_isMonitoring) {
        m_monitorTimer.stop();
        m_isMonitoring = false;
        m_windowCache.clear();
    }
}

bool AppMonitor::isMonitoring() const
{
    return m_isMonitoring;
}

QString AppMonitor::getProcessPath(pid_t pid)
{
    QString path = QString("/proc/%1/exe").arg(pid);
    char exePath[PATH_MAX];
    ssize_t len = readlink(path.toStdString().c_str(), exePath, sizeof(exePath) - 1);
    
    if (len != -1) {
        exePath[len] = '\0';
        QString processPath = QString(exePath);
        
        QFile cmdlineFile(QString("/proc/%1/cmdline").arg(pid));
        if (cmdlineFile.open(QIODevice::ReadOnly)) {
            QByteArray cmdline = cmdlineFile.readAll();
            cmdlineFile.close();
            
            QString cmdlineStr = QString::fromUtf8(cmdline);
            if (cmdlineStr.contains("flatpak")) {
                QRegularExpression rx("flatpak run ([^\\s]+)");
                QRegularExpressionMatch match = rx.match(cmdlineStr);
                if (match.hasMatch()) {
                    return QString("flatpak run %1").arg(match.captured(1));
                }
            }
        }
        
        if (processPath.startsWith("/snap/")) {
            QFileInfo snapInfo(processPath);
            QString snapName = snapInfo.fileName();
            return QString("/snap/bin/%1").arg(snapName);
        }
        
        return processPath;
    }
    
    return QString();
}

QString AppMonitor::getWindowName(Window window)
{
    if (!m_display) return QString();
    
    XTextProperty name;
    if (XGetWMName(m_display, window, &name) && name.value) {
        QString windowName = QString::fromUtf8(reinterpret_cast<char*>(name.value));
        XFree(name.value);
        return windowName;
    }
    
    return QString();
}

Window AppMonitor::findWindowByPid(pid_t pid)
{
    if (!m_display)
        return None;
    
    Window root = DefaultRootWindow(m_display);
    Window parent, root_return, *children;
    unsigned int nchildren;
    
    if (XQueryTree(m_display, root, &root_return, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            Window window = children[i];
            
            Atom actualType;
            int actualFormat;
            unsigned long nItems, bytesAfter;
            unsigned char* prop = nullptr;
            
            if (XGetWindowProperty(m_display, window, 
                                 XInternAtom(m_display, "_NET_WM_PID", False),
                                 0, 1, False, XA_CARDINAL, 
                                 &actualType, &actualFormat, 
                                 &nItems, &bytesAfter, &prop) == Success) {
                
                if (prop && nItems > 0 && actualFormat == 32) {
                    pid_t windowPid = *reinterpret_cast<pid_t*>(prop);
                    
                    if (windowPid == pid) {
                        XFree(prop);
                        XFree(children);
                        return window;
                    }
                }
                
                if (prop)
                    XFree(prop);
            }
        }
        XFree(children);
    }
    
    return None;
}

void AppMonitor::checkRunningApps()
{
    if (!m_database || !m_database->isInitialized())
        return;

    if (!m_database->isBlockingActive() || !m_database->isBlockingNow())
        return;

    QSet<Window> currentActiveWindows;
    
    DIR* procDir = opendir("/proc");
    if (!procDir) {
        logToFileAM("Failed to open /proc directory");
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        bool isNumeric = true;
        for (int i = 0; entry->d_name[i] != '\0'; i++) {
            if (!isdigit(entry->d_name[i])) {
                isNumeric = false;
                break;
            }
        }
        
        if (!isNumeric) continue;
        
        pid_t pid = atoi(entry->d_name);
        QString processPath = getProcessPath(pid);
        
        if (processPath.isEmpty() || processPath.contains("foccuss")) {
            continue;
        }
        
        bool isBlocked = false;
        QList<std::shared_ptr<AppModel>> blockedApps = m_database->getBlockedApps();
        for (const auto& app : blockedApps) {
            QString blockedPath = app->getPath();
            
            if (blockedPath.startsWith("flatpak run ")) {
                if (processPath.startsWith("flatpak run ") && 
                    processPath.contains(blockedPath.mid(12))) {
                    isBlocked = true;
                    break;
                }
            } else if (blockedPath.startsWith("/snap/bin/")) {
                if (processPath.startsWith("/snap/bin/") && 
                    processPath.endsWith(blockedPath.mid(10))) {
                    isBlocked = true;
                    break;
                }
            } else if (processPath == blockedPath) {
                isBlocked = true;
                break;
            }
        }

        if (isBlocked) {
            Window window = findWindowByPid(pid);
            if (window != None) {
                currentActiveWindows.insert(window);
                if (!m_windowCache.contains(window)) {
                    m_windowCache.insert(window);
                    QString processName = QFileInfo(processPath).fileName();

                    emit blockedAppLaunched(window, processPath, processName);
                    QThread::msleep(100);
                }
            }
        }
    }
    
    closedir(procDir);
    
    QSet<Window> windowsToRemove = m_windowCache;
    windowsToRemove.subtract(currentActiveWindows);
    
    if (!windowsToRemove.empty()) {
        for (const auto& window : windowsToRemove) {
            m_windowCache.remove(window);
        }
    }
}