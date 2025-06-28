#ifndef BLOCKOVERLAY_H
#define BLOCKOVERLAY_H

#include "../../include/Common.h"

class BlockOverlay : public QWidget
{
    Q_OBJECT
    
public:
    explicit BlockOverlay(const X11Window targetWindow, const QString& appPath, const QString& appName, QWidget *parent = nullptr);
    ~BlockOverlay();
    
    void showOverlay();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    bool event(QEvent *event) override;
    
private slots:
    void onCloseClicked();
    void onKillAppClicked();
    void onCheckWindowPosition();
    
private:
    void updatePosition();
    void positionOverlayFallback(Display *display, const XWindowAttributes &attrs);
    void positionOverlayFallback(Display *display);
    void positionOverlayFullScreen();

private:
    QString m_appPath;
    QString m_appName;
    QLabel *m_messageLabel;
    QLabel *m_appNameLabel;
    QPushButton *m_closeButton;
    QPushButton *m_killButton;
    
    X11Window m_targetWindow;
    QTimer m_checkTimer;
    bool m_isClosing;
    int m_positionRetryCount;
};

#endif // BLOCKOVERLAY_H 