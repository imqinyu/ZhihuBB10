#ifndef ZHIHU_HPP
#define ZHIHU_HPP
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QSet>
#include <QStringList>
#include <QSslError>
#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/GroupDataModel>
class QNetworkReply;
class Feed;
namespace bb { namespace system { class InvokeManager; } }
class Zhihu : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark NOTIFY settingsChanged)
    Q_PROPERTY(bool largeText READ largeText NOTIFY settingsChanged)
    Q_PROPERTY(int cacheLimitMb READ cacheLimitMb WRITE setCacheLimitMb NOTIFY settingsChanged)
    Q_PROPERTY(QString cacheUsageText READ cacheUsageText NOTIFY cacheUsageChanged)
    Q_PROPERTY(int cacheUsagePercent READ cacheUsagePercent NOTIFY cacheUsageChanged)
    Q_PROPERTY(QVariantMap account READ account NOTIFY accountChanged)
    Q_PROPERTY(QVariantList accounts READ accounts NOTIFY accountChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY accountChanged)
    Q_PROPERTY(bool verifying READ verifying NOTIFY accountChanged)
    Q_PROPERTY(QString accountError READ accountError NOTIFY accountChanged)
    Q_PROPERTY(int imageRevision READ imageRevision NOTIFY imagesChanged)
    Q_PROPERTY(int followUnread READ followUnread NOTIFY countersChanged)
    Q_PROPERTY(int notificationUnread READ notificationUnread NOTIFY countersChanged)
public:
    explicit Zhihu(QObject *parent = 0);
    virtual ~Zhihu();
    bool dark() const;
    bool largeText() const;
    int cacheLimitMb() const;
    void setCacheLimitMb(int mb);
    QString cacheUsageText() const;
    int cacheUsagePercent() const;
    Q_INVOKABLE void refreshCacheUsage();
    Q_INVOKABLE void clearCache();
    QVariantMap account() const;
    QVariantList accounts() const;
    bool loggedIn() const;
    bool verifying() const { return m_verifying; }
    QString accountError() const { return m_accountError; }
    int imageRevision() const { return m_imageRevision; }
    int followUnread() const { return m_followUnread; }
    int notificationUnread() const { return m_notificationUnread; }
    Q_INVOKABLE QObject *createFeed(const QString &kind, const QString &param, QObject *owner);
    Q_INVOKABLE void releaseFeed(QObject *object);
    Q_INVOKABLE bool saveSettings(bool dark, bool large);
    Q_INVOKABLE void verifyAccount(const QString &cookie);
    Q_INVOKABLE void clearAccountError();
    Q_INVOKABLE void verifyCookieValues(const QString &session, const QString &device);
    Q_INVOKABLE QVariantMap splitCookieValues(const QString &text) const;
    Q_INVOKABLE QObject *createPhoneLogin(QObject *owner);
    Q_INVOKABLE void switchAccount(const QString &id);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void removeAccount(const QString &id);
    Q_INVOKABLE QString clipboardText() const;
    Q_INVOKABLE bool copyAccountCredential(const QString &id);
    static QString normalizedCookie(const QString &value);
    Q_INVOKABLE QString image(const QString &url);
    Q_INVOKABLE QString readerBaseUrl() const;
    Q_INVOKABLE bool isSaved(const QVariantMap &item) const;
    Q_INVOKABLE void toggleSaved(const QVariantMap &item);
    Q_INVOKABLE void remember(const QVariantMap &item);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void openBrowser(const QString &url);
    Q_INVOKABLE bool openImage(const QString &url);
    Q_INVOKABLE bool openReaderImage(const QString &link);
    Q_INVOKABLE QVariantMap linkTarget(const QString &url) const;
    Q_INVOKABLE void refreshCounters();
    QString cookie() const;
    QString scope() const;
    QString storage(const QString &name) const;
    QVariant read(const QString &name) const;
    void write(const QString &name, const QVariant &data) const;
    QNetworkReply *get(const QString &url, const QString &tag, QObject *receiver,
                       const char *slot, const QString &credential = QString());
    QString transportUrl(const QString &url) const;
    QString error(QNetworkReply *reply) const;
    QString document(const QVariantMap &data, const QVariantMap &item);
    static QVariantMap normalize(const QVariantMap &raw);
    static QVariantMap normalizeComment(const QVariantMap &raw);
    static QString plain(const QString &html);
    static QString identity(const QVariantMap &row);
signals:
    void settingsChanged();
    void cacheUsageChanged();
    void accountChanged();
    void sessionChanged();
    void libraryChanged();
    void imagesChanged();
    void countersChanged();
    void toast(const QString &text);
private slots:
    void sslErrors(const QList<QSslError> &errors);
    void verifyFinished();
    void phoneLoginFinished(const QVariantMap &credentials);
    void imageFinished();
    void counterFinished();
    void pumpImages();
    void abortRequest();
    void imageInvokeFinished();
    void cleanupImagePreview();
private:
    QNetworkAccessManager m_network;
    bb::system::InvokeManager *m_invokeManager;
    QSettings m_settings;
    qint64 m_cacheBytes;
    QVariantList m_accounts;
    QString m_active;
    bool m_verifying;
    QString m_accountError;
    int m_imageRevision, m_followUnread, m_notificationUnread, m_epoch, m_imageActive;
    QStringList m_imageQueue;
    QSet<QString> m_imagePending, m_imageFailed;
    QString m_previewSource, m_sharedPreviewPath;
    bool m_previewActive;
    bool showImagePreview(const QString &localPath);
    void saveAccounts();
    void verifyCandidate(const QString &cookie, const QVariantMap &mobile);
    void changedSession();
    void applyCacheLimit();
    void pruneCache() const;
};
class Feed : public QObject {
    Q_OBJECT
    Q_PROPERTY(bb::cascades::DataModel *model READ model NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY changed)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)
    Q_PROPERTY(bool cached READ cached NOTIFY changed)
    Q_PROPERTY(QString html READ html NOTIFY changed)
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY changed)
    Q_PROPERTY(QString heading READ heading WRITE setHeading NOTIFY changed)
public:
    Feed(Zhihu *client, const QString &kind, const QString &param, QObject *parent);
    bb::cascades::DataModel *model() const {
        if (m_groupedModel) return m_groupedModel;
        return m_model;
    }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    int count() const;
    bool hasMore() const { return !m_next.isEmpty(); }
    bool loaded() const { return m_loaded; }
    bool cached() const { return m_cached; }
    QString html() const { return m_html; }
    QVariantMap profile() const { return m_profile; }
    QString heading() const { return m_heading; }
    void setHeading(const QString &heading);
    Q_INVOKABLE void load(bool more = false);
    Q_INVOKABLE void setItem(const QVariantMap &item);
signals:
    void changed();
private slots:
    void finished();
    void reset();
    void libraryChanged();
private:
    Zhihu *m_client;
    bb::cascades::ArrayDataModel *m_model;
    bb::cascades::GroupDataModel *m_groupedModel;
    QString m_kind, m_param, m_next, m_error, m_html;
    QString m_heading;
    QVariantMap m_item;
    QVariantMap m_profile;
    bool m_busy, m_loaded, m_cached;
    bool m_dailyShortComments, m_continueComments;
    int m_generation;
    QSet<QString> m_seen;
    QString initialUrl() const;
    QString cacheName() const;
    void replace(const QVariantList &rows);
    void consume(const QVariantMap &data, bool more);
};
#endif
