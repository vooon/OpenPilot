#include "uavsettingsmanagerdialog.h"
#include "ui_uavsettingsmanagerdialog.h"

UAVSettingsManagerDialog::UAVSettingsManagerDialog( UAVSettingsManagerGadgetConfiguration *config, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UAVSettingsManagerDialog)
{
    ui->setupUi(this);
    ui->widget->loadConfiguration(config);
    setWindowTitle(tr("Import Export Settings"));

    connect( ui->widget, SIGNAL(done()), this, SLOT(close()));
}

UAVSettingsManagerDialog::~UAVSettingsManagerDialog()
{
    delete ui;
}

void UAVSettingsManagerDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
