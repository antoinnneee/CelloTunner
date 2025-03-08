#ifndef __QMLAPP_H
#define __QMLAPP_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QtQuick/QQuickView>
#include "tunerengine.h"

class QmlApp : public QQmlApplicationEngine
{
    Q_OBJECT

public:
    explicit QmlApp(QWindow *parent = nullptr);
    bool event(QEvent *event) override;
    ~QmlApp() override;


signals:

public slots:

private slots:

private:
    TunerEngine* m_tunerEngine;
};

#endif // __QMLAPP_H
