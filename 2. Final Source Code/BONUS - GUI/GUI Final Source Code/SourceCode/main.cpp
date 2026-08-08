#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QTextStream>
#include "src/ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    app.setWindowIcon(QIcon(QStringLiteral(":/app/icon.png")));

    // Load stylesheet
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream ts(&styleFile);
        const QString stylesheet = ts.readAll();
        app.setStyleSheet(stylesheet);
        if (qEnvironmentVariableIsSet("NEXUS_DEBUG_STYLE")) {
            qDebug() << "APP STYLE loaded from" << styleFile.fileName()
                     << "chars =" << stylesheet.size();
        }
        styleFile.close();
    } else if (qEnvironmentVariableIsSet("NEXUS_DEBUG_STYLE")) {
        qWarning() << "APP STYLE failed to load" << styleFile.fileName();
    }

    MainWindow window;
    window.show();

    return app.exec();
}
