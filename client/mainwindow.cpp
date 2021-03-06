#include <QFile>
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "gameconfig/gameconfigdialog.h"
#include "game/gamecreatedialog.h"
#include "game/gamejoindialog.h"
#include "game/gamewidget.h"

MainWindow::MainWindow(QWidget *parent, const QString &configPath) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    configPath(configPath),
    server(nullptr),
    broadcaster(nullptr)
{
    ui->setupUi(this);
    if (loadConfig()) {
        qDebug() << "Successfully loaded configuration from" << configPath;
        ui->statusBar->showMessage(tr("Configuration successfully loaded"), 3000);
    }

    connect(ui->actionNewGame, &QAction::triggered, [this] () {
        GameCreateDialog dialog{this, configs};
        if (!dialog.exec()) {
            return;
        }

        auto index = dialog.selection();
        if (index == -1) {
            // Create new config
            GameConfig config{tr("Game")};
            if (GameConfigEditDialog{this, config}.exec()) {
                index = configs.size();
                configs.push_back(config);
                saveConfig();
            } else {
                return;
            }
        }

        const GameConfig &config = configs[index];
        qInfo() << "Creating game:" << config.name();

        auto server = dialog.server();
        QUrl url;
        if (server.isEmpty()) {
            url = startServer();
        } else {
            url = server;
            if (!url.isValid()) {
                return; // TODO
            }
        }

        qDebug() << "Creating game on" << url;
        setupConnection(new GameClient{this, url, config}, dialog.isPublic());
    });

    connect(ui->actionJoinGame, &QAction::triggered, [this] () {
        GameJoinDialog dialog{this};
        if (!dialog.exec()) {
            return;
        }

        auto url = dialog.address();
        if (!url.isValid() || !url.hasQuery()) {
            return; // TODO
        }

        QUuid id{url.query()};
        if (id.isNull()) {
            return; // TODO
        }

        qDebug() << "Connecting to game" << url;
        setupConnection(new GameClient{this, url}, false);
    });

    connect(ui->actionGames, &QAction::triggered, [this] () {
        if (GameConfigDialog{this, configs}.exec()) {
            if (saveConfig()) {
                qDebug() << "Successfully saved configuration to" << this->configPath;
                ui->statusBar->showMessage(tr("Configuration successfully saved"), 3000);
            }
        }
    });

    connect(ui->actionAboutQt, &QAction::triggered, &QApplication::aboutQt);
    connect(ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);

    connect(ui->tabWidgetGames, &QTabWidget::tabCloseRequested, [this] (auto i) {
        auto widget = static_cast<GameWidget*>(ui->tabWidgetGames->widget(i));
        auto text = widget->name();
        if (QMessageBox::question(this, tr("Close game '%1'").arg(text),
                              tr("Do you really want to close the game '%1'?").arg(text)) == QMessageBox::Yes) {
            ui->tabWidgetGames->removeTab(i);
            delete widget;
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

QUrl MainWindow::startServer()
{
    if (server == nullptr) {
        // TODO: Detect IP address
        /*for (const auto &address : QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress{QHostAddress::LocalHost}) {
                host = address.toString();
                qDebug() << "Found IP address" << address;
                break;
            }
        }*/

        server = new Network::Server{this, "", false};
    }

    if (!server->isStarted()) {
        qDebug("Starting integrated server...");
        if (!server->start()) {
            qWarning("Retrying to start server on random port");
            server->start(0);
        }
    }

    return server->url();
}

void MainWindow::setupConnection(GameClient *client, bool publicGame)
{
    ui->tabWidgetGames->setCurrentIndex(ui->tabWidgetGames->addTab(new GameWidget{this, client}, tr("Connecting...")));

    connect(client, &GameClient::invitePlayer, [this, client, publicGame] (auto name, auto url) {
        this->replaceTab(new GameConnectWidget{ui->tabWidgetGames, client, url.toString()}, name);

        if (publicGame) {
            auto id = client->id();
            if (id.isNull()) {
                return;
            }

            if (broadcaster == nullptr) {
                broadcaster = new GameBroadcaster{this};
            }

            broadcaster->add(id, url);
        }
    });

    connect(client, &GameClient::prepare, [this, client] () {
        if (broadcaster != nullptr) {
            broadcaster->remove(client->id());
        }

        this->replaceTab(new GamePrepareWidget{ui->tabWidgetGames, client}, client->game().config().name());
    });

    connect(client, &GameClient::start, [this, client] (auto ships) {
        this->replaceTab(new GameMainWidget{ui->tabWidgetGames, client, ships}, client->game().config().name());
    });
}

bool MainWindow::loadConfig()
{
    QFile file{configPath};
    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open configuration file" << configPath << file.errorString();
        QMessageBox::critical(this, tr("Failed to open configuration file"),
                              tr("Failed to open configuration file: %1").arg(configPath));
        return false;
    }

    auto doc = QJsonDocument::fromJson(file.readAll());
    auto json = doc.object();

    QJsonArray configs = json["configs"].toArray();
    this->configs.clear();
    for (const auto &config : configs) {
        this->configs.emplace_back(config);
    }
    return true;
}

bool MainWindow::saveConfig()
{
    QFile file{configPath};
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Failed to open configuration file" << configPath << file.errorString();
        QMessageBox::critical(this, tr("Failed to open configuration file"),
                              tr("Failed to open configuration file: %1").arg(configPath));
        return false;
    }

    QJsonObject json;

    QJsonArray configs;
    for (const auto &config : this->configs) {
        configs.append(config);
    }
    json["configs"] = configs;

    QJsonDocument doc{json};
    file.write(doc.toJson());
    return true;
}

void MainWindow::replaceTab(GameWidget *tab, QString name)
{
    auto old = tab->replace();
    auto i = ui->tabWidgetGames->indexOf(old);
    auto selected = ui->tabWidgetGames->currentIndex() == i;

    ui->tabWidgetGames->removeTab(i);
    delete old;

    ui->tabWidgetGames->insertTab(i, tab, name);
    if (selected) {
        ui->tabWidgetGames->setCurrentIndex(i);
    }
}
