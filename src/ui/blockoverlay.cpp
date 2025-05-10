#include "blockoverlay.h"
#include "../../include/Common.h"

#include <QFileInfo>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QCloseEvent>
#include <QProcess>

BlockOverlay::BlockOverlay(const QString& appPath, const QString& appName, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      m_appPath(appPath),
      m_appName(appName),
      m_isClosing(false)
{
    setObjectName("blockOverlay");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);
    
    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName("blockMessageLabel");
    m_messageLabel->setText("This application has been blocked!");
    m_messageLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_messageLabel);
    
    m_appNameLabel = new QLabel(this);
    m_appNameLabel->setText(m_appName);
    m_appNameLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_appNameLabel);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    mainLayout->addLayout(buttonLayout);
    
    m_closeButton = new QPushButton("Close Overlay", this);
    connect(m_closeButton, &QPushButton::clicked, this, &BlockOverlay::onCloseClicked);
    
    m_killButton = new QPushButton("Force Close App", this);
    connect(m_killButton, &QPushButton::clicked, this, &BlockOverlay::onKillAppClicked);
    
    buttonLayout->addWidget(m_closeButton);
    buttonLayout->addWidget(m_killButton);
    
    setMinimumSize(400, 300);
    
    m_checkTimer.setInterval(500);
    connect(&m_checkTimer, &QTimer::timeout, this, &BlockOverlay::checkAppStatus);
}

BlockOverlay::~BlockOverlay()
{
    m_checkTimer.stop();
}

void BlockOverlay::showOverlay()
{
    // Show the overlay in the center of the screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    show();
    raise();
    activateWindow();
    
    m_checkTimer.start();
    
    // Try to kill the process by name
    QFileInfo fileInfo(m_appPath);
    QString processName = fileInfo.fileName();
    
    QProcess process;
    process.start("pkill", QStringList() << "-f" << processName);
    process.waitForFinished();
}

void BlockOverlay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    
    painter.setOpacity(0.5);
    painter.setBrush(QColor(200, 30, 30, 128));
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());
    
    QWidget::paintEvent(event);
}

void BlockOverlay::closeEvent(QCloseEvent *event)
{
    m_checkTimer.stop();
    m_isClosing = true;
    event->accept();
}

void BlockOverlay::onCloseClicked()
{
    close();
}

void BlockOverlay::onKillAppClicked()
{
    QFileInfo fileInfo(m_appPath);
    QString processName = fileInfo.fileName();
    
    // Try to kill the process
    QProcess process;
    process.start("pkill", QStringList() << "-9" << "-f" << processName);
    process.waitForFinished();
    
    close();
}

void BlockOverlay::checkAppStatus()
{
    if (m_isClosing)
        return;
    
    QFileInfo fileInfo(m_appPath);
    QString processName = fileInfo.fileName();
    
    // Check if the process is still running
    QProcess process;
    process.start("pgrep", QStringList() << "-f" << processName);
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    if (output.trimmed().isEmpty()) {
        // Process is no longer running
        close();
    } else {
        // Process is still running, kill it
        QProcess killProcess;
        killProcess.start("pkill", QStringList() << "-f" << processName);
        killProcess.waitForFinished();
        
        raise();
        activateWindow();
    }
} 