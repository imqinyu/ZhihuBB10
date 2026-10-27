#include "probe.hpp"
#include "zhihu.hpp"
#include "signature.hpp"
#include <bb/cascades/Application>
#include <bb/cascades/Page>
#include <bb/cascades/Label>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/AbstractPane>
#include <bb/data/JsonDataAccess>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QDebug>
#include <cstdio>
using namespace bb::cascades;
static void message(QtMsgType, const char *text) { std::fprintf(stderr, "%s\n", text); std::fflush(stderr); }
static QString dataPath(const QString &name) {
    return QDir::cleanPath(QCoreApplication::applicationDirPath()+"/../../data/"+name);
}
AuthProbe::AuthProbe(QObject *parent) : QObject(parent), m_client(new Zhihu(this)), m_pending(0),
    m_modelsStarted(false), m_modelPending(0) {
    connect(m_client, SIGNAL(accountChanged()), this, SLOT(accountChanged()));
}
void AuthProbe::start() {
    QFile input(dataPath("auth-input"));
    if (!input.open(QIODevice::ReadOnly)) {
        QTimer::singleShot(1000, this, SLOT(start())); return;
    }
    m_cookie = QString::fromUtf8(input.readAll()).trimmed();
    input.close();
    bool removed = input.remove();
    qDebug() << "[auth] private input consumed; removed=" << removed;
    qDebug() << "[signature selftest]" << zhihuSignature(QUrl("https://www.zhihu.com/api/v4/me"), "test-device");
    QStringList urls;
    urls << "https://www.zhihu.com/api/v4/me"
         << "https://api.zhihu.com/topstory/recommend"
         << "https://www.zhihu.com/api/v4/search_v3?t=general&q=BlackBerry&offset=0&limit=5"
         << "https://www.zhihu.com/api/v3/moments?limit=10&desktop=true"
         << "https://api.zhihu.com/notifications/v3/message/v3?limit=20"
         << "https://news-at.zhihu.com/api/4/news/latest";
    foreach (const QString &url, urls) {
        QNetworkReply *reply = m_client->get(url, "auth_probe", this, SLOT(finished()), m_cookie);
        if (reply) ++m_pending;
    }
}
void AuthProbe::finished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bb::data::JsonDataAccess json;
    QVariantMap root = json.loadFromBuffer(reply->readAll()).toMap();
    QVariantMap error = root.value("error").toMap();
    QVariantMap result;
    result["host"] = reply->url().host();
    result["path"] = reply->url().path();
    result["status"] = status;
    result["qtError"] = int(reply->error());
    result["errorCode"] = error.value("code", 0);
    result["needLogin"] = error.value("need_login", false);
    result["profileValid"] = !root.value("id").toString().isEmpty() && !root.value("name").toString().isEmpty();
    result["count"] = root.value("data", root.value("stories")).toList().size();
    result["jsonValid"] = !json.hasError();
    m_results.append(result);
    qDebug() << "[auth result]" << result;
    reply->deleteLater();
    if (--m_pending == 0) {
        json.save(m_results, dataPath("auth-results.json"));
        qDebug() << "[auth] raw requests COMPLETE; verifying production account flow";
        m_client->verifyAccount(m_cookie);
        m_cookie.clear();
    }
}
void AuthProbe::accountChanged() {
    if (m_modelsStarted || m_client->verifying()) return;
    if (!m_client->loggedIn()) {
        qDebug() << "[auth verification] failed=" << m_client->accountError(); return;
    }
    m_modelsStarted = true;
    qDebug() << "[auth verification] production account verified";
    // Wait until changedSession() has emitted sessionChanged and reset existing feeds.
    QTimer::singleShot(0, this, SLOT(startModels()));
}
void AuthProbe::startModels() {
    QStringList kinds; kinds << "home" << "follow" << "daily" << "search" << "notifications" << "question" << "detail";
    foreach (const QString &kind, kinds) {
        QString parameter;
        if (kind == "search") parameter = "BlackBerry";
        if (kind == "question") parameter = "31921579";
        Feed *feed = new Feed(m_client, kind, parameter, this);
        feed->setProperty("probeKind", kind);
        if (kind == "detail") {
            QVariantMap item; item["kind"] = "answer"; item["id"] = "2094699456440673419";
            item["title"] = ""; item["author"] = ""; feed->setItem(item);
        }
        connect(feed, SIGNAL(changed()), this, SLOT(modelChanged()));
        ++m_modelPending;
        feed->load(false);
    }
}
void AuthProbe::modelChanged() {
    Feed *feed = qobject_cast<Feed *>(sender());
    if (!feed || feed->busy() || !feed->loaded() || feed->property("probeDone").toBool()) return;
    feed->setProperty("probeDone", true);
    QVariantMap result;
    result["model"] = feed->property("probeKind"); result["count"] = feed->count();
    result["error"] = feed->error(); result["hasMore"] = feed->hasMore(); result["htmlLength"] = feed->html().size();
    m_results.append(result);
    qDebug() << "[model result]" << result;
    if (--m_modelPending == 0) {
        bb::data::JsonDataAccess json; json.save(m_results, dataPath("auth-results.json"));
        qDebug() << "[auth] COMPLETE; private test sandbox must be removed after collecting results";
    }
}
Q_DECL_EXPORT int main(int argc, char **argv) {
    qInstallMsgHandler(message);
    Application app(argc, argv);
    AuthProbe probe;
    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(&probe);
    qml->setContextProperty("zhihu", probe.client());
    AbstractPane *root = qml->createRootObject<AbstractPane>();
    if (!root) { qCritical() << "[auth UI] production QML failed"; return 1; }
    app.setScene(root);
    QTimer::singleShot(0, &probe, SLOT(start()));
    return Application::exec();
}
