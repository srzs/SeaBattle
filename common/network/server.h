#pragma once

#include <vector>
#include <QWebSocketServer>
#include <QUuid>
#include <QHash>
#include "client.h"
#include "player.h"
#include "game.h"

namespace SeaBattle {
namespace Network {

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent);
    ~Server();

    bool start(unsigned int port = 43560);

    QUrl url() const;
    unsigned int port() const;

signals:
    void closed();

private slots:
    void accept();

private:
    void sendGameCreated(Client* client, Game* game, const QUuid &id);

    QWebSocketServer *socket;
    QHash<QUuid, Client*> clients;
    QHash<QUuid, Game*> games;
};

}
}
