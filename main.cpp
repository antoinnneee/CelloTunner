#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "qmlapp.h"
#include "tools/crashReportTool.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    installCrashHandler();
    qInstallMessageHandler(0);
    
    QCoreApplication::setOrganizationDomain("cb4tech.com");
    QCoreApplication::setOrganizationName("CB4Tech");
    QCoreApplication::setApplicationName("CelloTuner");

    QmlApp a;

    return app.exec();
}
