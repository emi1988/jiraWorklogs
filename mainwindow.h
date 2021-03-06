#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dialoguserdata.h"

#include <QMainWindow>
#include <QObject>
#include <QByteArray>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Ui {
class MainWindow;
}


struct stIssueData
{
    int id;
    QString key;
    QString summary;
    QString self;

};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QByteArray downloadedData() const;

    QList<stIssueData> m_issueData;

    void fetchIssueDataFromJira();
    void doUrlWebRequest(QString url);
    void parseJsonIssues(QString jasonData);
    void parseJsonWorklogs(QString jasonData);
    void readUserData();

    QNetworkAccessManager m_WebCtrl;
    QByteArray m_DownloadedData;
    DialogUserData *m_userDataDialog;
    QString m_userName, m_userPw;

    QString m_jsonData;
    int m_secondsOfDay;

    //fetchMode=1 -> fetch issues
    //fetchMode=2 -> fetch worklogs
    int m_fetchMode;

signals:
 void downloaded();

private slots:
 void fileDownloaded(QNetworkReply* pReply);

 void on_calendarWidget_selectionChanged();

 void on_action_about();
 void on_action_userData();

 void handleNewUserData(QString user, QString pw, bool save);

 void on_pushButtonRefetchData_clicked();

 void raiseTimerFinished();
};

#endif // MAINWINDOW_H
