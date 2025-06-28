#include "appmodel.h"

AppModel::AppModel(const QString& path, const QString& name, const bool active)
    : m_path(path)
    , m_name(name)
    , m_active(active)
{
    loadIcon();
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
    return !m_path.isEmpty();
}

void AppModel::setActive(const bool active)
{
    m_active = active;
}

void AppModel::loadIcon()
{
    QFileInfo fileInfo(m_path);
    if (fileInfo.exists()) {
        QFileIconProvider iconProvider;
        m_icon = iconProvider.icon(fileInfo);
    }
} 