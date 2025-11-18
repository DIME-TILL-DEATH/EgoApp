#include "sdcontentmodel.h"

SdContentModel::SdContentModel(QObject *parent)
    : QFileSystemModel{parent}
{
    setRootPath("C:/");
    // qDebug() << QDir::currentPath();

    QStringList filter;
    filter.append("*.wav");
    filter.append("*.mid");
    setNameFilters(filter);
    setNameFilterDisables(false);
    setReadOnly(false);

    m_sdProxyModel.setSourceModel(this);
    m_sdProxyModel.setFilterRegularExpression("SONGS");
}

QModelIndex SdContentModel::rootIndex() const
{
    return m_sdProxyModel.mapFromSource(index(rootPath()));
}

void SdContentModel::setWorkspace(const QDir &workspaceDir)
{
    setRootPath(workspaceDir.absolutePath());
    emit rootIndexChanged();
}

void SdContentModel::addFolder(QModelIndex parentIndex, QString name)
{
    QModelIndex realIndex = m_sdProxyModel.mapToSource(parentIndex);

    QDir rootDir(rootPath());

    if(!rootDir.exists("SONGS"))
    {
        realIndex = mkdir(index(rootPath()), "SONGS");
    }

    mkdir(realIndex, name);
}

