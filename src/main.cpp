#include "../include/Common.h"
#include "../include/ForwardDeclarations.h"

#include "ui/mainwindow.h"
#include "data/database.h"
#include "service/linuxservice.h"

static QString s_logFilePath;

void logToFileM(const QString& message)
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

bool isRunningAsAdmin() {
    return geteuid() == 0;
}

bool requestAdminPrivileges() {
    QProcess process;
    process.start("pkexec", {QCoreApplication::applicationFilePath()});
    return process.waitForStarted();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Foccuss");
    app.setOrganizationName("Foccuss");
    app.setOrganizationDomain("foccuss.app");

    if (argc > 1 && QString(argv[1]) == "--service") {
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

        return app.exec();
    }

    if (argc > 1 && QString(argv[1]) == "--elevated") {
    } else if (!isRunningAsAdmin()) {
        if (requestAdminPrivileges()) {
            QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList() << "--elevated");
            return 0;
        } else {
            QMessageBox::critical(nullptr, "Foccuss", 
                "This application requires administrator privileges to install and manage the service.");
            return 1;
        }
    }

    // Ensure only one instance of the application is running
    QSharedMemory sharedMemory("FoccussAppInstance");
    if (!sharedMemory.create(1)) {
        sharedMemory.attach();
    }
    
    QFile styleFile(":/styles/main_style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }
    
    QPluginLoader loader;
    loader.setFileName("./sqldrivers/libqsqlite.so");
    if (!loader.load()) {
        QMessageBox::critical(nullptr, "Foccuss", 
            QString("Failed to load SQLite driver: %1").arg(loader.errorString()));
        return 1;
    }
    
    Database* database = new Database();
    if (!database->initialize()) {
        QMessageBox::critical(nullptr, "Foccuss", "Failed to initialize database.");
        delete database;
        return 1;
    }

    LinuxService service(database);
    if (!service.isServiceInstalled()) {
        if (!service.installService()) {
            QMessageBox::warning(nullptr, "Foccuss", 
                "Failed to install the service. The application will run without service support.");
        } else {
            if (!service.startService()) {
                QMessageBox::warning(nullptr, "Foccuss", 
                    "Failed to start the service. The application will run without service support.");
            }
        }
    }
    
    MainWindow mainWindow(database);
    mainWindow.show();
    
    return app.exec();
} 