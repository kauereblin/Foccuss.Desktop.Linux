#include "../include/Common.h"
#include "../include/ForwardDeclarations.h"

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QSharedMemory>
#include <QSqlDatabase>
#include <QSqlError>
#include <QPluginLoader>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>

#include "ui/mainwindow.h"
#include "data/database.h"
#include "service/linuxservice.h"

bool isRunningAsSudo() {
    // Check if we're running with sudo/root privileges
    uid_t uid = getuid();
    return (uid == 0);
}

bool requestRootPrivileges() {
    QString appPath = QCoreApplication::applicationFilePath();
    QProcess process;
    
    // Try with pkexec first (graphical sudo)
    process.start("pkexec", QStringList() << appPath << "--elevated");
    
    if (!process.waitForStarted()) {
        // Fall back to gksudo or regular sudo
        QString sudoCmd = "gksudo";
        QProcess checkProcess;
        checkProcess.start("which", QStringList() << "gksudo");
        if (!checkProcess.waitForFinished() || checkProcess.exitCode() != 0) {
            sudoCmd = "sudo";
        }
        
        process.start(sudoCmd, QStringList() << appPath << "--elevated");
        if (!process.waitForStarted()) {
            return false;
        }
    }
    
    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Foccuss");
    app.setOrganizationName("Foccuss");
    app.setOrganizationDomain("foccuss.app");

    // Check if running as service
    if (argc > 1 && QString(argv[1]) == "--service") {
        // Initialize database
        Database* database = new Database();
        if (!database->initialize()) {
            qDebug() << "Failed to initialize database";
            delete database;
            return 1;
        }

        LinuxService service(database);
        if (!service.initialize()) {
            qDebug() << "Failed to initialize service";
            delete database;
            return 1;
        }
        
        // For Linux service mode, we'll just start the app monitor
        // and run the application's event loop
        qDebug() << "Starting Foccuss service...";
        service.m_appMonitor->startMonitoring();
        
        return app.exec();
    }

    // Check if running elevated
    bool needsElevation = false;
    if (argc > 1 && QString(argv[1]) == "--elevated") {
        // Continue with elevated privileges
    } else if (!isRunningAsSudo() && needsElevation) {
        // Request elevation
        if (requestRootPrivileges()) {
            return 0; // Exit current instance
        } else {
            QMessageBox::critical(nullptr, "Foccuss", 
                "This application may require root privileges to manage the service.");
            // Continue anyway, since many Linux operations can be done without root
        }
    }

    // Ensure only one instance of the application is running
    QSharedMemory sharedMemory("FoccussAppInstance");
    if (!sharedMemory.create(1)) {
        QMessageBox::warning(nullptr, "Foccuss", "An instance of Foccuss is already running.");
        return 1;
    }
    
    // Load application stylesheet
    QFile styleFile(":/styles/main_style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }
    
    // Check for SQLite driver
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        QMessageBox::critical(nullptr, "Foccuss", 
            "SQLite driver not available. Please install qt6-sql-sqlite.");
        return 1;
    }
    
    // Initialize database
    Database* database = new Database();
    if (!database->initialize()) {
        QMessageBox::critical(nullptr, "Foccuss", "Failed to initialize database.");
        delete database;
        return 1;
    }

    // Create the service manager
    LinuxService service(database);
    
    // Create and show main window
    MainWindow mainWindow(database);
    mainWindow.show();
    
    return app.exec();
} 