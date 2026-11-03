#include "platform/SingleInstance.h"

#include <QLocalSocket>

namespace {

constexpr int kTimeoutMs = 200;
constexpr char kShowCommand[] = "show\n";

}  // namespace

SingleInstance::SingleInstance(QObject* parent) : QObject(parent) {}

bool SingleInstance::claim(const QString& name) {
    QLocalSocket probe;
    probe.connectToServer(name);
    if (probe.waitForConnected(kTimeoutMs)) {
        probe.write(kShowCommand);
        probe.waitForBytesWritten(kTimeoutMs);
        return false;
    }

    QLocalServer::removeServer(name);
    if (!m_server.listen(name)) return true;
    connect(&m_server, &QLocalServer::newConnection, this, &SingleInstance::handleConnection);
    return true;
}

void SingleInstance::handleConnection() {
    QLocalSocket* socket = m_server.nextPendingConnection();
    if (!socket) return;

    connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    auto handle = [this, socket] {
        if (socket->readAll().contains("show")) emit showRequested();
    };
    connect(socket, &QLocalSocket::readyRead, this, handle);
    if (socket->bytesAvailable() > 0) handle();
}
