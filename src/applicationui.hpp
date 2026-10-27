#ifndef APPLICATIONUI_HPP
#define APPLICATIONUI_HPP
#include <QObject>
class ApplicationUI : public QObject {
    Q_OBJECT
public:
    explicit ApplicationUI(QObject *parent = 0);
};
#endif
