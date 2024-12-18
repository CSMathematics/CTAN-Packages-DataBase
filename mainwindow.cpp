#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->openButton,&QPushButton::clicked,this,[=](){
        saveLocation = QFileDialog::getExistingDirectory(this, tr("Select Directory"), QDir::homePath(),
                                                         QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!saveLocation.isEmpty()) {
            ui->getDataButton->setEnabled(true);
            ui->progressBar->setEnabled(true);
            ui->stopNetworkAcces->setEnabled(true);
            ui->createJsonFileButton->setEnabled(true);
            ui->folderLocationLine->setText(saveLocation);
        } else {
            QMessageBox::warning(this, tr("No Location Selected"), tr("No location selected for saving."));
        }
    });
    // ui->progressBar->setStyleSheet("QProgressBar { height: 30px; }");

    //Load package database
    QNetworkAccessManager * packageManager = new QNetworkAccessManager(this);
    connect(packageManager,&QNetworkAccessManager::finished,this, [=](QNetworkReply * reply){
        QByteArray data = reply->readAll();
        QJsonDocument jsonPackages = QJsonDocument::fromJson(data);
        QJsonArray jsonArray = jsonPackages.array();
        for (const QJsonValue &value : jsonArray) {
            if (value.isObject()) {
                QJsonObject object = value.toObject();
                QString key = object["key"].toString();
                packagesIds.append(key);
            }
        }
    });

    //Load authors database
    QNetworkAccessManager * authorsManager = new QNetworkAccessManager(this);
    connect(authorsManager,&QNetworkAccessManager::finished,this, [=](QNetworkReply * reply){
        QByteArray data = reply->readAll();
        QJsonDocument jsonAuthors = QJsonDocument::fromJson(data);
        QJsonArray jsonArray = jsonAuthors.array();
        for (const QJsonValue &value : jsonArray) {
            if (value.isObject()) {
                QJsonObject object = value.toObject();
                QString author = object["key"].toString();
                authors.insert(author,object);
            }
        }
    });

    //Load topics database
    QNetworkAccessManager * topicsManager = new QNetworkAccessManager(this);
    connect(topicsManager,&QNetworkAccessManager::finished,this, [=](QNetworkReply * reply){
        QByteArray data = reply->readAll();
        QJsonDocument jsonTopics = QJsonDocument::fromJson(data);
        QJsonArray jsonArray = jsonTopics.array();
        for (const QJsonValue &value : jsonArray) {
            if (value.isObject()) {
                QJsonObject object = value.toObject();
                QString topic = object["key"].toString();
                topics.insert(topic,object);
            }
        }
    });

    //Load licenses database
    QNetworkAccessManager * licensesManager = new QNetworkAccessManager(this);
    connect(licensesManager,&QNetworkAccessManager::finished,this, [=](QNetworkReply * reply){
        QByteArray data = reply->readAll();
        QJsonDocument jsonLicenses = QJsonDocument::fromJson(data);
        QJsonArray jsonArray = jsonLicenses.array();
        for (const QJsonValue &value : jsonArray) {
            if (value.isObject()) {
                QJsonObject object = value.toObject();
                QString license = object["key"].toString();
                licenses.insert(license,object);
            }
        }
    });

    // packagesIds = {"a4wide","aaai","aalok"};
    totalPackages = packagesIds.count();
    connect(this,&MainWindow::packageCount,this,[&](){
        completedPackages++;
        ui->progressBar->setValue(completedPackages);
        ui->statusLabel->setText(QString::number(completedPackages)+" out of "+QString::number(totalPackages)+" packages completed.");
        ui->createJsonFileButton->setEnabled(completedPackages == totalPackages);
    });

    QUrl packageUrl("https://ctan.org/json/2.0/packages");
    packageManager->get(QNetworkRequest(packageUrl));

    QUrl authorsUrl("https://ctan.org/json/2.0/authors");
    authorsManager->get(QNetworkRequest(authorsUrl));

    QUrl topicsUrl("https://ctan.org/json/2.0/topics");
    topicsManager->get(QNetworkRequest(topicsUrl));

    QUrl licensesUrl("https://ctan.org/json/2.0/licenses");
    licensesManager->get(QNetworkRequest(licensesUrl));

    connect(ui->getDataButton,&QPushButton::clicked,this,&MainWindow::getData);
    connect(ui->createJsonFileButton,&QPushButton::clicked,this,&MainWindow::createJsonDatabaseFile);
    connect(ui->stopNetworkAcces, &QPushButton::clicked, [=]() {
        foreach (auto &reply, manager->findChildren<QNetworkReply *>()) {
            reply->abort();
        }
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getData()
{
    manager = new QNetworkAccessManager(this);
    totalPackages = packagesIds.count();
    ui->progressBar->setMaximum(totalPackages);
    connect(manager,&QNetworkAccessManager::finished,this, [=](QNetworkReply * reply){
        QByteArray data = reply->readAll();
        QString str(data);
        QJsonDocument packageDoc = QJsonDocument::fromJson(data);
        QJsonObject object = packageDoc.object();
        packages.append(object);
        emit packageCount();
    });
    for (QString package : packagesIds) {
        QUrl Url("https://www.ctan.org/json/2.0/pkg/"+package);
        manager->get(QNetworkRequest(Url));
    }
}

void MainWindow::createJsonDatabaseFile()
{
    for(QJsonObject package : packages){
        QJsonArray packageAuthors = package["authors"].toArray();
        QJsonArray newAuthorsArray;
        for(QJsonValue value : packageAuthors){
            QJsonObject authorObject = value.toObject();
            QString authorId = authorObject["id"].toString();
            authorObject = authors.value(authorId);
            newAuthorsArray.append(authorObject);
        }
        package.insert("authors",newAuthorsArray);

        QJsonArray packageTopics = package["topics"].toArray();
        int i=0;
        for(QJsonValue value : packageTopics){
            QString topicKey = value.toString();
            QJsonObject topicObject;
            topicObject = topics.value(topicKey);
            packageTopics.replace(i,topicObject);
            i++;
        }
        package.insert("topics",packageTopics);


        if(package["license"].isArray()){
            QJsonArray packageLicenses = package["license"].toArray();
            int k=0;
            for(QJsonValue value : packageLicenses){
                QString licenseKey = value.toString();
                QJsonObject licenseObject;
                licenseObject = licenses.value(licenseKey);
                packageLicenses.replace(k,licenseObject);
                k++;
            }
            package.insert("license",packageLicenses);
        }
        else{
            QString licenseKey = package["license"].toString();
            QJsonObject licenseObject;
            licenseObject = licenses.value(licenseKey);
            package.insert("license",licenseObject);
        }
        qDebug()<<package;
        packagesNewArray.append(package);
    }


    QFile file(saveLocation+QDir::separator()+"packageDatabase.json");
    file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
    // QString text = "["+packagesContents.join(",")+"]";
    QJsonDocument Document(packagesNewArray);
    file.resize(0);
    file.write(Document.toJson(QJsonDocument::Indented));
    file.close();
}

