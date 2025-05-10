#ifndef BLOCKOVERLAY_H
#define BLOCKOVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

class BlockOverlay : public QWidget
{
    Q_OBJECT
    
public:
    explicit BlockOverlay(const QString& appPath, const QString& appName, QWidget *parent = nullptr);
    ~BlockOverlay();
    
    void showOverlay();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    void onCloseClicked();
    void onKillAppClicked();
    void checkAppStatus();
    
private:
    QString m_appPath;
    QString m_appName;
    QLabel *m_messageLabel;
    QLabel *m_appNameLabel;
    QPushButton *m_closeButton;
    QPushButton *m_killButton;
    
    QTimer m_checkTimer;
    bool m_isClosing;
};

#endif // BLOCKOVERLAY_H 