#include "zhihu.hpp"
#include "signature.hpp"
#include "phone_login.hpp"
#include "daily/Requester/Requester.hpp"
#include "daily/WebImageView/WebImageView.hpp"
#include <bb/data/JsonDataAccess>
#include <bb/cascades/Application>
#include <bb/cascades/ThemeSupport>
#include <bb/cascades/VisualStyle>
#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>
#include <bb/system/InvokeTargetReply>
#include <bb/system/InvokeReplyError>
#include <bb/system/Clipboard>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkCookieJar>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTimer>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDate>
#include <QDateTime>
#include <QDeclarativeEngine>
#include <cstdio>
using namespace bb::cascades;
using bb::data::JsonDataAccess;
static QString digest(const QString &s) {
    return QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha1).toHex();
}
static QString escape(QString s) {
    return s.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;").replace('"', "&quot;");
}
static QVariant decode(const QByteArray &bytes) {
    JsonDataAccess json;
    QVariant value = json.loadFromBuffer(bytes);
    return json.hasError() ? QVariant() : value;
}
// Credentials are explicit per request; never retain upstream Set-Cookie in a shared jar.
class EmptyCookieJar : public QNetworkCookieJar {
public:
    EmptyCookieJar(QObject *parent) : QNetworkCookieJar(parent) {}
    bool setCookiesFromUrl(const QList<QNetworkCookie> &, const QUrl &) { return false; }
};
Zhihu::Zhihu(QObject *parent) : QObject(parent), m_network(this),
    m_invokeManager(new bb::system::InvokeManager(this)),
    m_settings(QDir::homePath() + "/data/settings.ini", QSettings::IniFormat), m_cacheBytes(0),
    m_verifying(false), m_imageRevision(0), m_followUnread(0),
    m_notificationUnread(0), m_epoch(0), m_imageActive(0), m_previewActive(false) {
    QDir().mkpath(QDir::homePath() + "/data/images");
    m_network.setCookieJar(new EmptyCookieJar(&m_network));
    m_accounts = m_settings.value("accounts").toList();
    m_active = m_settings.value("active").toString();
    Application::instance()->themeSupport()->setVisualStyle(dark() ? VisualStyle::Dark : VisualStyle::Bright);
    applyCacheLimit();
    refreshCacheUsage();
    QSettings dailySettings;
    dailySettings.setValue("newsLargeFont", largeText() ? "1" : "0");
    dailySettings.setValue("newsEyeProtectionMode", dark() ? "1" : "0");
    connect(m_invokeManager, SIGNAL(childCardDone(bb::system::CardDoneMessage)), this, SLOT(cleanupImagePreview()));
}
Zhihu::~Zhihu() { cleanupImagePreview(); }
// Only these disposable files participate in cache accounting and eviction.
// Account settings, reading history and saved items are deliberately excluded.
static QFileInfoList nativeCacheFiles(bool images) {
    QDir dir(QDir::homePath() + (images ? "/data/images" : "/data"));
    return dir.entryInfoList(images ? QStringList("*.jpg") : QStringList("feed_*.json"),
                             QDir::Files | QDir::NoSymLinks, QDir::Time | QDir::Reversed);
}
static void trimFiles(const QFileInfoList &files, qint64 limit) {
    qint64 total = 0;
    foreach (const QFileInfo &file, files) total += file.size();
    foreach (const QFileInfo &file, files) {
        if (total <= limit) break;
        if (QFile::remove(file.absoluteFilePath())) total -= file.size();
    }
}
int Zhihu::cacheLimitMb() const {
    int mb = m_settings.value("cacheLimitMb", 512).toInt();
    return mb == 1024 || mb == 2048 ? mb : 512;
}
void Zhihu::pruneCache() const {
    const qint64 share = qint64(cacheLimitMb()) * 1024 * 1024 / 4;
    trimFiles(nativeCacheFiles(true), share);
    trimFiles(nativeCacheFiles(false), share);
}
void Zhihu::applyCacheLimit() {
    // Four bounded stores share the selected total budget equally.
    const qint64 share = qint64(cacheLimitMb()) * 1024 * 1024 / 4;
    Requester::setCacheLimit(share);
    WebImageView::setCacheLimit(share);
    pruneCache();
}
void Zhihu::setCacheLimitMb(int mb) {
    if (mb != 512 && mb != 1024 && mb != 2048) return;
    if (mb == cacheLimitMb()) return;
    m_settings.setValue("cacheLimitMb", mb); m_settings.sync();
    QFile::setPermissions(m_settings.fileName(), QFile::ReadOwner | QFile::WriteOwner);
    applyCacheLimit(); refreshCacheUsage(); emit settingsChanged();
}
int Zhihu::cacheUsagePercent() const {
    return int(qMin(qint64(100), m_cacheBytes * 100 / (qint64(cacheLimitMb()) * 1024 * 1024)));
}
QString Zhihu::cacheUsageText() const {
    return QString("%1 MB / %2 MB (%3%)").arg(m_cacheBytes / 1048576.0, 0, 'f', 1)
        .arg(cacheLimitMb()).arg(cacheUsagePercent());
}
void Zhihu::refreshCacheUsage() {
    m_cacheBytes = 0;
    foreach (const QFileInfo &file, nativeCacheFiles(true)) m_cacheBytes += file.size();
    foreach (const QFileInfo &file, nativeCacheFiles(false)) m_cacheBytes += file.size();
    foreach (const QString &path, QStringList() << Requester::CACHE_DIR << WebImageView::CACHE_DIR) {
        QDirIterator files(path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while (files.hasNext()) { files.next(); m_cacheBytes += files.fileInfo().size(); }
    }
    emit cacheUsageChanged();
}
void Zhihu::clearCache() {
    foreach (const QFileInfo &file, nativeCacheFiles(true)) QFile::remove(file.absoluteFilePath());
    foreach (const QFileInfo &file, nativeCacheFiles(false)) QFile::remove(file.absoluteFilePath());
    Requester::clearCache(); WebImageView::clearCache();
    m_imageFailed.clear();
    refreshCacheUsage();
    emit toast(m_cacheBytes == 0 ? "cache_cleared" : "cache_clear_partial");
}
bool Zhihu::dark() const { return m_settings.value("dark", false).toBool(); }
bool Zhihu::largeText() const { return m_settings.value("large", false).toBool(); }
QVariantMap Zhihu::account() const {
    foreach (const QVariant &a, m_accounts) {
        QVariantMap row = a.toMap();
        if (row.value("id").toString() == m_active) { row.remove("cookie"); row.remove("mobile"); return row; }
    }
    QVariantMap empty;
    empty["id"] = ""; empty["name"] = ""; empty["headline"] = "";
    empty["avatar_url"] = ""; empty["masked"] = "";
    return empty;
}
QVariantList Zhihu::accounts() const {
    QVariantList result;
    foreach (const QVariant &a, m_accounts) {
        QVariantMap row = a.toMap(); row.remove("cookie"); row.remove("mobile"); result.append(row);
    }
    return result;
}
QString Zhihu::cookie() const {
    foreach (const QVariant &a, m_accounts)
        if (a.toMap().value("id").toString() == m_active) return a.toMap().value("cookie").toString();
    return "";
}
bool Zhihu::loggedIn() const { return !cookie().isEmpty(); }
QString Zhihu::scope() const { return digest(m_active.isEmpty() ? "guest" : m_active); }
QString Zhihu::storage(const QString &name) const { return QDir::homePath() + "/data/" + name + ".json"; }
QString Zhihu::readerBaseUrl() const {
    // A local origin is required for BB10 WebView to load cached file images.
    return QUrl::fromLocalFile(QDir::homePath() + "/data/reader.html").toString();
}
QVariant Zhihu::read(const QString &name) const {
    QFile file(storage(name));
    if (!file.open(QIODevice::ReadOnly) || file.size() > 8 * 1024 * 1024) return QVariant();
    return decode(file.readAll());
}
void Zhihu::write(const QString &name, const QVariant &data) const {
    JsonDataAccess json; QByteArray bytes;
    json.saveToBuffer(data, &bytes);
    if (json.hasError()) return;
    const QString path = storage(name), pending = path + ".new";
    QFile file(pending);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    if (file.write(bytes) != bytes.size()) { file.close(); file.remove(); return; }
    file.flush(); file.close();
    // POSIX rename replaces the destination atomically on the target QNX filesystem.
    ::rename(pending.toLocal8Bit().constData(), path.toLocal8Bit().constData());
    if (name.startsWith("feed_")) pruneCache();
}
bool Zhihu::saveSettings(bool night, bool large) {
    if (night == dark() && large == largeText()) return true;
    m_settings.setValue("dark", night); m_settings.setValue("large", large);
    m_settings.sync();
    QFile::setPermissions(m_settings.fileName(), QFile::ReadOwner | QFile::WriteOwner);
    Application::instance()->themeSupport()->setVisualStyle(night ? VisualStyle::Dark : VisualStyle::Bright);
    QSettings dailySettings;
    dailySettings.setValue("newsLargeFont", large ? "1" : "0");
    dailySettings.setValue("newsEyeProtectionMode", night ? "1" : "0"); dailySettings.sync();
    m_imageFailed.clear();
    emit settingsChanged();
    return true;
}
QString Zhihu::transportUrl(const QString &source) const {
    QUrl url(source);
    url.setScheme("https");
    // These mobile endpoints were verified with authenticated read-only requests.
    // Canonicalize legacy web paging links as well, keeping the original query.
    if (url.host() == "www.zhihu.com") {
        const QString path = url.path();
        if (path == "/api/v4/me" || path == "/api/v4/search_v3" ||
            path.startsWith("/api/v4/answers/") || path.startsWith("/api/v4/articles/") ||
            path.startsWith("/api/v4/questions/") || path.startsWith("/api/v4/members/") || path.startsWith("/api/v4/comment_v5/")) {
            url.setHost("api.zhihu.com"); url.setPath(path.mid(7));
        } else if (path == "/api/v3/moments") {
            url.setHost("api.zhihu.com"); url.setPath("/moments");
        }
    }
    return url.toString();
}
QNetworkReply *Zhihu::get(const QString &source, const QString &tag, QObject *receiver,
                          const char *slot, const QString &credential) {
    QUrl logical(source);
    bool media = tag == "image";
    bool daily = logical.host() == "news-at.zhihu.com";
    bool allowed = logical.host() == "api.zhihu.com" || logical.host() == "www.zhihu.com" || daily;
    if (media) allowed = allowed || logical.host().endsWith(".zhimg.com") || logical.host() == "zhimg.com";
    if (!allowed || (logical.scheme() != "https" && logical.scheme() != "http") || !logical.userInfo().isEmpty()) return 0;
    QString credentials = credential.isNull() ? cookie() : credential;
    // User requested direct HTTPS. Native BB10 10.3.1 probe verified this certificate
    // chain with peer verification enabled (validation/tls-probe/results-default.log).
    // All logical URLs, including older paging links, are upgraded before sending.
    QNetworkRequest request(QUrl(transportUrl(source)));
    request.setRawHeader("Accept", media ? "image/*" : "application/json");
    request.setRawHeader("Accept-Encoding", "identity");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/540.0 (KHTML, like Gecko) Ubuntu/10.10 Chrome/9.1.0.0 Safari/540.0");
    request.setRawHeader("Referer", "https://www.zhihu.com/"); // Direct HTTPS: verified by the native probe.
    request.setRawHeader("x-api-version", "3.0.91");
    QSslConfiguration ssl = request.sslConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyPeer);
    request.setSslConfiguration(ssl);
    if (!daily && !media && !credentials.isEmpty()) {
        request.setRawHeader("Cookie", credentials.toUtf8());
        QRegExp device("(?:^|;)\\s*d_c0=([^;]+)");
        if (device.indexIn(credentials) >= 0) {
            request.setRawHeader("x-zse-93", "101_3_3.0");
            request.setRawHeader("x-zse-96", zhihuSignature(request.url(), device.cap(1).trimmed().toUtf8()));
            request.setRawHeader("x-requested-with", "fetch");
        }
    }
    QNetworkReply *reply = m_network.get(request);
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    reply->setProperty("tag", tag); reply->setProperty("source", source);
    reply->setProperty("epoch", m_epoch);
    connect(reply, SIGNAL(finished()), receiver, slot);
    QTimer *timer = new QTimer(reply); timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(abortRequest()));
    connect(reply, SIGNAL(finished()), timer, SLOT(stop()));
    timer->start(20000);
    return reply;
}
void Zhihu::sslErrors(const QList<QSslError> &errors) {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    foreach (const QSslError &error, errors)
        qWarning() << "[ssl]" << (reply ? reply->url().host() : QString()) << int(error.error()) << error.errorString();
    // Never suppress certificate errors, especially on authenticated requests.
}
void Zhihu::abortRequest() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender()->parent());
    if (reply && reply->isRunning()) { reply->setProperty("timedOut", true); reply->abort(); }
}
QString Zhihu::error(QNetworkReply *reply) const {
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 300 && status < 400) return "redirect";
    if (reply->property("timedOut").toBool()) return "timeout";
    if (status == 401) return "unauthorized";
    if (status == 403) return "forbidden";
    if (status == 429) return "rate_limited";
    if (status >= 400) return "http_" + QString::number(status);
    if (reply->error() == QNetworkReply::SslHandshakeFailedError) return "tls_failed";
    if (reply->error() != QNetworkReply::NoError) return "network";
    return "";
}
void Zhihu::saveAccounts() {
    m_settings.setValue("accounts", m_accounts); m_settings.setValue("active", m_active);
    m_settings.sync();
    QFile::setPermissions(m_settings.fileName(), QFile::ReadOwner | QFile::WriteOwner);
}
void Zhihu::changedSession() {
    ++m_epoch; m_followUnread = 0; m_notificationUnread = 0;
    emit accountChanged(); emit countersChanged(); emit sessionChanged();
    refreshCounters();
}
void Zhihu::verifyAccount(const QString &value) {
    verifyCandidate(value, QVariantMap());
}
void Zhihu::clearAccountError() {
    if (!m_accountError.isEmpty()) { m_accountError.clear(); emit accountChanged(); }
}
// Accept raw values or a copied name=value; retain '=' and '|' inside values.
static QString cookieValue(QString value, const QString &name) {
    value = value.trimmed();
    if (value.startsWith(name + '=')) value.remove(0, name.size()+1);
    if (value.endsWith(';')) value.chop(1);
    if (value.size() >= 2 && ((value.startsWith('"') && value.endsWith('"')) ||
        (value.startsWith('\'') && value.endsWith('\'')))) value = value.mid(1,value.size()-2);
    if (value.isEmpty() || value.contains(';') || value.size() > 8192) return QString();
    for(int i=0;i<value.size();++i) if(value[i].unicode()<33 || value[i].unicode()>126) return QString();
    return value;
}
void Zhihu::verifyCookieValues(const QString &session, const QString &device) {
    if (m_verifying) return;
    const QString z = cookieValue(session,"z_c0"), d = cookieValue(device,"d_c0");
    if (z.isEmpty() || d.isEmpty()) { m_accountError="invalid_cookie_values"; emit accountChanged(); return; }
    verifyAccount("z_c0=" + z + "; d_c0=" + d);
}
QVariantMap Zhihu::splitCookieValues(const QString &text) const {
    const QString cookie = normalizedCookie(text); QVariantMap result;
    foreach(const QString &part,cookie.split(';',QString::SkipEmptyParts)) {
        const QString pair=part.trimmed(); const int separator=pair.indexOf('=');
        const QString name=pair.left(separator);
        if(name=="z_c0" || name=="d_c0") result[name]=cookieValue(pair.mid(separator+1),name);
    }
    return result;
}
QObject *Zhihu::createPhoneLogin(QObject *owner) {
    PhoneLogin *login = new PhoneLogin(owner ? owner : this);
    connect(login,SIGNAL(credentialsReady(QVariantMap)),this,SLOT(phoneLoginFinished(QVariantMap)));
    return login;
}
void Zhihu::phoneLoginFinished(const QVariantMap &credentials) {
    QVariantMap mobile;
    foreach(const QString &key,QStringList()<<"access_token"<<"refresh_token"<<"token_type"<<"expires_in")
        mobile[key]=credentials.value(key);
    verifyCandidate(credentials.value("cookie").toString(), mobile);
}
void Zhihu::verifyCandidate(const QString &value, const QVariantMap &mobile) {
    if (m_verifying) return;
    QString candidate = normalizedCookie(value);
    m_accountError.clear();
    if (candidate.isEmpty()) {
        m_accountError = "invalid_cookie"; emit accountChanged(); return;
    }
    QNetworkReply *reply = get("http://api.zhihu.com/me", "verify", this, SLOT(verifyFinished()), candidate);
    if (!reply) { m_accountError = "invalid_url"; emit accountChanged(); return; }
    reply->setProperty("candidate", candidate);
    reply->setProperty("mobile", mobile);
    m_verifying = true; emit accountChanged();
}
void Zhihu::verifyFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater(); m_verifying = false;
    if (reply->property("epoch").toInt() != m_epoch) { emit accountChanged(); return; }
    m_accountError = error(reply);
    QVariantMap profile = decode(reply->readAll()).toMap();
    if (m_accountError.isEmpty() && (profile.value("id").toString().isEmpty()
        || profile.value("name").toString().isEmpty())) m_accountError = "invalid_response";
    if (!m_accountError.isEmpty()) {
        if ((m_accountError == "unauthorized" || m_accountError == "forbidden" || m_accountError == "invalid_response")
            && !reply->property("candidate").toString().contains(QRegExp("(?:^|;)\\s*z_c0=\\S+")))
            m_accountError = "missing_session_cookie";
        emit accountChanged(); return;
    }
    QVariantMap a;
    a["id"] = profile.value("id"); a["name"] = profile.value("name");
    a["headline"] = profile.value("headline", ""); a["avatar_url"] = profile.value("avatar_url", "");
    a["url_token"] = profile.value("url_token", profile.value("id"));
    a["cookie"] = reply->property("candidate"); a["masked"] = "Cookie: ********";
    if (!reply->property("mobile").toMap().isEmpty()) a["mobile"] = reply->property("mobile");
    for (int i = m_accounts.size() - 1; i >= 0; --i)
        if (m_accounts[i].toMap().value("id") == a["id"]) m_accounts.removeAt(i);
    m_accounts.append(a); m_active = a["id"].toString(); saveAccounts(); changedSession();
    emit toast("account_verified");
}
QString Zhihu::normalizedCookie(const QString &value) {
    QString cookie = value.trimmed();
    if (cookie.startsWith("Cookie:", Qt::CaseInsensitive)) cookie = cookie.mid(7).trimmed();
    if (cookie.size() >= 2 && ((cookie.startsWith('"') && cookie.endsWith('"')) ||
        (cookie.startsWith('\'') && cookie.endsWith('\'')))) cookie = cookie.mid(1, cookie.size()-2).trimmed();
    if (cookie.size() > 16384 || cookie.contains('\r') || cookie.contains('\n')) return QString();
    QStringList pairs;
    foreach (const QString &part, cookie.split(';', QString::SkipEmptyParts)) {
        const QString pair = part.trimmed();
        const int separator = pair.indexOf('=');
        if (separator < 1 || !QRegExp("[A-Za-z0-9_!#$%&'*+.^`|~-]+").exactMatch(pair.left(separator))) return QString();
        for (int i = 0; i < pair.size(); ++i)
            if (pair[i].unicode() < 32 || pair[i].unicode() > 126) return QString();
        pairs << pair;
    }
    return pairs.join("; ");
}
QString Zhihu::clipboardText() const {
    bb::system::Clipboard clipboard;
    return QString::fromUtf8(clipboard.value("text/plain"));
}
bool Zhihu::copyAccountCredential(const QString &id) {
    foreach (const QVariant &account, m_accounts) {
        const QVariantMap row = account.toMap();
        if (row.value("id").toString() != id) continue;
        bb::system::Clipboard clipboard;
        const bool ok = clipboard.insert("text/plain", row.value("cookie").toString().toUtf8());
        emit toast(ok ? "credential_copied" : "clipboard_failed");
        return ok;
    }
    return false;
}
void Zhihu::switchAccount(const QString &id) {
    foreach (const QVariant &a, m_accounts) if (a.toMap().value("id").toString() == id) {
        m_active = id; saveAccounts(); changedSession(); return;
    }
}
void Zhihu::logout() { m_active.clear(); saveAccounts(); changedSession(); }
void Zhihu::removeAccount(const QString &id) {
    for (int i = m_accounts.size() - 1; i >= 0; --i)
        if (m_accounts[i].toMap().value("id").toString() == id) m_accounts.removeAt(i);
    if (m_active == id) m_active.clear();
    saveAccounts(); changedSession();
}
QString Zhihu::plain(const QString &html) {
    QString text = html;
    text.remove(QRegExp("<[^>]*>"));
    QRegExp numeric("&#(x[0-9a-fA-F]+|[0-9]+);");
    int pos = 0;
    while ((pos = numeric.indexIn(text, pos)) >= 0) {
        QString n = numeric.cap(1); bool ok = false;
        uint code = n.startsWith('x') ? n.mid(1).toUInt(&ok, 16) : n.toUInt(&ok);
        QString replacement = ok && code <= 0x10ffff ? QString::fromUcs4(&code, 1) : QString();
        text.replace(pos, numeric.matchedLength(), replacement); pos += replacement.size();
    }
    return text.replace("&nbsp;", " ").replace("&lt;", "<").replace("&gt;", ">")
        .replace("&quot;", "\"").replace("&#39;", "'").replace("&amp;", "&").simplified();
}
QString Zhihu::identity(const QVariantMap &row) { return row.value("kind").toString() + ":" + row.value("id").toString(); }
QVariantMap Zhihu::normalizeComment(const QVariantMap &raw) {
    const QString id = raw.value("id").toString();
    if (id.isEmpty()) return QVariantMap();
    QVariantMap author = raw.value("author").toMap();
    if (author.value("member").type() == QVariant::Map) author = author.value("member").toMap();
    QVariantMap row;
    row["kind"] = "comment"; row["id"] = id;
    row["author"] = raw.value("author").type() == QVariant::String ? raw.value("author") : author.value("name", "");
    row["avatar"] = raw.value("avatar", author.value("avatar_url", ""));
    row["author_id"] = author.value("id", ""); row["author_token"] = author.value("url_token", author.value("id", ""));
    row["content"] = plain(raw.value("content").toString());
    row["deleted"] = raw.value("is_delete", false);
    row["likes"] = raw.value("like_count", raw.value("vote_count", raw.value("likes", 0)));
    row["children"] = raw.value("child_comment_count", 0);
    const qint64 time = raw.value("created_time", raw.value("time", 0)).toLongLong();
    row["time"] = time;
    row["time_text"] = time > 0 ? QDateTime::fromMSecsSinceEpoch(time * 1000).toString("yyyy-MM-dd hh:mm") : "";
    QVariantMap reply = raw.value("reply_to").toMap();
    if (reply.isEmpty() && raw.value("reply_to_author").type() == QVariant::Map) {
        reply["author"] = raw.value("reply_to_author").toMap().value("name", "");
        reply["content"] = ""; reply["status"] = 0;
    }
    if (!reply.isEmpty()) {
        reply["status"] = reply.value("status", 0).toInt();
        reply["content"] = plain(reply.value("content").toString());
        row["reply_to"] = reply;
    }
    return row;
}
QVariantMap Zhihu::normalize(const QVariantMap &raw) {
    QVariantMap target = raw;
    // Feed/search/collection wrappers differ; unwrap only structured content objects.
    if (raw.value("object").type() == QVariant::Map) target = raw.value("object").toMap();
    else if (raw.value("target").type() == QVariant::Map) target = raw.value("target").toMap();
    else if (raw.value("content").type() == QVariant::Map) target = raw.value("content").toMap();
    QString kind = target.value("type").toString();
    if (kind.isEmpty() && target.contains("images")) kind = "daily";
    if (kind != "answer" && kind != "article" && kind != "question" && kind != "daily") return QVariantMap();
    QString id = target.value("id").toString();
    if (id.isEmpty()) return QVariantMap();
    QVariantMap author = target.value("author").toMap(), question = target.value("question").toMap();
    QString title = target.value("title").toString();
    if (title.isEmpty()) title = question.value("title").toString();
    QString summary = target.value("excerpt", target.value("detail")).toString();
    QVariantMap row;
    row["id"] = id; row["kind"] = kind; row["title"] = plain(title);
    row["summary"] = plain(summary).left(320); row["author"] = author.value("name", "");
    row["author_id"] = author.value("id", ""); row["author_token"] = author.value("url_token", author.value("id", ""));
    row["author_avatar"] = author.value("avatar_url", "");
    row["votes"] = target.value("voteup_count", 0); row["comments"] = target.value("comment_count", 0);
    row["answers"] = target.value("answer_count", 0); row["question_id"] = question.value("id", "");
    QString image = target.value("thumbnail").toString();
    QVariantList images = target.value("images").toList();
    if (image.isEmpty() && !images.isEmpty()) image = images.first().toString();
    row["image"] = image;
    QString url = "http://www.zhihu.com/" + kind + "/" + id;
    if (kind == "answer") url = "http://www.zhihu.com/question/" + question.value("id").toString() + "/answer/" + id;
    if (kind == "article") url = "http://zhuanlan.zhihu.com/p/" + id;
    if (kind == "daily") url = "http://daily.zhihu.com/story/" + id;
    row["url"] = url;
    return row;
}
bool Zhihu::isSaved(const QVariantMap &item) const {
    foreach (const QVariant &v, read("saved_" + scope()).toList())
        if (identity(v.toMap()) == identity(item)) return true;
    return false;
}
void Zhihu::toggleSaved(const QVariantMap &item) {
    if (item.value("id").toString().isEmpty()) return;
    QVariantList rows = read("saved_" + scope()).toList();
    bool removed = false;
    for (int i = rows.size() - 1; i >= 0; --i) if (identity(rows[i].toMap()) == identity(item)) {
        rows.removeAt(i); removed = true;
    }
    if (!removed) {
        QVariantMap saved = item;
        saved["savedAt"] = QDateTime::currentMSecsSinceEpoch();
        rows.prepend(saved);
    }
    write("saved_" + scope(), rows); emit libraryChanged();
    emit toast(removed ? "unsaved" : "saved");
}
void Zhihu::remember(const QVariantMap &item) {
    if (item.value("id").toString().isEmpty()) return;
    QVariantList rows = read("history_" + scope()).toList();
    for (int i = rows.size() - 1; i >= 0; --i)
        if (identity(rows[i].toMap()) == identity(item)) rows.removeAt(i);
    QVariantMap visited = item;
    visited["visitedAt"] = QDateTime::currentMSecsSinceEpoch();
    rows.prepend(visited); while (rows.size() > 300) rows.removeLast();
    write("history_" + scope(), rows); emit libraryChanged();
}
void Zhihu::clearHistory() { write("history_" + scope(), QVariantList()); emit libraryChanged(); }
QString Zhihu::image(const QString &url) {
    if (url.isEmpty()) return "";
    QString source = url; QUrl parsed(source); parsed.setScheme("http"); source = parsed.toString();
    QString path = QDir::homePath() + "/data/images/" + digest(source) + ".jpg";
    if (QFile::exists(path)) return QUrl::fromLocalFile(path).toString();
    if (!m_imagePending.contains(source) && !m_imageFailed.contains(source)) {
        m_imagePending.insert(source); m_imageQueue.append(source);
        QTimer::singleShot(0, this, SLOT(pumpImages()));
    }
    return "";
}
void Zhihu::pumpImages() {
    while (m_imageActive < 4 && !m_imageQueue.isEmpty()) {
        QString source = m_imageQueue.takeFirst();
        QNetworkReply *reply = get(source, "image", this, SLOT(imageFinished()), QString(""));
        if (!reply) { m_imagePending.remove(source); m_imageFailed.insert(source); continue; }
        ++m_imageActive;
    }
}
void Zhihu::imageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater(); --m_imageActive;
    QString source = reply->property("source").toString(); m_imagePending.remove(source);
    bool ok = false;
    if (error(reply).isEmpty()) {
        QByteArray bytes = reply->readAll();
        if (bytes.size() < 8 * 1024 * 1024 && !bytes.isEmpty()
            && reply->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("image/")) {
            QFile file(QDir::homePath() + "/data/images/" + digest(source) + ".jpg");
            if (file.open(QIODevice::WriteOnly)) ok = file.write(bytes) == bytes.size();
        }
    }
    if (ok) { pruneCache(); ++m_imageRevision; emit imagesChanged(); } else m_imageFailed.insert(source);
    if (source == m_previewSource) {
        m_previewSource.clear();
        if (ok) showImagePreview(QDir::homePath() + "/data/images/" + digest(source) + ".jpg");
        else emit toast("image_load_failed");
    }
    pumpImages();
}
bool Zhihu::openReaderImage(const QString &link) {
    const QUrl url(link);
    if (url.scheme() != "zhihu-image" || url.host() != "preview") return false;
    return openImage(url.queryItemValue("url"));
}
bool Zhihu::openImage(const QString &value) {
    QUrl url(value);
    if ((url.scheme() != "http" && url.scheme() != "https") || !url.userInfo().isEmpty() ||
        !(url.host().endsWith(".zhimg.com") || url.host() == "zhimg.com")) {
        emit toast("image_load_failed"); return false;
    }
    if (m_previewActive) return false;
    url.setScheme("http"); const QString source = url.toString();
    m_imageFailed.remove(source);
    const QString cached = image(source);
    if (!cached.isEmpty()) return showImagePreview(QUrl(cached).toLocalFile());
    m_previewSource = source;
    emit toast("image_loading"); return true;
}
// Adapted from BBTiebaUI: the system card cannot read another app's private cache.
// Export only the selected image, and remove our shared copy when the card closes.
bool Zhihu::showImagePreview(const QString &localPath) {
    // Cache names all end in .jpg; sniff the bytes instead of relying on the
    // suffix or optional Qt image-reader plugins on BB10.
    QFile input(localPath);
    if (!input.open(QIODevice::ReadOnly)) { emit toast("image_load_failed"); return false; }
    const QByteArray header = input.read(16); input.close();
    QByteArray format;
    if (header.startsWith(QByteArray::fromHex("89504e470d0a1a0a"))) format = "png";
    else if (header.startsWith(QByteArray::fromHex("ffd8ff"))) format = "jpeg";
    else if (header.startsWith("GIF87a") || header.startsWith("GIF89a")) format = "gif";
    else if (header.startsWith("BM")) format = "bmp";
    if (format.isEmpty()) {
        emit toast("image_unsupported"); return false;
    }
    const QString directory = "/accounts/1000/shared/misc/zhihu-preview";
    if (!QDir().mkpath(directory)) { emit toast("image_shared_failed"); return false; }
    cleanupImagePreview();
    foreach (const QString &extension, QStringList()<<"jpg"<<"png"<<"gif"<<"bmp")
        QFile::remove(directory + "/preview." + extension);
    const QString path = directory + "/preview." + QString::fromLatin1(format == "jpeg" ? QByteArray("jpg") : format);
    if (!QFile::copy(localPath, path)) { emit toast("image_shared_failed"); return false; }
    m_sharedPreviewPath = path;
    bb::system::InvokeRequest request;
    request.setTarget("sys.pictures.card.previewer"); request.setAction("bb.action.VIEW");
    request.setMimeType("image/" + QString::fromLatin1(format)); request.setUri(QUrl::fromLocalFile(path));
    bb::system::InvokeTargetReply *reply = m_invokeManager->invoke(request);
    if (!reply) { cleanupImagePreview(); emit toast("image_preview_failed"); return false; }
    m_previewActive = true;
    connect(reply, SIGNAL(finished()), this, SLOT(imageInvokeFinished()));
    qDebug() << "[image-preview] requested" << format;
    return true;
}
void Zhihu::imageInvokeFinished() {
    bb::system::InvokeTargetReply *reply = qobject_cast<bb::system::InvokeTargetReply *>(sender());
    if (!reply) return;
    qDebug() << "[image-preview] result" << int(reply->error()) << reply->errorCode();
    if (reply->error() != bb::system::InvokeReplyError::None) {
        cleanupImagePreview(); emit toast("image_preview_failed");
    }
    reply->deleteLater();
}
void Zhihu::cleanupImagePreview() {
    if (!m_sharedPreviewPath.isEmpty()) {
        const bool removed = QFile::remove(m_sharedPreviewPath);
        qDebug() << "[image-preview] shared copy removed" << removed;
    }
    m_sharedPreviewPath.clear(); m_previewSource.clear(); m_previewActive = false;
}
void Zhihu::openBrowser(const QString &url) {
    QUrl parsed(url);
    if (parsed.scheme() != "http" && parsed.scheme() != "https") return;
    bb::system::InvokeManager *manager = new bb::system::InvokeManager(this);
    bb::system::InvokeRequest request;
    request.setTarget("sys.browser"); request.setAction("bb.action.OPEN"); request.setUri(parsed);
    manager->invoke(request); QTimer::singleShot(10000, manager, SLOT(deleteLater()));
}
QVariantMap Zhihu::linkTarget(const QString &value) const {
    QUrl url(value); QString path = url.path(), host = url.host();
    if (url.scheme() == "zhihu") {
        QString kind;
        if (host == "answers") kind = "answer";
        if (host == "questions") kind = "question";
        if (host == "articles") kind = "article";
        QRegExp idPattern("^/([0-9]+)$");
        if (!kind.isEmpty() && idPattern.exactMatch(path)) {
            QVariantMap raw; raw["type"] = kind; raw["id"] = idPattern.cap(1);
            return normalize(raw);
        }
        return QVariantMap();
    }
    if (host != "www.zhihu.com" && host != "zhuanlan.zhihu.com" && host != "daily.zhihu.com") return QVariantMap();
    QRegExp answer("/question/[0-9]+/answer/([0-9]+)"), question("/question/([0-9]+)"),
        article("/p/([0-9]+)"), daily("/story/([0-9]+)");
    QVariantMap row;
    if (answer.indexIn(path) >= 0) { row["kind"] = "answer"; row["id"] = answer.cap(1); }
    else if (question.indexIn(path) >= 0) { row["kind"] = "question"; row["id"] = question.cap(1); }
    else if (host == "zhuanlan.zhihu.com" && article.indexIn(path) >= 0) { row["kind"] = "article"; row["id"] = article.cap(1); }
    else if (host == "daily.zhihu.com" && daily.indexIn(path) >= 0) { row["kind"] = "daily"; row["id"] = daily.cap(1); }
    else return QVariantMap();
    row["title"] = ""; row["summary"] = ""; row["author"] = ""; row["image"] = ""; row["url"] = value;
    row["votes"] = 0; row["comments"] = 0; row["answers"] = 0; row["question_id"] = "";
    return row;
}
void Zhihu::refreshCounters() {
    if (!loggedIn()) return;
    get("http://api.zhihu.com/moments/recent?type=raw", "followCounter", this, SLOT(counterFinished()));
    get("http://api.zhihu.com/notifications/v3/message/v3?limit=20", "notificationCounter", this, SLOT(counterFinished()));
}
void Zhihu::counterFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();
    if (reply->property("epoch").toInt() != m_epoch || !error(reply).isEmpty()) return;
    QVariantMap root = decode(reply->readAll()).toMap(); int count = 0;
    foreach (const QVariant &v, root.value("data").toList()) count += v.toMap().value("unread_count").toInt();
    if (reply->property("tag").toString() == "followCounter") m_followUnread = count;
    else m_notificationUnread = root.value("unread").toMap().value("message").toMap().value("count").toInt();
    emit countersChanged();
}
QString Zhihu::document(const QVariantMap &data, const QVariantMap &item) {
    QString title = data.value("title", item.value("title")).toString();
    if (title.isEmpty()) title = data.value("question").toMap().value("title").toString();
    QString body = data.value("body", data.value("content", data.value("detail"))).toString();
    // Cache article images alongside text for history and saved reading. Prefer
    // full-size lazy-loading URLs; use local files on subsequent/offline opens.
    QRegExp active("<(script|style|iframe|object|embed)[^>]*>.*</\\1\\s*>", Qt::CaseInsensitive);
    active.setMinimal(true); body.remove(active);
    body.remove(QRegExp("<(script|iframe|object|embed)[^>]*/?>", Qt::CaseInsensitive));
    QRegExp handlers("\\s+on[a-z]+\\s*=\\s*(\"[^\"]*\"|'[^']*'|[^\\s>]+)", Qt::CaseInsensitive);
    body.remove(handlers);
    QRegExp imageLinks("<a\\b[^>]*>\\s*(<img\\b[^>]*>)\\s*</a\\s*>", Qt::CaseInsensitive);
    imageLinks.setMinimal(true); body.replace(imageLinks, "\\1");
    QRegExp images("<img[^>]*>", Qt::CaseInsensitive); images.setMinimal(true);
    int pos = 0;
    while ((pos = images.indexIn(body, pos)) >= 0) {
        QString tag = images.cap(0), replacement;
        QString source;
        foreach (const QString &attribute, QStringList() << "data-original" << "data-actualsrc" << "src") {
            QRegExp src("\\s" + attribute + "\\s*=\\s*[\"']([^\"']+)[\"']", Qt::CaseInsensitive);
            if (src.indexIn(tag) >= 0) { source = src.cap(1); break; }
        }
        if (!source.isEmpty()) {
            QUrl imageUrl(source.replace("&amp;", "&"));
            if (imageUrl.scheme().isEmpty()) imageUrl.setScheme("http");
            if (imageUrl.host().endsWith(".zhimg.com")) {
                imageUrl.setScheme("http");
                QString path = image(imageUrl.toString());
                if (path.isEmpty()) path = transportUrl(imageUrl.toString());
                QUrl preview("zhihu-image://preview"); preview.addQueryItem("url", imageUrl.toString());
                replacement = "<a href=\"" + escape(QString::fromLatin1(preview.toEncoded())) + "\"><img src=\"" + escape(path) + "\" alt=\"\"></a>";
            }
        }
        body.replace(pos, images.matchedLength(), replacement); pos += replacement.size();
    }
    QString author = data.value("author").toMap().value("name", item.value("author")).toString();
    return "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; img-src https: file: data:; style-src 'unsafe-inline'\">"
        "<style>body{margin:20px;background:" + QString(dark() ? "#000;color:#eee" : "#fff;color:#222") +
        ";font-family:sans-serif;font-size:" + QString(largeText() ? "24" : "20") +
        "px;line-height:1.7;word-wrap:break-word}h1{font-size:1.25em;line-height:1.45}img{max-width:100%;height:auto}"
        ".reader-author{color:#888}a{color:#1687e0}pre{white-space:pre-wrap}blockquote{margin:12px;color:#888}table{max-width:100%}</style></head><body><h1>" +
        escape(title) + "</h1><p class=\"reader-author\">" + escape(author) + "</p>" + body + "</body></html>";
}
QObject *Zhihu::createFeed(const QString &kind, const QString &param, QObject *owner) {
    Feed *feed = new Feed(this, kind, param, owner ? owner : this);
    QDeclarativeEngine::setObjectOwnership(feed, QDeclarativeEngine::CppOwnership);
    return feed;
}
void Zhihu::releaseFeed(QObject *object) {
    Feed *feed = qobject_cast<Feed *>(object);
    if (feed) feed->deleteLater();
}
Feed::Feed(Zhihu *client, const QString &kind, const QString &param, QObject *parent) :
    QObject(parent), m_client(client), m_model(new ArrayDataModel(this)), m_groupedModel(0), m_kind(kind), m_param(param),
    m_busy(false), m_loaded(false), m_cached(false), m_dailyShortComments(false), m_continueComments(false), m_generation(0) {
    connect(client, SIGNAL(sessionChanged()), this, SLOT(reset()));
    connect(client, SIGNAL(libraryChanged()), this, SLOT(libraryChanged()));
}
int Feed::count() const { return qMax(0, m_model->size() - (m_kind == "question" && !m_heading.isEmpty() ? 1 : 0)); }
void Feed::setHeading(const QString &heading) {
    if (heading == m_heading) return;
    m_heading = heading;
    if (m_kind != "question") return;
    QVariantList rows;
    for (int i = 0; i < m_model->size(); ++i)
        if (m_model->value(i).toMap().value("kind") != "questionHeader") rows << m_model->value(i);
    replace(rows);
}
QString Feed::cacheName() const {
    return "feed_" + m_client->scope() + "_" + digest(m_kind + ":" + m_param);
}
QString Feed::initialUrl() const {
    if (m_kind == "comments") {
        const QString kind = m_item.value("kind").toString(), id = m_item.value("id").toString();
        if (kind == "daily") return "http://news-at.zhihu.com/api/4/story/" + id + "/long-comments";
        if (kind == "answer" || kind == "article" || kind == "question")
            return "http://api.zhihu.com/comment_v5/" + kind + "s/" + id + "/root_comment?order_by=score&limit=20&offset=";
    }
    if (m_kind == "commentReplies") return "http://api.zhihu.com/comment_v5/comment/" + m_item.value("id").toString() + "/child_comment?limit=20&offset=";
    const QString member = QString::fromLatin1(QUrl::toPercentEncoding(m_param));
    if (m_kind == "profile") return "http://api.zhihu.com/people/" + member;
    if (m_kind == "memberQuestions") return "http://api.zhihu.com/members/" + member + "/questions?limit=20&include=data[*].answer_count,excerpt";
    if (m_kind == "memberAnswers") return "http://api.zhihu.com/members/" + member + "/answers?limit=20&sort_by=created&include=data[*].excerpt,question,author,thumbnail,voteup_count,comment_count";
    if (m_kind == "memberArticles") return "http://api.zhihu.com/people/" + member + "/articles?limit=20&sort_by=created&include=data[*].excerpt,author,thumbnail,voteup_count,comment_count";
    if (m_kind == "home") return "http://api.zhihu.com/topstory/recommend";
    if (m_kind == "follow") return "http://api.zhihu.com/moments?limit=10";
    if (m_kind == "daily") return "http://news-at.zhihu.com/api/4/news/latest";
    if (m_kind == "search") return "http://api.zhihu.com/search_v3?t=general&q=" +
        QString::fromLatin1(QUrl::toPercentEncoding(m_param)) + "&correction=1&offset=0&limit=20&search_source=Normal&show_all_topics=0";
    if (m_kind == "question") return "http://api.zhihu.com/questions/" + m_param +
        "/feeds?limit=20&order=default&include=data[*].content,excerpt,voteup_count,comment_count,question,author";
    if (m_kind == "detail") {
        QString kind = m_item.value("kind").toString(), id = m_item.value("id").toString();
        if (kind == "daily") return "http://news-at.zhihu.com/api/4/news/" + id;
        if (kind == "answer" || kind == "article") return "http://api.zhihu.com/" + kind + "s/" + id + "?include=content,author,question,voteup_count,comment_count";
    }
    if (m_kind == "notifications") return "http://api.zhihu.com/notifications/v3/message/v3?limit=20";
    if (m_kind == "notificationItems") return "http://api.zhihu.com/notifications/v3/timeline/entry/" + m_param + "?limit=20";
    return "";
}
void Feed::setItem(const QVariantMap &item) { m_item = item; m_param = Zhihu::identity(item); }
void Feed::replace(const QVariantList &rows) {
    QVariantList visibleRows = rows;
    if (m_kind == "question" && !m_heading.isEmpty()) {
        QVariantMap heading; heading["kind"] = "questionHeader"; heading["title"] = m_heading;
        visibleRows.prepend(heading);
    }
    ArrayDataModel *old = m_model; m_model = new ArrayDataModel(visibleRows, this);
    GroupDataModel *oldGroups = m_groupedModel;
    m_groupedModel = 0;
    if (m_kind == "history" || m_kind == "saved") {
        m_groupedModel = new GroupDataModel(QStringList() << "date" << "libraryAt", this);
        m_groupedModel->setGrouping(ItemGrouping::ByFullValue);
        m_groupedModel->setSortedAscending(false);
        QVariantList datedRows;
        foreach (const QVariant &value, rows) {
            QVariantMap row = value.toMap();
            const qint64 timestamp = row.value(m_kind == "history" ? "visitedAt" : "savedAt").toLongLong();
            // Older versions did not record dates. Keep those entries in an
            // explicit undated group rather than inventing a visit/save date.
            row["date"] = timestamp > 0 ? QDateTime::fromMSecsSinceEpoch(timestamp).date().toString(Qt::ISODate) : "0000-00-00";
            row["libraryAt"] = timestamp;
            datedRows.append(row);
        }
        m_groupedModel->insertList(datedRows);
    }
    m_seen.clear(); foreach (const QVariant &v, rows) m_seen.insert(Zhihu::identity(v.toMap()));
    emit changed(); old->deleteLater();
    if (oldGroups) oldGroups->deleteLater();
}
void Feed::reset() {
    ++m_generation; m_busy = false; m_loaded = false; m_cached = false;
    m_error.clear(); m_html.clear(); m_profile.clear(); m_next.clear();
    m_dailyShortComments = false; m_continueComments = false; replace(QVariantList());
    // Pages reload lazily when selected; old replies cannot cross account boundaries.
}
void Feed::libraryChanged() {
    if (m_kind == "history" || m_kind == "saved") load();
}
void Feed::load(bool more) {
    if (m_busy || (more && m_next.isEmpty())) return;
    if (!more) { m_dailyShortComments = false; m_continueComments = false; }
    m_error.clear();
    if (m_kind == "history" || m_kind == "saved") {
        replace(m_client->read(m_kind + "_" + m_client->scope()).toList());
        m_loaded = true; emit changed(); return;
    }
    if ((m_kind == "follow" || m_kind == "notifications" || m_kind == "notificationItems" ||
        ((m_kind == "comments" || m_kind == "commentReplies") && m_item.value("kind").toString() != "daily")) && !m_client->loggedIn()) {
        m_error = "login_required"; m_loaded = true; emit changed(); return;
    }
    if (!more && !m_loaded) {
        QVariantMap cache = m_client->read(cacheName()).toMap();
        if (!cache.isEmpty()) {
            if (m_kind == "detail") m_html = m_client->document(cache.value("detail").toMap(), m_item);
            else if (m_kind == "profile") m_profile = cache.value("profile").toMap();
            else replace(cache.value("rows").toList());
            m_cached = true;
        }
    }
    QString url = more ? m_next : initialUrl();
    QUrl parsed(url); parsed.setScheme("http"); url = parsed.toString();
    QNetworkReply *reply = m_client->get(url, m_kind, this, SLOT(finished()));
    if (!reply) { m_error = "invalid_url"; m_loaded = true; emit changed(); return; }
    reply->setProperty("more", more); reply->setProperty("generation", m_generation);
    m_busy = true; emit changed();
}
void Feed::finished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();
    if (reply->property("generation").toInt() != m_generation) return;
    m_busy = false; m_loaded = true; m_error = m_client->error(reply);
    QVariant root;
    if (m_error.isEmpty()) {
        QByteArray bytes = reply->readAll();
        if (bytes.size() <= 8 * 1024 * 1024) root = decode(bytes);
        if (root.type() != QVariant::Map || root.toMap().contains("error")) m_error = "invalid_response";
    }
    if (m_error.isEmpty()) consume(root.toMap(), reply->property("more").toBool());
    qDebug() << "[feed]" << m_kind << "count=" << count() << "error=" << m_error
             << "more=" << hasMore() << "html=" << m_html.size();
    emit changed();
    // An empty final long-comment page must transition to short comments even
    // though the list size did not change and no new scroll event will fire.
    if (m_continueComments && m_error.isEmpty()) { m_continueComments = false; load(true); }
}
void Feed::consume(const QVariantMap &data, bool more) {
    if (m_kind == "profile") {
        if (data.value("id").toString().isEmpty()) { m_error = "invalid_response"; return; }
        m_profile = data; m_cached = false;
        QVariantMap cache; cache["profile"] = data; m_client->write(cacheName(), cache); return;
    }
    if (m_kind == "detail") {
        if (!data.contains("body") && !data.contains("content") && !data.contains("detail")) {
            m_error = "invalid_response"; return;
        }
        m_html = m_client->document(data, m_item); m_cached = false;
        QVariantMap cache; cache["detail"] = data; m_client->write(cacheName(), cache); return;
    }
    const bool dailyComments = m_kind == "comments" && m_item.value("kind").toString() == "daily";
    QVariantList source;
    if (m_kind == "daily") source = data.value("stories").toList();
    else if (dailyComments) source = data.value("comments").toList();
    else if (m_kind == "notifications") source = data.value("head").toList();
    else source = data.value("data").toList();
    if (!data.contains(m_kind == "daily" ? "stories" : (dailyComments ? "comments" : "data"))) { m_error = "invalid_response"; return; }
    QVariantList rows;
    foreach (const QVariant &value, source) {
        QVariantMap raw = value.toMap(), row;
        if (m_kind == "comments" || m_kind == "commentReplies") row = Zhihu::normalizeComment(raw);
        else if (m_kind == "daily") { raw["type"] = "daily"; row = Zhihu::normalize(raw); }
        else if (m_kind == "notifications") {
            QStringList names; names << "comment" << "like" << "favlist_me" << "follow";
            QString name = raw.value("entry_name").toString();
            if (name.isEmpty() && rows.size() < names.size()) name = names.at(rows.size());
            if (name.isEmpty()) continue;
            row["kind"] = "notificationGroup"; row["id"] = name;
            row["title"] = Zhihu::plain(raw.value("detail_title", raw.value("name", raw.value("title", name))).toString());
            row["summary"] = Zhihu::plain(raw.value("content", "").toString());
            row["unread"] = raw.value("unread_count", 0);
            row["author"] = ""; row["image"] = ""; row["votes"] = 0; row["comments"] = 0;
        } else if (m_kind == "notificationItems") {
            QVariantMap content = raw.value("content").toMap();
            QString link = content.value("target_link", raw.value("target").toMap().value("url")).toString();
            row = m_client->linkTarget(link);
            if (row.isEmpty()) {
                row["kind"] = "notification"; row["id"] = raw.value("id", raw.value("verb")).toString();
                row["url"] = link; row["image"] = ""; row["author"] = "";
                row["votes"] = 0; row["comments"] = 0;
            }
            row["title"] = Zhihu::plain(content.value("title", raw.value("detail_title")).toString());
            row["summary"] = Zhihu::plain(content.value("text", content.value("sub_title", content.value("abstract_text", ""))).toString());
        } else {
            if (m_kind == "memberQuestions") raw["type"] = "question";
            if (m_kind == "memberAnswers") raw["type"] = "answer";
            if (m_kind == "memberArticles") raw["type"] = "article";
            row = Zhihu::normalize(raw);
        }
        if (!row.isEmpty() && (!more || !m_seen.contains(Zhihu::identity(row)))) {
            rows.append(row); if (more) m_seen.insert(Zhihu::identity(row));
        }
    }
    QString next;
    if (dailyComments) {
        const QString base = "http://news-at.zhihu.com/api/4/story/" + m_item.value("id").toString();
        if (!source.isEmpty()) next = base + (m_dailyShortComments ? "/short-comments/before/" : "/long-comments/before/") + source.last().toMap().value("id").toString();
        else if (!m_dailyShortComments) {
            m_dailyShortComments = true; next = base + "/short-comments"; m_continueComments = true;
        }
    } else if (m_kind == "daily" && !data.value("date").toString().isEmpty() && !source.isEmpty())
        next = "http://news-at.zhihu.com/api/4/news/before/" + data.value("date").toString();
    else {
        QVariantMap paging = data.value("paging").toMap();
        if (!paging.value("is_end", true).toBool()) next = paging.value("next").toString();
    }
    if (more && rows.isEmpty() && !m_continueComments) next.clear();
    if (more && !rows.isEmpty()) m_model->append(rows);
    else if (!more) replace(rows);
    m_next = next; m_cached = false;
    QVariantList all;
    for (int i = 0; i < m_model->size() && all.size() < 100; ++i)
        if (m_model->value(i).toMap().value("kind") != "questionHeader") all.append(m_model->value(i));
    QVariantMap cache; cache["rows"] = all; m_client->write(cacheName(), cache);
}
