#ifndef DIALOGUSERDATA_H
#define DIALOGUSERDATA_H

#include <QDialog>

namespace Ui {
class DialogUserData;
}

class DialogUserData : public QDialog
{
    Q_OBJECT

public:
    explicit DialogUserData(QWidget *parent = 0);
    ~DialogUserData();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::DialogUserData *ui;

signals:
    void signalSetUserData(QString user, QString pw, bool save);
};

#endif // DIALOGUSERDATA_H
