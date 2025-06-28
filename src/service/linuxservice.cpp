#include "linuxservice.h"
#include "../core/appmonitor.h"
#include "../data/database.h"

static QString s_logFilePath;

void logToFileLS(const QString& message)
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

LinuxService::LinuxService(Database* database, QObject *parent)
    : QObject(parent),
      m_serviceName("foccuss"),
      m_serviceDisplayName("Foccuss Service"),
      m_database(database),
      m_appMonitor(nullptr),
      m_running(false)
{
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
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static const char* argv[] = {m_serviceName.toStdString().c_str()};
        new QCoreApplication(argc, const_cast<char**>(argv));
    }

    if (!m_database || !m_database->isInitialized()) {
        logToFileLS("Database not initialized");
        return false;
    }
    
    m_appMonitor = new AppMonitor(m_database, this);
    if (!m_appMonitor) {
        logToFileLS("Failed to create AppMonitor");
        return false;
    }
    
    return true;
}

bool LinuxService::installService()
{
    if (!createSystemdServiceFile()) {
        logToFileLS("Failed to create systemd service file");
        return false;
    }

    if (!enableService()) {
        logToFileLS("Failed to enable service");
        return false;
    }

    logToFileLS("Service installed successfully");
    return true;
}

bool LinuxService::uninstallService()
{
    if (!disableService()) {
        logToFileLS("Failed to disable service");
        return false;
    }

    if (!removeSystemdServiceFile()) {
        logToFileLS("Failed to remove systemd service file");
        return false;
    }

    logToFileLS("Service uninstalled successfully");
    return true;
}

bool LinuxService::startService()
{
    QProcess process;
    process.start("systemctl", {"--user", "start", m_serviceName});
    process.waitForFinished();
    
    if (process.exitCode() != 0) {
        logToFileLS("Failed to start service: " + process.readAllStandardError());
        return false;
    }
    
    logToFileLS("Service started successfully");
    return true;
}

bool LinuxService::stopService()
{
    QProcess process;
    process.start("systemctl", {"--user", "stop", m_serviceName});
    process.waitForFinished();
    
    if (process.exitCode() != 0) {
        logToFileLS("Failed to stop service: " + process.readAllStandardError());
        return false;
    }
    
    logToFileLS("Service stopped successfully");
    return true;
}

bool LinuxService::isServiceInstalled() const
{
    QProcess process;
    process.start("systemctl", {"--user", "is-enabled", m_serviceName});
    process.waitForFinished();
    return process.exitCode() == 0;
}

bool LinuxService::isServiceRunning() const
{
    QProcess process;
    process.start("systemctl", {"--user", "is-active", m_serviceName});
    process.waitForFinished();
    return process.exitCode() == 0;
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
    // Create user's systemd directory if it doesn't exist
    QString userSystemdDir = QDir::homePath() + "/.config/systemd/user";
    QDir dir(userSystemdDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString servicePath = userSystemdDir + "/" + m_serviceName + ".service";
    QString appPath = QCoreApplication::applicationFilePath();
    
    QFile serviceFile(servicePath);
    if (!serviceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        logToFileLS("Failed to open service file for writing");
        return false;
    }
    
    QTextStream out(&serviceFile);
    out << "[Unit]\n"
        << "Description=Foccuss Application Blocker Service\n"
        << "After=network.target\n\n"
        << "[Service]\n"
        << "Type=simple\n"
        << "ExecStart=" << appPath << " --service\n"
        << "Restart=always\n"
        << "RestartSec=5\n\n"
        << "[Install]\n"
        << "WantedBy=default.target\n";
    
    serviceFile.close();
    
    // Enable and start the service using systemctl --user
    QProcess process;
    process.start("systemctl", {"--user", "daemon-reload"});
    process.waitForFinished();
    logToFileLS("Reloaded systemd daemon");

    return process.exitCode() == 0;
}

bool LinuxService::removeSystemdServiceFile()
{
    QString servicePath = QDir::homePath() + "/.config/systemd/user/" + m_serviceName + ".service";
    QFile serviceFile(servicePath);
    
    if (serviceFile.exists()) {
        if (!serviceFile.remove()) {
            logToFileLS("Failed to remove service file");
            return false;
        }
        
        // Reload systemd after removing the service
        QProcess process;
        process.start("systemctl", {"--user", "daemon-reload"});
        process.waitForFinished();
        
        return process.exitCode() == 0;
    }
    
    return true;
}

bool LinuxService::enableService()
{
    QProcess process;
    process.start("systemctl", {"--user", "enable", m_serviceName});
    process.waitForFinished();
    return process.exitCode() == 0;
}

bool LinuxService::disableService()
{
    QProcess process;
    process.start("systemctl", {"--user", "disable", m_serviceName});
    process.waitForFinished();
    return process.exitCode() == 0;
}

void LinuxService::serviceWorkerThread()
{
    if (!m_appMonitor)
        return;

    // Start monitoring
    logToFileLS("Starting AppMonitor...");
    m_appMonitor->startMonitoring();
    logToFileLS("AppMonitor started: " + QString(m_appMonitor->isMonitoring() ? "true" : "false"));
    
    // Main service loop
    logToFileLS("Entering service main loop");
    m_running = true;
    
    while (m_running) {
        QThread::msleep(1000);
    }
    
    logToFileLS("Service worker thread exiting");
} 