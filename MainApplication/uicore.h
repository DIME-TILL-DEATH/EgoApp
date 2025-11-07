#ifndef UICORE_H
#define UICORE_H

#include <QObject>

class UiCore : public QObject
{
    Q_OBJECT
public:
    explicit UiCore(QObject *parent = nullptr);

signals:
};

#endif // UICORE_H
