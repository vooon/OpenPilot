#include "updategadgetwidget.h"
#include "updategadget.h"
#include "ui_updategadgetwidget.h"
#include <QDebug>
#include "downloadpage.h"

UpdateGadgetWidget::UpdateGadgetWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UpdateGadgetWidget)
{
    setMinimumSize(64,64);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    updateLogic = new UpdateLogic();

    ui->setupUi(this);
    //    ui->pushButton->setText("Say hello!");

    connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(pushButtonClick()));
}

UpdateGadgetWidget::~UpdateGadgetWidget()
{
    delete ui;
}

void UpdateGadgetWidget::pushButtonClick(){
    qDebug()<<"CLICKED!";
    updateLogic->checkForNewGCS();
}
