#include "applicationui.hpp"
#include "zhihu.hpp"
#include "daily/Misc/Misc.hpp"
#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/AbstractPane>
#include <QDebug>
using namespace bb::cascades;
ApplicationUI::ApplicationUI(QObject *parent) : QObject(parent) {
    Zhihu *client = new Zhihu(this);
    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);
    qml->setContextProperty("zhihu", client);
    Misc *dailyMisc = new Misc();
    dailyMisc->setParent(this);
    qml->setContextProperty("_misc", dailyMisc);
    AbstractPane *root = qml->createRootObject<AbstractPane>();
    if (!root) { qCritical() << "[startup] main.qml failed"; return; }
    Application::instance()->setScene(root);
    qDebug() << "[startup] Zhihu scene ready";
}
