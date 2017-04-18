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

    m_fetchMode = 1;

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


    fetchIssueDataFromJira();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fetchIssueDataFromJira()
{
    //QUrl currentUrl("https://jira.ids-intranet.de/rest/api/2/search?fields=key,summary,worklog&jql=worklogDate >= '-24h' AND worklogAuthor in (currentUser())");
    //QUrl currentUrl("https://jira.ids-intranet.de/rest/api/2/search?fields=key,summary,worklog&jql=worklogAuthor in (currentUser())");

    //block a new request if the old request hasn't finished
    if(m_fetchMode == 1)
    {
        QDate selectedDate = ui->calendarWidget->selectedDate();

        QString selectedYear = QString::number(selectedDate.year());
        QString selectedMonth = QString::number(selectedDate.month());
        QString selectedDay = QString::number(selectedDate.day());

        QString urlBuilder = "https://jira.ids-intranet.de/rest/api/2/search?fields=key,summary&jql=worklogDate =";

        urlBuilder.append("'" + selectedYear + "/" + selectedMonth + "/" + selectedDay + "'");

        urlBuilder.append(" AND worklogAuthor in (currentUser())");

#ifdef USE_FILE_INSTEAD_OF_WEBREQUEST
        QFile file("WorklogJson.txt");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        m_jsonData = file.readAll();

        parseJsonIssues(m_jsonData);

        file.close();
        file.flush();
#else
        doUrlWebRequest(urlBuilder);
#endif
    }
}

void MainWindow::doUrlWebRequest(QString url)
{
    QUrl currentUrl(url);

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

    m_WebCtrl.get(request);

}

void MainWindow::parseJsonIssues(QString jasonData)
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

        m_fetchMode = 2;

        m_secondsOfDay = 0;

        //loop through issues

        m_issueData.clear();

        for (int currentIssueNumber = 0; currentIssueNumber < issuesArray.size(); ++currentIssueNumber)
        {
            QJsonObject currentIssueObj = issuesArray.at(currentIssueNumber).toObject();

            QString currentKey = currentIssueObj["key"].toString();

            qDebug() << "currentKey: " << currentKey;

            QString currentId = currentIssueObj["id"].toString();

            qDebug() << "currentId: " << currentId;

            QString currentSelf = currentIssueObj["self"].toString();

            qDebug() << "currentSelf: " << currentSelf;

            QJsonObject currentFieldsObj = currentIssueObj["fields"].toObject();

            QString currentSummary = currentFieldsObj["summary"].toString();

            qDebug() << "currentSummary" << currentSummary;

            //save current issue data
            stIssueData currentIssueData;

            currentIssueData.id      = currentId.toInt();
            currentIssueData.key     = currentKey;
            currentIssueData.self    = currentSelf;
            currentIssueData.summary = currentSummary;

            m_issueData.append(currentIssueData);

            //generate a new request for every issue

            QString urlBuilder = currentSelf;

            urlBuilder.append("/worklog");

#ifdef USE_FILE_INSTEAD_OF_WEBREQUEST
            QFile file("WorklogOnly.txt");
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return;

            m_jsonData = file.readAll();

            parseJsonWorklogs(m_jsonData);

            file.close();
            file.flush();


#else
            doUrlWebRequest(urlBuilder);
#endif

        }

        //old with limited worklog

        //            QJsonObject currentWorklogObj = currentFieldsObj["worklog"].toObject();

        //            //qDebug() << currentWorklogObj.length();

        //            //TODO: noch testen ob max results überschritten wird

        //            qDebug() << "total: " << currentWorklogObj["total"].toInt();
        //            qDebug() << "maxResults: " << currentWorklogObj["maxResults"].toInt();

        //            QJsonArray currentWorklogsArray = currentWorklogObj["worklogs"].toArray();

        //            //loop through the worklogs,
        //            qDebug() <<"amount of worklogs: " << currentWorklogsArray.size();

        //            for (int currentWorklogNumber = 0; currentWorklogNumber < currentWorklogsArray.size(); ++currentWorklogNumber) {

        //                QJsonObject currentWorklogObj2 = currentWorklogsArray.at(currentWorklogNumber).toObject();

        //                QString worklogDateStr = currentWorklogObj2["created"].toString();
        //                int worklogtimeSpentSeconds = currentWorklogObj2["timeSpentSeconds"].toInt();
        //                QString worklogtimeSpentStr = currentWorklogObj2["timeSpent"].toString();

        //                QStringList splitedDate = worklogDateStr.split("-");

        //                int worklogYear = splitedDate.at(0).toInt();
        //                int worklogMonth = splitedDate.at(1).toInt();
        //                int worklogDay = splitedDate.at(2).left(2).toInt();

        //                qDebug() << "current worklog date:" << worklogDay << "." << worklogMonth << "." << worklogYear;

        //                QDate selectedDate = ui->calendarWidget->selectedDate();

        //                int selectedYear = selectedDate.year();
        //                int selectedMonth = selectedDate.month();
        //                int selectedday = selectedDate.day();

        //                //TODO: zusätzlich auf worklogAuthor prüfen

        //                if((worklogYear == selectedYear) & (worklogMonth == selectedMonth) & (worklogDay == selectedday))
        //                {
        //                    qDebug() << "current worklog is from today";
        //                    secondsOfDay = secondsOfDay + worklogtimeSpentSeconds;

        //                    ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);

        //                    QTableWidgetItem *newKeyItem = new QTableWidgetItem(currentKey + "(" + currentSummary + ")");
        //                    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0,newKeyItem);

        //                    QTableWidgetItem *newValueItem = new QTableWidgetItem(worklogtimeSpentStr);
        //                    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,1,newValueItem);
        //                }
        //            }

    }
}

void MainWindow::parseJsonWorklogs(QString jasonData)
{
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

        QJsonArray worklogsArray = obj.value("worklogs").toArray();

        qDebug() <<"amount of worklogs for current issue: " << worklogsArray.size();

        int issueIdFromWorklog;

        //loop through all worklogs for current issue
        for (int currentWorklogNumber = 0; currentWorklogNumber < worklogsArray.size(); ++currentWorklogNumber)
        {
            QJsonObject currentWorklogObj = worklogsArray.at(currentWorklogNumber).toObject();

            issueIdFromWorklog = currentWorklogObj["issueId"].toString().toInt();

            QJsonObject currentAuthorObj = currentWorklogObj["author"].toObject();

            QString currentAuthor = currentAuthorObj["name"].toString();

            //only take worklogs from the current user
            if(currentAuthor.compare(m_userName) == 0)
            {
                QString worklogDateStr = currentWorklogObj["updated"].toString();
                int worklogtimeSpentSeconds = currentWorklogObj["timeSpentSeconds"].toInt();
                QString worklogtimeSpentStr = currentWorklogObj["timeSpent"].toString();

                QStringList splitedDate = worklogDateStr.split("-");

                int worklogYear = splitedDate.at(0).toInt();
                int worklogMonth = splitedDate.at(1).toInt();
                int worklogDay = splitedDate.at(2).left(2).toInt();

                qDebug() << "current worklog date:" << worklogDay << "." << worklogMonth << "." << worklogYear;

                QDate selectedDate = ui->calendarWidget->selectedDate();

                int selectedYear = selectedDate.year();
                int selectedMonth = selectedDate.month();
                int selectedday = selectedDate.day();

                //only take worklogs for the selected day
                if((worklogYear == selectedYear) & (worklogMonth == selectedMonth) & (worklogDay == selectedday))
                {
                    qDebug() << "current worklog is from today";
                    m_secondsOfDay = m_secondsOfDay + worklogtimeSpentSeconds;

                    //look-up the issue information
                    QString currentKey, currentSummary;
                    for (int i = 0; i < m_issueData.length(); ++i)
                    {
                        stIssueData currentIssueData = m_issueData.at(i);

                        if(currentIssueData.id == issueIdFromWorklog)
                        {
                            currentKey = currentIssueData.key;
                            currentSummary = currentIssueData.summary;

                            break;
                        }
                    }


                    ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);

                    QTableWidgetItem *newKeyItem = new QTableWidgetItem(currentKey + "(" + currentSummary + ")");
                    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0,newKeyItem);

                    QTableWidgetItem *newValueItem = new QTableWidgetItem(worklogtimeSpentStr);
                    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,1,newValueItem);
                }
            }
        }

            ui->lineEditTimeOfDay->setText(QString::number(m_secondsOfDay/3600, 'f', 0) + "h " + QString::number((m_secondsOfDay%3600)/60, 'f', 0) + "m");

            qDebug() <<"secondsOfDay converted:" << QString::number(m_secondsOfDay/3600, 'f', 0) + "h "+ QString::number((m_secondsOfDay%3600)/60, 'f', 0) + "m";

            ui->tableWidget->resizeColumnsToContents();
            ui->tableWidget->resizeRowsToContents();

            //remove the issue from the list
            for (int i = 0; i < m_issueData.length(); ++i)
            {
                stIssueData currentIssueData = m_issueData.at(i);

                if(currentIssueData.id == issueIdFromWorklog)
                {
                   m_issueData.removeAt(i);

                   //check if the list is empty
                    if(m_issueData.isEmpty() == true)
                    {
                        m_fetchMode = 1;
                    }

                   break;
                }

            }



            //if all issues are checked, change the fetchMode so a new date can be queried


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

    pReply->deleteLater();

    QString dataAsString = QString(m_DownloadedData);

    qDebug() << dataAsString;

    if(m_fetchMode == 1)
    {
        parseJsonIssues(dataAsString);
    }
    else
    {
        parseJsonWorklogs(dataAsString);
    }
}

void MainWindow::on_calendarWidget_selectionChanged()
{

#ifdef USE_FILE_INSTEAD_OF_WEBREQUEST

    QFile file("WorklogJson.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    parseJsonIssues(QString(file.readAll()));

    file.close();
    file.flush();

#else
    fetchIssueDataFromJira();
#endif

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
    fetchIssueDataFromJira();
}

void MainWindow::raiseTimerFinished()
{
    m_userDataDialog->raise();
}
