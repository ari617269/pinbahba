#pragma once

#include <QLocalServer>
#include <QObject>
#include <QString>

class SingleInstance : public QObject {
    Q_OBJECT

public:
    explicit SingleInstance(QObject* parent = nullptr);

    bool claim(const QString& name);

signals:
    void showRequested();

private:
    void handleConnection();

    QLocalServer m_server;
};
