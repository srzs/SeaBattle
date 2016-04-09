#pragma once

#include <QMainWindow>
#include "gameconfig.h"
#include "network/server.h"
#include "network/client.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent, const QString &configPath);
    ~MainWindow();

    void createGame(const SeaBattle::GameConfig &config);
    SeaBattle::Network::Client* joinGame(const QUrl &url);

    bool loadConfig();
    bool saveConfig();

private:
    Ui::MainWindow *ui;

    const QString configPath;
    SeaBattle::GameConfigs configs;

    SeaBattle::Network::Server *server;
    std::vector<SeaBattle::Network::Client*> clients;
};