#ifndef ZHIHU_DAILY_REQUEST_DEADLINE_HPP
#define ZHIHU_DAILY_REQUEST_DEADLINE_HPP
#include <QObject>
#include <QNetworkReply>
#include <QTimer>
class RequestDeadline : public QObject {
    Q_OBJECT
public:
    explicit RequestDeadline(QNetworkReply *reply) : QObject(reply), m_reply(reply) {
        QTimer *timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, SIGNAL(timeout()), this, SLOT(expire()));
        connect(reply, SIGNAL(finished()), timer, SLOT(stop()));
        timer->start(20000);
    }
private slots:
    void expire() { if (m_reply->isRunning()) m_reply->abort(); }
private:
    QNetworkReply *m_reply;
};
#endif
