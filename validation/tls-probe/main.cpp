#include "probe.hpp"
#include <bb/cascades/Application>
#include <bb/cascades/Page>
#include <bb/cascades/Label>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslCipher>
#include <QSslCertificate>
#include <QDateTime>
#include <QDebug>
#include <cstdio>
#ifdef ZHIHU_TLS_CURL_PROBE
#include <QThread>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <curl/curl.h>
#include <openssl/ssl.h>
#endif
using namespace bb::cascades;
static void message(QtMsgType, const char *text) { std::fprintf(stderr, "%s\n", text); std::fflush(stderr); }
#ifdef ZHIHU_TLS_CURL_PROBE
static CURLcode tls12(CURL *, void *context, void *) {
    return SSL_CTX_set_ssl_version(static_cast<SSL_CTX *>(context), TLSv1_2_client_method()) == 1
        ? CURLE_OK : CURLE_SSL_CONNECT_ERROR;
}
static size_t collect(char *bytes, size_t size, size_t count, void *context) {
    QByteArray *body = static_cast<QByteArray *>(context);
    if (body->size() + size*count > 1024*1024) return 0;
    body->append(bytes, size*count); return size*count;
}
class CurlProbe : public QThread {
public:
    QString caPath;
    void run() {
        qDebug() << "[native curl]" << curl_version();
        QStringList urls;
        urls << "https://news-at.zhihu.com/api/4/news/latest" << "https://api.zhihu.com/topstory/recommend";
        foreach (const QString &url, urls) {
            CURL *curl = curl_easy_init(); QByteArray body;
            QByteArray target=url.toUtf8(), ca=caPath.toUtf8();
            char error[CURL_ERROR_SIZE] = {0};
            curl_easy_setopt(curl, CURLOPT_URL, target.constData());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
            curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tls12);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_CAINFO, ca.constData());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, collect);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
            CURLcode code = curl_easy_perform(curl);
            long status=0; curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);
            qDebug() << "[native curl result]" << url << "HTTP=" << status << "curl=" << int(code)
                     << error << "bytes=" << body.size() << "prefix=" << body.left(60);
            curl_easy_cleanup(curl);
        }
        qDebug() << "[native curl] COMPLETE";
    }
};
#endif

Probe::Probe(QObject *parent) : QObject(parent), m_network(this), m_pending(0) {}
void Probe::start() {
#ifdef ZHIHU_TLS_CURL_PROBE
    curl_global_init(CURL_GLOBAL_DEFAULT);
    QString caPath=QCoreApplication::applicationDirPath()+"/probe-ca.pem";
    qDebug() << "[native curl CA]" << caPath << "exists=" << QFile::exists(caPath);
    CurlProbe *curlProbe=new CurlProbe(); curlProbe->caPath=caPath;
    connect(curlProbe,SIGNAL(finished()),curlProbe,SLOT(deleteLater())); curlProbe->start();
#endif
    qDebug() << "[TLS] native Qt" << qVersion() << "SSL supported=" << QSslSocket::supportsSsl()
             << "cipher count=" << QSslSocket::supportedCiphers().size()
             << "default protocol=" << int(QSslConfiguration::defaultConfiguration().protocol());
    QStringList cipherNames;
    foreach (const QSslCipher &cipher, QSslSocket::supportedCiphers()) cipherNames.append(cipher.name());
    qDebug() << "[TLS ciphers]" << cipherNames;
    QStringList urls;
    urls << "https://news-at.zhihu.com/api/4/news/latest"
         << "https://news-at.zhihu.com/api/4/news/9793487"
         << "https://api.zhihu.com/topstory/recommend"
         << "https://www.zhihu.com/api/v4/search_v3?t=general&q=BlackBerry&offset=0&limit=5"
         << "https://www.zhihu.com/api/v4/answers/2094699456440673419"
         << "https://picx.zhimg.com/v2-77448de077d6e6a9a1cfd675e1bcd1bc.jpg?source=8673f162";
    foreach (const QString &url, urls) {
        QNetworkRequest request((QUrl(url)));
        request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/540.0 (KHTML, like Gecko) Ubuntu/10.10 Chrome/9.1.0.0 Safari/540.0");
        request.setRawHeader("Accept", "application/json, image/*");
        request.setRawHeader("Accept-Encoding", "identity");
        QSslConfiguration config = request.sslConfiguration();
        config.setProtocol(QSsl::SecureProtocols);
        config.setPeerVerifyMode(QSslSocket::VerifyPeer);
#ifdef ZHIHU_TLS_GCM_PROBE
        QList<QSslCipher> modern;
        foreach (const QSslCipher &cipher, QSslSocket::supportedCiphers())
            if (cipher.name().contains("ECDHE-") && cipher.name().contains("-GCM-")) modern.append(cipher);
        config.setCiphers(modern);
#endif
        request.setSslConfiguration(config);
        request.setRawHeader("Referer", "https://www.zhihu.com/");
        request.setRawHeader("x-api-version", "3.0.91");
        // No cookies, custom CA, TLS downgrade or certificate-error suppression.
        QNetworkReply *reply = m_network.get(request);
        reply->setProperty("started", QDateTime::currentMSecsSinceEpoch());
        connect(reply, SIGNAL(finished()), this, SLOT(finished()));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
        QTimer *timer = new QTimer(reply); timer->setSingleShot(true);
        connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
        connect(reply, SIGNAL(finished()), timer, SLOT(stop())); timer->start(20000);
        ++m_pending;
    }
}
void Probe::sslErrors(const QList<QSslError> &errors) {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    foreach (const QSslError &error, errors)
        qDebug() << "[TLS certificate]" << reply->url().host() << int(error.error()) << error.errorString();
}
void Probe::timeout() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender()->parent());
    if (reply && reply->isRunning()) { reply->setProperty("timeout", true); reply->abort(); }
}
void Probe::finished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QByteArray bytes = reply->readAll();
    QSslConfiguration ssl = reply->sslConfiguration();
    qDebug() << "[TLS result]" << reply->url().host() << reply->url().path()
        << "HTTP=" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
        << "error=" << int(reply->error()) << reply->errorString()
        << "timeout=" << reply->property("timeout").toBool()
        << "bytes=" << bytes.size() << "cipher=" << ssl.sessionCipher().name()
        << "protocol=" << ssl.sessionCipher().protocolString()
        << "ms=" << QDateTime::currentMSecsSinceEpoch() - reply->property("started").toLongLong();
    qDebug() << "[TLS peer]" << ssl.peerCertificate().subjectInfo(QSslCertificate::CommonName)
        << ssl.peerCertificate().issuerInfo(QSslCertificate::CommonName)
        << ssl.peerCertificate().expiryDate().toString(Qt::ISODate);
    if (!reply->url().host().endsWith(".zhimg.com"))
        qDebug() << "[TLS body prefix]" << bytes.left(80);
    reply->deleteLater();
    if (--m_pending == 0) qDebug() << "[TLS] COMPLETE";
}
Q_DECL_EXPORT int main(int argc, char **argv) {
    qInstallMsgHandler(message);
    Application app(argc, argv);
    Page *page = new Page();
    page->setContent(Label::create().text("Native HTTPS probe. Results: logs/log"));
    app.setScene(page);
    Probe probe;
    QTimer::singleShot(0, &probe, SLOT(start()));
    return Application::exec();
}
