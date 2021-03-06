#include "server.h"

#include <QWebSocket>
#include <QDebug>
#include <QNetworkInterface>

namespace SeaBattle {
namespace Network {

Server::Server(QObject *parent, const QString &host, bool allowExternalGames) :
    QObject(parent),
    socket(new QWebSocketServer(QStringLiteral("SeaBattle"), QWebSocketServer::NonSecureMode, this)),
    host(host),
    allowExternalGames(allowExternalGames)
{
}

bool Server::start(unsigned int port)
{
    if (socket->listen(QHostAddress::Any, port)) {
        connect(socket, &QWebSocketServer::newConnection, this, &Server::accept);
        connect(socket, &QWebSocketServer::closed, this, &Server::closed);

        qInfo() << "Started SeaBattle server on:" << socket->serverPort();
        return true;
    } else {
        qCritical() << "Failed to bind server:" << socket->errorString();
        return false;
    }
}

bool Server::isStarted() const
{
    return socket->isListening();
}

QUrl Server::url() const
{
    return socket->serverUrl();
}

QUrl Server::externalUrl() const
{
    if (host.isEmpty()) {
        return socket->serverUrl();
    }

    auto url = socket->serverUrl();
    url.setHost(host);
    return url;
}

unsigned int Server::port() const
{
    return socket->serverPort();
}

void Server::accept()
{
    auto client = new ServerClient{this, socket->nextPendingConnection()};
    qDebug() << "Client accepted" << client->url();

    connect(client, &Client::disconnected, [client] () {
        qDebug() << "Client disconnected";
        client->deleteLater();
    });

    connect(client, &ServerClient::createGame, [this, client] (auto config) {
        if (!allowExternalGames && !client->address().isLoopback()) {
            client->disconnect("Server does not accept external games");
            return;
        }

        auto game = new ServerGame{config, this};
        auto id = QUuid::createUuid();
        games[id] = game;

        game->player(0)->setClient(client);
        client->sendGameCreated(this->externalUrl(), id);
    });

    connect(client, &ServerClient::joinGame, [this, client] (auto id) {
        auto game = games[id];
        if (!game) {
            auto player = players[id];
            if (!player) {
                client->disconnect("Unknown game");
                return;
            }

            auto oldClient = player->client();
            if (oldClient) {
                oldClient->disconnect("Logged in from another client");
            }

            player->setClient(client);
            client->sendStartGame();

            // TODO: Send complete state
            switch (player->game()->state()) {
            case Game::State::Preparing:
                client->sendSetShips();
                break;
            case Game::State::Playing:
                client->sendShips();
                break;
            default:
                break;
            }
            return;
        }

        if (game->state() != Game::State::Connecting) {
            client->disconnect("Invalid game state");
            return;
        }

        Player *player1 = game->player(0);
        if (!player1->isValid()) {
            player1->setClient(client);
            client->sendGameCreated(socket->serverUrl(), id);
        } else {
            Player *player2 = game->player(1);
            if (player2->isValid()) {
                client->disconnect("Game already full");
                return;
            }

            player2->setClient(client);

            qDebug() << "Starting game:" << game->config().name();
            games.remove(id);

            players[player1->id()] = player1;
            players[player2->id()] = player2;

            game->sendStartGame();
            game->setState(Game::State::Preparing);
        }
    });

    connect(client, &ServerClient::shipsSet, [this, client] (auto ships) {
        auto player = client->player();
        player->setShips(ships);

        if (player->opponent()->hasShips()) {
            player->game()->setState(Game::State::Playing);
        }
    });

    connect(client, &ServerClient::shoot, [this, client] (auto target) {
        auto player = client->player()->opponent();
        auto ship = player->shoot(target);

        if (ship) {
            auto sunken = player->isSunken(ship);
            client->sendShootResult(target, true, sunken, !player->isAttackFinished());
        } else {
            client->sendShootResult(target, false, false, false);
        }

        if (player->isAttackFinished() && client->player()->isAttackFinished()) {
            if (!client->player()->hasShips() || !player->hasShips()) {
                player->game()->setState(Game::State::Finished);

                players.remove(player->id());
                players.remove(client->player()->id());
                player->game()->deleteLater();
            } else {
                player->game()->sendContinue();
            }
        }
    });

    client->initialize();
}

}
}
