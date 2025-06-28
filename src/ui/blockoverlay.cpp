#include "blockoverlay.h"

static QString s_logFilePath;

void logToFileBO(const QString& message)
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

BlockOverlay::BlockOverlay(const X11Window targetWindow, const QString& appPath, const QString& appName, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
      m_targetWindow(targetWindow),
      m_appPath(appPath),
      m_appName(appName),
      m_isClosing(false),
      m_positionRetryCount(0)
{
    setObjectName("blockOverlay");
    
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setAttribute(Qt::WA_AlwaysStackOnTop, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    
    setWindowModality(Qt::ApplicationModal);
    
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
    
    m_checkTimer.setInterval(100);
    connect(&m_checkTimer, &QTimer::timeout, this, &BlockOverlay::onCheckWindowPosition);
}

BlockOverlay::~BlockOverlay()
{
    m_checkTimer.stop();
}

void BlockOverlay::showOverlay()
{
    positionOverlayFullScreen();
    
    show();
    raise();
    activateWindow();
    
    m_checkTimer.start();
}

void BlockOverlay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    
    painter.setOpacity(0.8);
    painter.setBrush(QColor(0, 0, 0, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());
    
    painter.setOpacity(1.0);
    painter.setPen(QPen(QColor(255, 0, 0), 5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect());
    
    QWidget::paintEvent(event);
}

void BlockOverlay::closeEvent(QCloseEvent *event)
{
    m_checkTimer.stop();
    m_isClosing = true;
    event->accept();
}

void BlockOverlay::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    raise();
    activateWindow();
}

bool BlockOverlay::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        raise();
        activateWindow();
    } else if (event->type() == QEvent::WindowDeactivate) {
        QTimer::singleShot(0, this, [this]() {
            raise();
            activateWindow();
        });
    }
    
    return QWidget::event(event);
}

void BlockOverlay::onCloseClicked()
{
    close();
}

void BlockOverlay::onKillAppClicked()
{
    QFileInfo fileInfo(m_appPath);
    QString processName = fileInfo.fileName();
    
    QProcess process;
    process.start("pkill", QStringList() << "-9" << "-f" << processName);
    process.waitForFinished();
    
    close();
}

void BlockOverlay::onCheckWindowPosition()
{
    if (m_targetWindow == None) {
        return;
    }

    updatePosition();
}

void BlockOverlay::updatePosition()
{
    if (m_targetWindow == None) {
        return;
    }

    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        logToFileBO("Failed to open X display");
        return;
    }

    XWindowAttributes attrs;
    if (XGetWindowAttributes(display, m_targetWindow, &attrs)) {
        if (attrs.map_state == IsViewable) {
            if (attrs.width <= 0 || attrs.height <= 0) {
                logToFileBO("Window has invalid dimensions, using fallback positioning");
                positionOverlayFallback(display, attrs);
                return;
            }

            positionOverlayFullScreen();
            
            if (m_checkTimer.interval() == 100) {
                m_checkTimer.setInterval(500);
            }
            
            m_positionRetryCount = 0;
        }
        else if (attrs.map_state == IsUnmapped) {
            m_positionRetryCount++;
            
            if (m_positionRetryCount > 50) {
                positionOverlayFallback(display, attrs);
                m_checkTimer.setInterval(500);
            }
        }
        else if (attrs.map_state == IsUnviewable) {
            positionOverlayFallback(display, attrs);
        }
        else {
            logToFileBO(QString("Unknown window state: %1").arg(attrs.map_state));
            positionOverlayFallback(display, attrs);
        }
    } else {
        logToFileBO("Failed to get window attributes for target window");
        
        Window root_return, parent_return;
        Window* children_return;
        unsigned int nchildren_return;
        
        if (XQueryTree(display, m_targetWindow, &root_return, &parent_return, &children_return, &nchildren_return) == 0) {
            logToFileBO("Target window no longer exists, closing overlay");
            close();
            return;
        }
        
        if (children_return) {
            XFree(children_return);
        }
        
        positionOverlayFallback(display);
    }

    XCloseDisplay(display);

    if (m_isClosing)
        close();
}

void BlockOverlay::positionOverlayFallback(Display *display, const XWindowAttributes &attrs)
{
    positionOverlayFullScreen();
}

void BlockOverlay::positionOverlayFallback(Display *display)
{
    positionOverlayFullScreen();
}

void BlockOverlay::positionOverlayFullScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect virtualGeometry = screen->virtualGeometry();
        if (virtualGeometry.isValid()) {
            move(virtualGeometry.x(), virtualGeometry.y());
            resize(virtualGeometry.width(), virtualGeometry.height());
        } else {
            QRect screenGeometry = screen->geometry();
            move(screenGeometry.x(), screenGeometry.y());
            resize(screenGeometry.width(), screenGeometry.height());
        }
    } else {
        move(0, 0);
        resize(1920, 1080);
        logToFileBO("Emergency full screen overlay at (0, 0) size: 1920x1080");
    }
    
    raise();
    activateWindow();
}
