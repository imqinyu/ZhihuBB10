#ifndef AUTH_PROBE_HPP
#define AUTH_PROBE_HPP
#include <QObject>
#include <QVariantList>
class Zhihu;
class AuthProbe : public QObject {
    Q_OBJECT
public:
    explicit AuthProbe(QObject *parent = 0);
    Zhihu *client() const { return m_client; }
public slots:
    void start();
private slots:
    void finished();
    void accountChanged();
    void modelChanged();
    void startModels();
private:
    Zhihu *m_client;
    QString m_cookie;
    int m_pending;
    QVariantList m_results;
    bool m_modelsStarted;
    int m_modelPending;
};
#endif
