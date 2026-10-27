// Protocol port of Zhihu++ ZhihuPhoneLoginClient.kt, zly2006, AGPL-3.0-only.
#include "phone_login.hpp"
#include <bb/data/JsonDataAccess>
#include <bb/cascades/Image>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QCryptographicHash>
#include <QDateTime>
#include <QTimer>
#include <QUrl>
static const char *clientId = "8d5227e0aaaa4797a763ac64e0c3b8";
static const char *clientSecret = "ecbefbf6b17e47ecb9035107866380";
static const char *source = "com.zhihu.android";
static QByteArray field(const char *name, const QString &value) {
    QByteArray encoded = QUrl::toPercentEncoding(value); encoded.replace("%20", "+");
    return QByteArray(name) + '=' + encoded;
}
class PhoneCookieJar : public QNetworkCookieJar {
public:
    PhoneCookieJar(QObject *parent) : QNetworkCookieJar(parent) {}
    bool setCookiesFromUrl(const QList<QNetworkCookie> &, const QUrl &) { return false; }
};
PhoneLogin::PhoneLogin(QObject *parent) : QObject(parent), m_network(this), m_busy(false), m_sent(false),
    m_captchaRequired(false), m_rechecked(false), m_cooldown(0), m_authorization(QString("oauth ") + clientId) {
    m_network.setCookieJar(new PhoneCookieJar(&m_network));
    QTimer *timer = new QTimer(this); connect(timer, SIGNAL(timeout()), this, SLOT(tick())); timer->start(1000);
}
void PhoneLogin::tick() { if (m_cooldown > 0) { --m_cooldown; emit changed(); } }
QString PhoneLogin::normalizePhone(QString phone) {
    phone = phone.trimmed().remove(' ').remove('-');
    if (phone.startsWith("+86")) phone.remove(0, 3);
    else if (phone.size() == 13 && phone.startsWith("86")) phone.remove(0, 2);
    return QRegExp("1[0-9]{10}").exactMatch(phone) ? "+86" + phone : QString();
}
QByteArray PhoneLogin::hmacSha1(QByteArray key, const QByteArray &message) {
    if (key.size() > 64) key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
    key = key.leftJustified(64, '\0'); QByteArray inner(64, '\x36'), outer(64, '\x5c');
    for (int i=0; i<64; ++i) { inner[i] = inner.at(i) ^ key.at(i); outer[i] = outer.at(i) ^ key.at(i); }
    return QCryptographicHash::hash(outer + QCryptographicHash::hash(inner + message, QCryptographicHash::Sha1), QCryptographicHash::Sha1).toHex();
}
void PhoneLogin::fail(const QString &message) { m_busy = false; m_error = message; emit changed(); }
void PhoneLogin::requestDigits(const QString &phone) {
    if (m_busy || m_cooldown > 0) return;
    const QString normalized = normalizePhone(phone);
    if (normalized.isEmpty()) { fail("invalid_phone"); return; }
    m_phone = normalized; m_sent = false; m_rechecked = false; m_error.clear();
    m_pending = "send"; m_busy = true; emit changed(); ensureGuest();
}
void PhoneLogin::signIn(const QString &phone, const QString &digits) {
    if (m_busy) return;
    const QString normalized = normalizePhone(phone);
    if (normalized.isEmpty()) { fail("invalid_phone"); return; }
    if (!QRegExp("[0-9]{4,8}").exactMatch(digits.trimmed())) { fail("invalid_digits"); return; }
    if (!m_phone.isEmpty() && normalized != m_phone && m_sent) { fail("phone_changed"); return; }
    m_phone = normalized; m_digits = digits.trimmed(); m_error.clear();
    m_pending = "signin"; m_busy = true; emit changed(); ensureGuest();
}
void PhoneLogin::ensureGuest() {
    if (!m_udid.isEmpty()) { advance(); return; }
    send(m_webDevice.isEmpty() ? "web" : "init");
}
void PhoneLogin::advance() {
    if (m_pending == "signin") {
        const QByteArray ts = QByteArray::number(QDateTime::currentMSecsSinceEpoch()/1000);
        const QByteArray sig = hmacSha1(clientSecret, QByteArray("digits") + clientId + source + ts);
        send("signin", field("client_id", clientId) + '&' + field("digits", m_digits) + "&grant_type=digits&signature=" + sig
            + "&source=" + source + "&timestamp=" + ts + '&' + field("username", m_phone));
    } else send("check");
}
void PhoneLogin::refreshCaptcha() {
    if (m_busy || m_udid.isEmpty()) return;
    m_busy = true; m_error.clear(); emit changed(); send("captcha");
}
void PhoneLogin::verifyCaptcha(const QString &input) {
    if (m_busy || !m_captchaRequired || m_udid.isEmpty()) return;
    if (input.trimmed().isEmpty()) { fail("invalid_captcha"); return; }
    m_busy = true; m_error.clear(); emit changed(); send("verify", field("input_text", input.trimmed()));
}
void PhoneLogin::send(const QString &stage, const QByteArray &form) {
    QString path = stage == "web" ? "/udid" : stage == "init" ? "/api/account/prod/init/udid_guest" :
        stage == "digits" ? "/api/account/prod/auth/digits" : stage == "signin" ? "/api/account/prod/sign_in" : "/captcha";
    // User requires direct HTTPS; certificate errors remain fatal. Web preheating is
    // intentionally isolated from the guest cookie store, as in the Android client.
    QNetworkRequest request(QUrl(QString("https://") + (stage == "web" ? "www.zhihu.com" : "api.zhihu.com") + path));
    QSslConfiguration ssl = request.sslConfiguration(); ssl.setPeerVerifyMode(QSslSocket::VerifyPeer); request.setSslConfiguration(ssl);
    request.setRawHeader("Accept", "application/json"); request.setRawHeader("Accept-Encoding", "identity");
    request.setRawHeader("User-Agent", "com.zhihu.android/Futureve/11.4.0 Mozilla/5.0 (Linux; Android 12; Android SDK built for arm64 Build/SE1A.220621.001; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/57.0.1000.10 Mobile Safari/537.36");
    request.setRawHeader("Authorization", m_authorization.toUtf8());
    if (!m_udid.isEmpty()) request.setRawHeader("x-udid", m_udid.toUtf8());
    request.setRawHeader("x-api-version", "3.0.93"); request.setRawHeader("x-app-version", "11.4.0");
    request.setRawHeader("x-app-build", "release"); request.setRawHeader("x-app-bundleid", source);
    request.setRawHeader("x-app-flavor", "honor"); request.setRawHeader("x-network-type", "3G");
    request.setRawHeader("x-page-id", "44"); request.setRawHeader("x-zse-93", "101_1_1.0");
    request.setRawHeader("x-app-za", "OS=Android&Release=12&Model=Android+SDK+built+for+arm64&VersionName=11.4.0&VersionCode=40408&Product=com.zhihu.android&Width=1080&Height=2274&Installer=%E8%8D%A3%E8%80%80%E5%95%86%E5%BA%97&DeviceType=AndroidPhone&Brand=Android");
    if (stage != "web") {
        QStringList cookies;
        for (QMap<QString,QString>::const_iterator it=m_cookies.begin(); it!=m_cookies.end(); ++it)
            if (it.key() != "d_c0") cookies << it.key() + '=' + it.value();
        if (!cookies.isEmpty()) request.setRawHeader("Cookie", cookies.join("; ").toUtf8());
    }
    QByteArray body = form;
    if (stage == "init") {
        // The upstream protocol expects Android-shaped device fields. No phone,
        // contacts, location or hardware identifiers are read from the BB10 device.
        body = "app_build=40408&app_install_time=" + QByteArray::number(QDateTime::currentMSecsSinceEpoch())
            + "&app_ticket=interface+is+empty&app_version=11.4.0&bt_ck=0&bundle_id=com.zhihu.android&cp_ct=4&cp_tp=arm64-v8a&cp_us=0&d_n=Android+SDK+built+for+arm64&fr_mem=1024&fr_st=8192&latitude=0.0&longitude=0.0&nt_st=0&ph_br=Android&ph_md=Android+SDK+built+for+arm64&ph_os=Android+12&pre_install=InterfaceIsNull&tt_mem=2048&tt_st=16384&tz_of=28800&zx_expired=0";
        const QByteArray ts = QByteArray::number(QDateTime::currentMSecsSinceEpoch()/1000);
        request.setRawHeader("x-app-id", "1355"); request.setRawHeader("x-sign-version", "2"); request.setRawHeader("x-req-ts", ts);
        request.setRawHeader("x-req-signature", hmacSha1("dd49a835-56e7-4a0f-95b5-efd51ea5397f", "13552" + body + ts));
    }
    QNetworkReply *reply;
    if (stage == "check") reply = m_network.get(request);
    else if (stage == "captcha") reply = m_network.put(request, QByteArray());
    else {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        reply = m_network.post(request, stage == "web" ? QByteArray() : encrypt(body));
    }
    reply->setProperty("stage", stage);
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
    QTimer *timer = new QTimer(reply); timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout())); connect(reply, SIGNAL(finished()), timer, SLOT(stop())); timer->start(20000);
}
void PhoneLogin::timeout() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender()->parent());
    if (reply && reply->isRunning()) { reply->setProperty("timedOut", true); reply->abort(); }
}
void PhoneLogin::finished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender()); if (!reply) return;
    reply->deleteLater();
    const QString stage = reply->property("stage").toString();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bb::data::JsonDataAccess json; const QVariantMap data = json.loadFromBuffer(reply->readAll()).toMap();
    const bool success = status >= 200 && status < 300 && reply->error() == QNetworkReply::NoError;
    const QVariantMap remoteError = data.value("error", data).toMap();
    const int code = remoteError.value("code").toInt();
    if (!success) {
        if (stage == "digits" && (code == 120001 || code == 120002)) { send("captcha"); return; }
        if (stage == "digits" && code == 120005 && !m_rechecked) { m_rechecked = true; send("check"); return; }
        QString error = reply->property("timedOut").toBool() ? "timeout" :
            reply->error() == QNetworkReply::SslHandshakeFailedError ? "tls_failed" :
            status >= 300 && status < 400 ? "redirect" : status > 0 ? "http_"+QString::number(status) : "network";
        const QString message = remoteError.value("message").toString();
        if (!message.isEmpty() && message.size() <= 300) error = "server:" + message;
        fail(error); return;
    }
    if (stage != "web" && (json.hasError() || data.isEmpty())) { fail("invalid_response"); return; }
    const QList<QNetworkCookie> received = qvariant_cast<QList<QNetworkCookie> >(reply->header(QNetworkRequest::SetCookieHeader));
    foreach (const QNetworkCookie &cookie, received) {
        if (cookie.name() == "d_c0") m_webDevice = QString::fromUtf8(cookie.value());
        else if (stage != "web") m_cookies[QString::fromLatin1(cookie.name())] = QString::fromUtf8(cookie.value());
    }
    if (stage == "web") {
        if (m_webDevice.isEmpty()) { fail("missing_device_cookie"); return; }
        send("init");
    } else if (stage == "init") {
        const QVariantMap guest = data.value("guest").toMap();
        const QString token = guest.value("access_token").toString(); m_udid = data.value("udid").toString();
        if (token.isEmpty() || m_udid.isEmpty()) { fail("invalid_response"); return; }
        const QVariantMap cookies = guest.value("cookie").toMap();
        for (QVariantMap::const_iterator it=cookies.begin(); it!=cookies.end(); ++it) if (!it.value().toString().isEmpty()) m_cookies[it.key()] = it.value().toString();
        m_authorization = guest.value("token_type", "bearer").toString() + ' ' + token; advance();
    } else if (stage == "check") {
        if (data.value("show_captcha", data.value("showCaptcha", false)).toBool()) send("captcha");
        else send("digits", field("username", m_phone) + '&' + field("client_id", clientId));
    } else if (stage == "captcha") {
        QString encoded = data.value("img_base64", data.value("imgBase64")).toString();
        if (encoded.startsWith("data:")) encoded = encoded.mid(encoded.indexOf(',')+1);
        if (encoded.isEmpty()) { fail("invalid_response"); return; }
        m_captchaImage = QVariant::fromValue(bb::cascades::Image(QByteArray::fromBase64(encoded.toLatin1())));
        m_captchaRequired = true; m_busy = false; emit changed();
    } else if (stage == "verify") {
        if (!data.value("success").toBool()) { fail("invalid_captcha"); return; }
        m_captchaRequired = false; m_captchaImage = QVariant();
        send("digits", field("username", m_phone) + '&' + field("client_id", clientId));
    } else if (stage == "digits") {
        m_sent = true; m_cooldown = 60; m_busy = false; m_captchaRequired = false; emit changed();
    } else if (stage == "signin") {
        if (data.value("access_token").toString().isEmpty()) { fail("invalid_response"); return; }
        const QVariantMap cookies = data.value("cookie").toMap();
        for (QVariantMap::const_iterator it=cookies.begin(); it!=cookies.end(); ++it) if (!it.value().toString().isEmpty()) m_cookies[it.key()] = it.value().toString();
        if (m_cookies.value("d_c0").isEmpty()) m_cookies["d_c0"] = m_webDevice;
        if (m_cookies.value("d_c0").isEmpty()) { fail("missing_device_cookie"); return; }
        QStringList pairs; for (QMap<QString,QString>::const_iterator it=m_cookies.begin(); it!=m_cookies.end(); ++it) pairs << it.key() + '=' + it.value();
        QVariantMap credentials = data; credentials["cookie"] = pairs.join("; ");
        m_busy = false; m_digits.clear(); emit changed(); emit credentialsReady(credentials);
    }
}
