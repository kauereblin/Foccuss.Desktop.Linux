#include "appmodel.h"

#include <QFileInfo>
#include <QFileIconProvider>
#include <QProcess>
#include <QDebug>

AppModel::AppModel(const QString& path, const QString& name, const bool active)
    : m_path(path), m_name(name), m_active(active)
{
    if (!m_path.isEmpty()) {
        loadIcon();
    }
}

QString AppModel::getPath() const
{
    return m_path;
}

QString AppModel::getName() const
{
    return m_name;
}

QIcon AppModel::getIcon() const
{
    return m_icon;
}

bool AppModel::getActive() const
{
    return m_active;
}

void AppModel::setPath(const QString& path)
{
    m_path = path;
    loadIcon();
}

void AppModel::setName(const QString& name)
{
    m_name = name;
}

bool AppModel::isValid() const
{
    return !m_path.isEmpty() && QFileInfo(m_path).exists();
}

void AppModel::setActive(const bool active)
{
    m_active = active;
}

bool AppModel::isRunning() const
{
    if (!isValid()) {
        return false;
    }
    
    // Extract executable name from path
    QFileInfo fileInfo(m_path);
    QString exeName = fileInfo.fileName().toLower();
    
    // Get list of running processes using ps command
    QProcess process;
    process.start("ps", QStringList() << "-e" << "-o" << "comm");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Skip the header line
    for (int i = 1; i < lines.size(); ++i) {
        QString processName = lines[i].trimmed().toLower();
        if (processName == exeName) {
            return true;
        }
    }
    
    return false;
}

void AppModel::loadIcon()
{
    QFileInfo fileInfo(m_path);
    if (fileInfo.exists()) {
        QFileIconProvider iconProvider;
        m_icon = iconProvider.icon(fileInfo);
    }
} 