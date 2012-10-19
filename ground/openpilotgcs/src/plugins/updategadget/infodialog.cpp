#include "infodialog.h"
#include "ui_infodialog.h"

InfoDialog::InfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InfoDialog)
{
    ui->setupUi(this);

    ui->mainLabel->setOpenExternalLinks(true);

}

InfoDialog::~InfoDialog()
{
    delete ui;
}

void InfoDialog::SetMainLabelText(QString text)
{
    ui->mainLabel->setText(text);
}
