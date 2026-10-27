#ifndef UI_CHECK_HPP
#define UI_CHECK_HPP
#include <QObject>
#include <bb/cascades/ArrayDataModel>
class Fixture : public QObject {
    Q_OBJECT
    Q_PROPERTY(bb::cascades::ArrayDataModel *model READ model CONSTANT)
    Q_PROPERTY(int count READ count CONSTANT)
    Q_PROPERTY(bool busy READ no CONSTANT)
    Q_PROPERTY(bool cached READ no CONSTANT)
    Q_PROPERTY(bool hasMore READ no CONSTANT)
    Q_PROPERTY(bool loaded READ yes CONSTANT)
    Q_PROPERTY(QString error READ error CONSTANT)
public:
    explicit Fixture(QObject *parent = 0, bool question = false, bool comments = false);
    bb::cascades::ArrayDataModel *model() const { return m_model; }
    int count() const { return m_model->size(); }
    bool no() const { return false; }
    bool yes() const { return true; }
    QString error() const { return QString(); }
private:
    bb::cascades::ArrayDataModel *m_model;
};
class Check : public QObject {
    Q_OBJECT
public:
    Check(QObject *scene, QObject *parent = 0);
private slots:
    void poll();
    void toast(const QString &code);
    void commentProbeFinished();
private:
    QObject *m_scene;
    int m_settingsToasts;
};
#endif
