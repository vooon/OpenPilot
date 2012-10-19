#ifndef UPDATEGADGETWIDGET_H
#define UPDATEGADGETWIDGET_H

#include <QWidget>
#include "updategadget.h"
#include "updatelogic.h"

namespace Ui {
class UpdateGadgetWidget;
}

class UpdateGadgetWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit UpdateGadgetWidget(QWidget *parent = 0);
    ~UpdateGadgetWidget();
    
private:
    Ui::UpdateGadgetWidget *ui;
    UpdateLogic *updateLogic;

private slots:
    void pushButtonClick();
};

#endif // UPDATEGADGETWIDGET_H
