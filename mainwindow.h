#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSettings>
#include <QMainWindow>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QNetworkInterface>
#include <QStringList>
#include "linkthread.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent *);
    void timerEvent(QTimerEvent *);
private:
    //Load Network Links To UI ComboBox
    void loadNetLink();
    void createTray(); //create SystemTray
    //Settings Operation
    void loadDefaultSettings();
    void loadSettingsToUI();
    void loadSettings();
    void updateSettings();

    int showMessage(QString text,QString detail,
                    QMessageBox::StandardButton button1,
                    bool hasButton2=false,
                    QMessageBox::StandardButton button2=QMessageBox::Cancel);//Show buttons on the front
    //Runtime Settings
    int timeOnline;
    int timerID;

    //Settings Attributes
    QString account;
    QString password;
    QString linkType;       //"@internet" or "@edu"
    bool rememberPassword;
    bool autoHide;          //automatically hide window after authentication succeeded
    bool autoLink;          //automatically start authentication after the program is started
    int waitRespond;        //time to wait for respond from the server after sending packet
    int retryNum;           //times to retry if authentication failed
    int maxInterval;

    //Interface Settings
    int currentInterface;   //index for the current interface used in the interface list,Default is -1Not available
    QStringList ifName;     //Device name list
    QStringList description;//Device description list (Human Readable)
    QStringList hdAddress;  //Hardware Address list of Devices

    LinkThread *linkThread;
    //UI Elements
    bool msgInUse;
    QMessageBox *msgBox;
    Ui::MainWindow *ui;
    QSystemTrayIcon *systray;
private slots:
    void on_autoLink_toggled(bool checked);
    void on_whatsthis_clicked();
    void disconnectNet();
    void tellStatus(int id=0);//Tell status of the link by balloon

    void on_disconnect_clicked();
    void on_aboutAuthor_clicked();
    void on_about_clicked();
    void echoLinkThreadNotify(int id);
    void on_detectNetLink_clicked();
    void on_link_clicked();
    void on_password_editingFinished();
    void on_account_editingFinished();
    void on_exit_clicked();
    void on_cancel_clicked();
    void on_settings_clicked();
    void on_rememberPassword_toggled(bool checked);
    void on_linkType_currentIndexChanged(QString );
    void on_apply_clicked();
};

#endif // MAINWINDOW_H
