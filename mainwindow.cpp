#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "simplecrypt.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_userName = "";
    m_userPw   = "";

    readUserData();

    //setup the ui
    //QDateTime systemTime = QDateTime::currentDateTime();

    connect(ui->actionAbout,SIGNAL (triggered()), this, SLOT(on_action_about()));
    connect(ui->actionUser_data,SIGNAL (triggered()), this, SLOT(on_action_userData()));

    m_userDataDialog = new DialogUserData();

    connect(m_userDataDialog, SIGNAL(signalSetUserData(QString,QString,bool)), this, SLOT(handleNewUserData(QString,QString,bool)));

    ui->tableWidget->clear();
    ui->tableWidget->setColumnCount(2);

    //set table headers
    ui->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("Project"));
    ui->tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("Time"));

    connect(&m_WebCtrl, SIGNAL (finished(QNetworkReply*)),this, SLOT (fileDownloaded(QNetworkReply*)));


    fetchDataFromJira();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fetchDataFromJira()
{
    //QUrl currentUrl("https://jira.ids-intranet.de/rest/api/2/search?fields=key,summary,worklog&jql=worklogDate >= '-24h' AND worklogAuthor in (currentUser())");
    QUrl currentUrl("https://jira.ids-intranet.de/rest/api/2/search?fields=key,summary,worklog&jql=worklogAuthor in (currentUser())");

    QNetworkRequest request(currentUrl);

    if(m_userName.isEmpty() | m_userPw.isEmpty())
    {
        m_userDataDialog->show();
        m_userDataDialog->raise();

        // raise comes to early so rais m_userDataDialog with a timer
        QTimer::singleShot(200, this, SLOT(raiseTimerFinished()));

        return;
    }

    // HTTP Basic authentication header value: base64(username:password)
    QString concatenated = m_userName + ":" + m_userPw;
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());


    QFile file("WorklogJson.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    m_jsonData = file.readAll();

    parseJson(m_jsonData);

    file.close();
    file.flush();

//DEBUG
  //  m_WebCtrl.get(request);
}

void MainWindow::parseJson(QString jasonData)
{
    if(m_userName.isEmpty() | m_userPw.isEmpty())
    {
        m_userDataDialog->show();
        m_userDataDialog->raise();

        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jasonData.toUtf8(), &err);
    qDebug() << err.errorString();

    if(err.errorString().compare("no error occurred") == 0)
    {
        ui->lineEditUserOutput->setText("succes");
    }
    else
    {
        ui->lineEditUserOutput->setText("error");
    }

    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);


    if(doc.isObject())
    {
        QJsonObject obj = doc.object();

        //qDebug() << obj.count();

        QJsonArray issuesArray = obj.value("issues").toArray();

        qDebug() <<"amount of issues: " << issuesArray.size();

        //loop through issues

        int secondsOfDay = 0;

        for (int currentIssueNumber = 0; currentIssueNumber < issuesArray.size(); ++currentIssueNumber) {

            QJsonObject currentIssueObj = issuesArray.at(currentIssueNumber).toObject();

            QString currentKey = currentIssueObj["key"].toString();

            qDebug() << "currentKey: " << currentKey;

            QJsonObject currentFieldsObj = currentIssueObj["fields"].toObject();

            QString currentSummary = currentFieldsObj["summary"].toString();

            qDebug() << "currentSummary" << currentSummary;

            QJsonObject currentWorklogObj = currentFieldsObj["worklog"].toObject();

            //qDebug() << currentWorklogObj.length();

            //TODO: noch testen ob max results überschritten wird

            qDebug() << "total: " << currentWorklogObj["total"].toInt();
            qDebug() << "maxResults: " << currentWorklogObj["maxResults"].toInt();

            QJsonArray currentWorklogsArray = currentWorklogObj["worklogs"].toArray();

            //loop through the worklogs,
            qDebug() <<"amount of worklogs: " << currentWorklogsArray.size();

            for (int currentWorklogNumber = 0; currentWorklogNumber < currentWorklogsArray.size(); ++currentWorklogNumber) {

                QJsonObject currentWorklogObj2 = currentWorklogsArray.at(currentWorklogNumber).toObject();

                QString worklogDateStr = currentWorklogObj2["created"].toString();
                int worklogtimeSpentSeconds = currentWorklogObj2["timeSpentSeconds"].toInt();
                QString worklogtimeSpentStr = currentWorklogObj2["timeSpent"].toString();

                QStringList splitedDate = worklogDateStr.split("-");

                int worklogYear = splitedDate.at(0).toInt();
                int worklogMonth = splitedDate.at(1).toInt();
                int worklogDay = splitedDate.at(2).left(2).toInt();

                qDebug() << "current worklog date:" << worklogDay << "." << worklogMonth << "." << worklogYear;

                QDate selectedDate = ui->calendarWidget->selectedDate();

                int selectedYear = selectedDate.year();
                int selectedMonth = selectedDate.month();
                int selectedday = selectedDate.day();

                if((worklogYear == selectedYear) & (worklogMonth == selectedMonth) & (worklogDay == selectedday))
                {
                    qDebug() << "current worklog is from today";
                    secondsOfDay = secondsOfDay + worklogtimeSpentSeconds;

                    ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);

                    QTableWidgetItem *newKeyItem = new QTableWidgetItem(currentKey + "(" + currentSummary + ")");
                    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0,newKeyItem);

                    QTableWidgetItem *newValueItem = new QTableWidgetItem(worklogtimeSpentStr);
                    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,1,newValueItem);
                }
            }
        }



        ui->lineEditTimeOfDay->setText(QString::number(secondsOfDay/3600, 'f', 0) + "h " + QString::number((secondsOfDay%3600)/60, 'f', 0) + "m");

        ui->tableWidget->resizeColumnsToContents();
        ui->tableWidget->resizeRowsToContents();
    }

}

void MainWindow::readUserData()
{
    QString filePath = QDir::currentPath() + "/userData.txt";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "not user data found";
        return;
    }

    SimpleCrypt crypto;

    crypto.setKey(Q_UINT64_C(0xafc6608b9e15f8cb));

    m_userName = crypto.decryptToString(QString(file.readLine()));
    m_userPw = crypto.decryptToString(QString(file.readLine()));


    file.close();
    file.flush();
}


void MainWindow::fileDownloaded(QNetworkReply* pReply) {

    m_DownloadedData = pReply->readAll();
    //emit a signal
    pReply->deleteLater();

    QString dataAsString = QString(m_DownloadedData);

    qDebug() << dataAsString;

    parseJson(dataAsString);
}

void MainWindow::on_calendarWidget_selectionChanged()
{
    //parseJson(m_jsonData);

    //DEBUG

    QFile file("WorklogJson.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    parseJson(QString(file.readAll()));

    file.close();
    file.flush();
}

void MainWindow::on_action_about()
{
    QMessageBox msgBox;
    msgBox.setText("Version 1.0 created by E. Halberstadt");
    msgBox.exec();

}

void MainWindow::on_action_userData()
{
    m_userDataDialog->show();
    m_userDataDialog->raise();
}

void MainWindow::handleNewUserData(QString user, QString pw, bool save)
{
    m_userName = user;
    m_userPw   = pw;

    QString filePath = QDir::currentPath() + "/userData.txt";

    QFile file(filePath);

    if(save == true)
    {
        if (file.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            SimpleCrypt crypto;

            crypto.setKey(Q_UINT64_C(0xafc6608b9e15f8cb));

            QTextStream out(&file);

            out << crypto.encryptToString(user) << "\n";
            out << crypto.encryptToString(pw) << "\n";

            out.flush();

            file.close();
            file.flush();
        }
    }
    else
    {
        //clear the file
        file.open(QIODevice::ReadWrite | QIODevice::Text| QIODevice::Truncate);

        file.close();
        file.flush();
    }


    //fetch the data from jira
}

void MainWindow::on_pushButtonRefetchData_clicked()
{
    fetchDataFromJira();
}

void MainWindow::raiseTimerFinished()
{
    m_userDataDialog->raise();
}
