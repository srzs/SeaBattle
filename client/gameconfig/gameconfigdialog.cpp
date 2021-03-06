#include <QMessageBox>
#include "gameconfigdialog.h"
#include "ui_gameconfigdialog.h"
#include "ui_gameconfigeditdialog.h"

GameConfigDialog::GameConfigDialog(QWidget *parent, GameConfigs &configs) :
    QDialog(parent),
    ui(new Ui::GameConfigDialog),
    model(this, configs)
{
    ui->setupUi(this);

    ui->listView->setModel(&model);

    connect(ui->listView->selectionModel(), &QItemSelectionModel::currentRowChanged, [&] (const QModelIndex &current) {
        const auto state = current.isValid();
        ui->pushButtonEdit->setEnabled(state);
        ui->pushButtonDelete->setEnabled(state);
    });

    connect(ui->pushButtonNew, &QPushButton::clicked, [&] () {
        GameConfig config{tr("Game")};
        if (GameConfigEditDialog{this, config}.exec()) {
            auto index = model.append(config);

            // Set as current item
            ui->listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
            ui->listView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        }
    });

    connect(ui->pushButtonEdit, &QPushButton::clicked, [&] () {
        for (const auto &index : ui->listView->selectionModel()->selectedRows()) {
            if (index.isValid()) {
                GameConfig config = model.get(index);
                if (GameConfigEditDialog{this, config}.exec()) {
                    model.update(index, config);
                }
            }
        }
    });

    connect(ui->pushButtonDelete, &QPushButton::clicked, [&] () {
        for (const auto &index : ui->listView->selectionModel()->selectedRows()) {
            if (index.isValid()) {
                if (QMessageBox::warning(this, tr("Confirm deletion"), tr("Do you really want to delete '%1'?").arg(index.data().toString()),
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    model.removeRow(index.row());
                }
            }
        }
    });

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

GameConfigDialog::~GameConfigDialog()
{
    delete ui;
}

GameConfigEditDialog::GameConfigEditDialog(QWidget *parent, GameConfig &config) :
    QDialog(parent),
    ui(new Ui::GameConfigEditDialog),
    config(config)
{
    ui->setupUi(this);

    // Set existing name
    ui->lineEditName->setText(config.name());
    // Set existing size
    ui->spinBoxWidth->setValue(config.size().x());
    ui->spinBoxHeight->setValue(config.size().y());

    // Set existing ships and model
    ui->tableViewShips->setModel(new GameConfigShipModel(this, config.ships()));

    // Make "name" extending column
    ui->tableViewShips->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableViewShips->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    validateInput();

    connect(ui->lineEditName, &QLineEdit::textChanged, this, &GameConfigEditDialog::validateInput);
    connect(ui->tableViewShips->model(), &QAbstractItemModel::dataChanged, this, &GameConfigEditDialog::validateInput);
    connect(ui->tableViewShips->model(), &QAbstractItemModel::rowsRemoved, this, &GameConfigEditDialog::validateInput);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void GameConfigEditDialog::validateInput()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                !ui->lineEditName->text().isEmpty() && !config.ships().empty());
}

void GameConfigEditDialog::accept()
{
    config.setName(ui->lineEditName->text());
    config.setSize({ui->spinBoxHeight->value(), ui->spinBoxWidth->value()});
    QDialog::accept();
}

GameConfigEditDialog::~GameConfigEditDialog()
{
    delete ui;
}
