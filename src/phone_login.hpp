#ifndef PHONE_LOGIN_HPP
#define PHONE_LOGIN_HPP
#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantMap>
#include <QMap>
class QNetworkReply;
class PhoneLogin : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(bool sent READ sent NOTIFY changed)
    Q_PROPERTY(int cooldown READ cooldown NOTIFY changed)
    Q_PROPERTY(bool captchaRequired READ captchaRequired NOTIFY changed)
    Q_PROPERTY(QVariant captchaImage READ captchaImage NOTIFY changed)
public:
    explicit PhoneLogin(QObject *parent = 0);
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    bool sent() const { return m_sent; }
    int cooldown() const { return m_cooldown; }
    bool captchaRequired() const { return m_captchaRequired; }
    QVariant captchaImage() const { return m_captchaImage; }
    Q_INVOKABLE void requestDigits(const QString &phone);
    Q_INVOKABLE void signIn(const QString &phone, const QString &digits);
    Q_INVOKABLE void refreshCaptcha();
    Q_INVOKABLE void verifyCaptcha(const QString &input);
    static QByteArray encrypt(const QByteArray &form);
    static QByteArray hmacSha1(QByteArray key, const QByteArray &message);
    static QString normalizePhone(QString phone);
signals:
    void changed();
    void credentialsReady(const QVariantMap &credentials);
private slots:
    void finished();
    void timeout();
    void tick();
private:
    QNetworkAccessManager m_network;
    bool m_busy, m_sent, m_captchaRequired, m_rechecked;
    int m_cooldown;
    QString m_error, m_phone, m_digits, m_authorization, m_udid, m_webDevice, m_pending;
    QVariant m_captchaImage;
    QMap<QString, QString> m_cookies;
    void ensureGuest();
    void advance();
    void send(const QString &stage, const QByteArray &form = QByteArray());
    void fail(const QString &message);
};
#endif
