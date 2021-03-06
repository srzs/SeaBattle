#pragma once

#include <unordered_set>
#include <QUuid>
#include "gameconfig.h"
#include "field.h"

namespace SeaBattle {
class ServerGame;
namespace Network {
class ServerClient;
}

struct Player : public QObject
{
    Player(ServerGame *game, bool first, const GameConfig &config);

    ServerGame *game() const;
    const QUuid &id() const;

    bool isValid() const;
    Player *opponent() const;

    Network::ServerClient* client() const;
    void setClient(Network::ServerClient* client);

    const std::unordered_set<const Ship*> &ships() const;
    bool hasShips() const;
    void setShips(const std::unordered_set<const Ship*> &ships);

    const Ship* shoot(const Coordinate &target);
    bool isSunken(const Ship* ship);

    bool isAttackFinished() const;
    const std::vector<Coordinate> &attackedFields() const;
    void resetTargets();

private:
    ServerGame *game_;
    const bool first;
    const QUuid id_;

    Network::ServerClient* client_;
    std::unordered_set<const Ship*> ships_;
    Sea sea;

    bool attackFinished;
    std::vector<Coordinate> attacked;
};

}
