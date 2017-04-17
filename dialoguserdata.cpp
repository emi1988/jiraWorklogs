#include "dialoguserdata.h"
#include "ui_dialoguserdata.h"

DialogUserData::DialogUserData(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogUserData)
{
    ui->setupUi(this);
}

DialogUserData::~DialogUserData()
{
    delete ui;
}

void DialogUserData::on_buttonBox_accepted()
{
    emit signalSetUserData(ui->lineEditUser->text(), ui->lineEditPw->text(), ui->checkBoxSaveUserData->isChecked());
}
