#include "gamecontroller.h"

#include <sailfishapp.h>

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QScopedPointer>
#include <QtQml>

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName(QStringLiteral("rs.r8"));
    app->setApplicationName(QStringLiteral("harbour-cribbage"));
    app->setApplicationVersion(QStringLiteral(APP_VERSION));

    qmlRegisterUncreatableType<GameController>("harbour.cribbage", 1, 0, "Cribbage",
                                               QStringLiteral("Use the game context property"));

    Session online(QStringLiteral("cribbage"), 2);
    GameController game(&online);
    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("online"), &online);
    view->rootContext()->setContextProperty(QStringLiteral("game"), &game);
    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    return app->exec();
}
