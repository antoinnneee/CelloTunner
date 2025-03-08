#include <QDebug>

#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtQml/QQmlContext>

#include "qmlapp.h"
#include "tunerengine.h"

#include <QDir>
#include <QStandardPaths>
#ifdef Q_OS_ANDROID
#include <QJniObject.h>
#endif

QmlApp::QmlApp(QWindow *parent)
    : QQmlApplicationEngine(parent)
    , m_tunerEngine(new TunerEngine(this))
{
    QQuickStyle::setStyle("Material");
    
    // Expose the tuner engine to QML before loading the QML file
    rootContext()->setContextProperty("tuner", m_tunerEngine);
    
    // Start the tuner
    m_tunerEngine->start();
    
    qDebug() << "set UI link";
    load(QUrl("qrc:/qml/main.qml"));
}

bool QmlApp::event(QEvent *event)
{
    if (event->type() == QEvent::Close) {
        m_tunerEngine->stop();
    }
    return QQmlApplicationEngine::event(event);
}

QmlApp::~QmlApp()
{
    m_tunerEngine->stop();
}
