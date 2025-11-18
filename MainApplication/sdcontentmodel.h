#ifndef SDCONTENTMODEL_H
#define SDCONTENTMODEL_H

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QObject>
#include <QQmlEngine>

class SdProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    SdProxyModel(QObject* parent) : QSortFilterProxyModel{parent} {};

private:

    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);

        QFileSystemModel* srcModel = qobject_cast<QFileSystemModel*>(sourceModel());
        QModelIndex rootIndex = srcModel->index(srcModel->rootPath());

        if(rootIndex == sourceParent)
        {
            return sourceModel()->data(index0).toString().contains(filterRegularExpression());
        }
        else
        {
            return true;
        }

    };
};

class SdContentModel : public QFileSystemModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SdProxyModel* sdProxyModel READ sdProxyModel CONSTANT)
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex NOTIFY rootIndexChanged FINAL)
public:
    explicit SdContentModel(QObject *parent = nullptr);

    SdProxyModel* sdProxyModel() {return &m_sdProxyModel;};
    QModelIndex rootIndex() const;

    void setWorkspace(const QDir& workspaceDir);

    Q_INVOKABLE void addFolder(QModelIndex parentIndex, QString name);

signals:
    void rootIndexChanged();

private:
    SdProxyModel m_sdProxyModel{this};
};

#endif // SDCONTENTMODEL_H
