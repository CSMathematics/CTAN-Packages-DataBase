#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFileDialog>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    void getData();
    void createJsonDatabaseFile();

    int completedPackages = 0;
    int totalPackages;
    QNetworkAccessManager * manager;
    QStringList packagesIds;
    QList<QJsonObject> packages;
    QHash<QString,QJsonObject> authors;
    QHash<QString,QJsonObject> licenses;
    QHash<QString,QJsonObject> topics;
    QJsonArray packagesNewArray;
    QString saveLocation;

signals:
    int packageCount();
};
#endif // MAINWINDOW_H
