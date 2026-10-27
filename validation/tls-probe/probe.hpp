#ifndef TLS_PROBE_HPP
#define TLS_PROBE_HPP
#include <QObject>
#include <QNetworkAccessManager>
#include <QSslError>
class Probe : public QObject {
    Q_OBJECT
public:
    explicit Probe(QObject *parent = 0);
public slots:
    void start();
private slots:
    void finished();
    void sslErrors(const QList<QSslError> &errors);
    void timeout();
private:
    QNetworkAccessManager m_network;
    int m_pending;
};
#endif
