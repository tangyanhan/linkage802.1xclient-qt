#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "global.h"
#include <QCloseEvent>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QTimerEvent>
#include <QMenu>
#include <QTextCodec>

#define HALF_WIDTH  330
#define FULL_WIDTH  670
#define HEIGHT 290



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    msgBox=NULL;
    msgInUse=false;
    timerID=0;

    createTray();

    linkThread=new LinkThread;
    connect(linkThread,SIGNAL(notify(int)),this,SLOT(echoLinkThreadNotify(int)));
    loadSettings();
    this->resize(HALF_WIDTH,HEIGHT);
    if(autoLink){
        on_link_clicked();
    }
}

MainWindow::~MainWindow()
{
    delete linkThread;
    delete ui;
}

/* 每分钟进行一次更新，更新在线时间 */
void MainWindow::timerEvent(QTimerEvent *event)
{
    if(event->timerId()==timerID){
        ++timeOnline;
        systray->setToolTip(tr("您已在线： %1 小时 %2分钟").arg(timeOnline/60).arg(timeOnline%60));
    }
}

void MainWindow::tellStatus(int id)
{
    assert(systray);

    switch(id){
        case STAT_SEARCH_SERVER:{
            systray->showMessage(
                            tr("发起认证"),tr("正在寻找服务器，请稍候……"),
                            QSystemTrayIcon::Information,1000);
            }
            break;
        case STAT_SENDING_ACCOUNT:{
            systray->showMessage(
                            tr("验证身份"),tr("正在验证用户名密码，请稍候……"),
                            QSystemTrayIcon::Information,1000);
            }
            break;
        case STAT_AUTHEN_SUCCESS:{
            systray->showMessage(
                            tr("认证成功"),tr("认证成功，正在配置网络环境，请稍候……"),
                            QSystemTrayIcon::Information,1000);
            systray->setIcon(QIcon(":/images/trayIcon-A.png"));
            }
            break;
        case STAT_RELEASE_IP:{
            systray->showMessage(
                    tr("释放连接"),tr("正在释放IP地址，请稍候……"),
                    QSystemTrayIcon::Information,1000);
            }
            break;
        case STAT_RELEASE_OK:{
            systray->showMessage(
                    tr("释放连接"),tr("IP连接释放完成"),
                    QSystemTrayIcon::Information,1000);
            }
            break;
        case STAT_CONFIG_OK:{
            //网络配置成功，如果设定成功联网后隐藏，则隐藏窗口。并开始计费
            systray->showMessage(
                tr("连接已建立"),tr("您随时可以右键点击托盘菜单中的”时间提示“查看在线时间"),
                QSystemTrayIcon::Information,2500);
            if(autoHide)
            this->hide();
            timerID=startTimer(60000);//启动计时
            timeOnline=0;
            }
            break;
        //对于所有从正常连接中退出的情况，一律通知在线时间
        case ERR_LOST_CONTACT:
        case ERR_NO_MONEY:
        case ERR_BREAK_LINK:
        case ERR_RESPOND_TIMEOUT:
        case ERR_LOCAL_KEEP:
        case STAT_LOGOFF:{
            killTimer(timerID);
            timerID=0;
            this->show();
            systray->showMessage(
                            tr("连接已终止"),tr("您共计在线: %1小时 %2分钟").arg(timeOnline/60).arg(timeOnline%60),
                            QSystemTrayIcon::Information,1000);
            }
            break;
        default:{
            if(!(linkThread->isRunning())){
                systray->showMessage(
                        tr("当前未连接"),tr("当前没有正在进行的连接"),
                        QSystemTrayIcon::Information,1500);
            }else
                systray->showMessage(
                            tr("在线时间"),tr("您已在线: %1小时 %2分钟").arg(timeOnline).arg(timeOnline%60),
                            QSystemTrayIcon::Information,1500);
            }
            break;
   }
}

/* Create the system tray and context menu */
void MainWindow::createTray()
{
    systray=new QSystemTrayIcon(QIcon(":/images/trayicon-B.png"),this);
    QMenu *trayMenu=new QMenu(this);
    trayMenu->addAction(QIcon(":/images/disconnect.png"),tr("断开连接"),this,SLOT(on_disconnect_clicked()));
    trayMenu->addAction(QIcon(":/images/close.png"),tr("退出客端"),this,SLOT(close()));
    trayMenu->addSeparator();
    trayMenu->addAction(QIcon(":/images/aboutProgram.png"),tr("关于程序"),this,SLOT(on_about_clicked()));
    trayMenu->addAction(QIcon(":/images/aboutAuthor.png"),tr("关于作者"),this,SLOT(on_aboutAuthor_clicked()));
    trayMenu->addSeparator();
    trayMenu->addAction(QIcon(":/images/timeHint.png"),tr("时间提示"),this,SLOT(tellStatus()));
    trayMenu->addAction(QIcon(":/images/redo.png"),tr("隐藏到托盘"),this,SLOT(hide()));
    trayMenu->addAction(QIcon(":/images/icon24.png"),tr("主界面"),this,SLOT(show()));

    systray->setContextMenu(trayMenu);
    QFont font;
    font.setPixelSize(13);
    trayMenu->setFont(font);

    systray->show();
}

/** Settings Operation **/

/* 获取可用网络连接并将相关信息拷贝至 name,description,hdAddrss三个列表 */
void MainWindow::loadNetLink()
{
    QList<QNetworkInterface>ifList=QNetworkInterface::allInterfaces();
    ifName.clear();
    description.clear();
    hdAddress.clear();
    for(int i=0;i<ifList.size();++i){
        if(ifList[i].name().contains("eth")){
            ifName.append(ifList[i].name());
            description.append(ifList[i].humanReadableName());
            hdAddress.append(ifList[i].hardwareAddress());
        }
    }
}

/* Load Default Settings ,so the program will be usable
   when used for the first time*/
void MainWindow::loadDefaultSettings()
{
    account.clear();
    password.clear();
    linkType="@internet";
    rememberPassword=true;
    autoHide=true;
    autoLink=false;
    waitRespond=1;
    retryNum=2;
    currentInterface=0;
    maxInterval=60;
    ifName.append("eth0");
    description.append("eth0");
}
/* Load Settings value to the UI elements */
void MainWindow::loadSettingsToUI()
{
    //Load settings to UI
    ui->account->setText(account);
    ui->password->setText(password);
    ui->linkType->setCurrentIndex(ui->linkType->findText(linkType));
    ui->rememberPassword->setChecked(rememberPassword);
    ui->autoHide->setChecked(autoHide);
    ui->autoLink->setChecked(autoLink);
    ui->waitRespond->setValue(waitRespond);
    ui->retryTimes->setValue(retryNum);
    ui->maxInterval->setValue(maxInterval);
    //网卡加载
    if(-1==currentInterface){//如果现在还没有可以用的网络接口信息，那么重新进行加载
        loadNetLink();
    }
    //将列表中的连接名字插入ComboBox中
    for(int i=0;i<ifName.size();++i)
        ui->interfaceBox->insertItem(i,description[i]);
    if(-1==currentInterface){
        ui->interfaceBox->setCurrentIndex(0);
    }else{
        ui->interfaceBox->setCurrentIndex(currentInterface);
    }
}

/* Load Settings from system record when startup */
void MainWindow::loadSettings()
{
    loadDefaultSettings();

    QSettings settings;
    //Load System settings,if settings exist,it will be overwritten
    account=settings.value("account",account).toString();
    password=settings.value("password",password).toString();
    linkType=settings.value("linktype",linkType).toString();
    rememberPassword=settings.value("rememberpassword",rememberPassword).toBool();
    autoLink=settings.value("autolink",autoLink).toBool();
    autoHide=settings.value("autohide",autoHide).toBool();
    waitRespond=settings.value("waitrespond",waitRespond).toInt();
    retryNum=settings.value("retrynum",retryNum).toInt();

    currentInterface=settings.value("CurrentInterface",currentInterface).toInt();
    ifName=settings.value("ListName",ifName).toStringList();
    description=settings.value("ListDescription",description).toStringList();
    hdAddress=settings.value("ListHDAddress",hdAddress).toStringList();
    maxInterval=settings.value("MaxInterval",maxInterval).toInt();

    loadSettingsToUI();
}
/* Update Settings after data verified by user*/
void MainWindow::updateSettings()
{
    QSettings settings;
    settings.setValue("account",account);
    if(rememberPassword)
        settings.setValue("password",password);
    else
        settings.setValue("password","");
    settings.setValue("linktype",linkType);
    settings.setValue("rememberpassword",rememberPassword);
    settings.setValue("autohide",autoHide);
    settings.setValue("autolink",autoLink);
    settings.setValue("waitrespond",waitRespond);
    settings.setValue("retrynum",retryNum);
    settings.setValue("CurrentInterface",currentInterface);
    settings.setValue("ListName",ifName);
    settings.setValue("ListDescription",description);
    settings.setValue("ListHDAddress",hdAddress);
    settings.setValue("MaxInterval",maxInterval);
    settings.sync();
}

void MainWindow::disconnectNet()
{
    if(this->isHidden())
        this->show();
    systray->setIcon(QIcon(":/images/trayicon-B.png"));
    if(linkThread->isRunning()){
        linkThread->stop();
        linkThread->wait();
    }
}

/* Actions before the program exited*/
void MainWindow::closeEvent(QCloseEvent *event)
{   
    disconnectNet();
    updateSettings();
    event->accept();
}

int MainWindow::showMessage(QString text,QString detail,QMessageBox::StandardButton button1,
                            bool hasButton2,QMessageBox::StandardButton button2)
{
    if(msgInUse)
        delete msgBox;
    else
        msgInUse=true;

    msgBox=new QMessageBox(this);
    msgBox->setText(text);
    if(!(detail.isEmpty()))
        msgBox->setDetailedText(detail);
    msgBox->addButton(button1);
    if(hasButton2)
        msgBox->addButton(button2);
    return msgBox->exec();
}

/* 对认证线程传来的消息进行解析和相应 */
void MainWindow::echoLinkThreadNotify(int id)
{
    int ret;
    if(id== NO_NOTIFY)
        return;
    if(IS_ERROR(id)){
        tellStatus(id);
        this->show();
    }
    
    switch(id){
        case STAT_SEARCH_SERVER:
                ret=showMessage(tr("寻找服务器……"),tr(""),QMessageBox::Cancel);
                if(ret==QMessageBox::Cancel)
                    disconnectNet();
                break;
        case STAT_SENDING_ACCOUNT:
                ret=showMessage(tr("发送用户名密码……"),tr(""),QMessageBox::Cancel);
                if(ret==QMessageBox::Cancel){
                    disconnectNet();
                }
                break;
        case STAT_AUTHEN_SUCCESS:
                //认证成功，不做任何回应，只提示用户
                showMessage(tr("认证成功！正在配置网络环境……"),tr(""),QMessageBox::NoButton);
                ui->disconnect->setEnabled(true);
                timeOnline=0;
                break;
        case STAT_CONFIG_OK:
                //网络配置成功，如果设定成功联网后隐藏，则隐藏窗口。并开始计费
                delete msgBox;
                if(autoHide)
                    this->hide();
                tellStatus(id);//通知状态，启动计时
                break;
        case STAT_RELEASE_IP:
        case STAT_RELEASE_OK:
                tellStatus(id);
                break;
        case STAT_LOGOFF:
                tellStatus(id);//停止计费,并通知上网用时
                this->show();
                break;
        //Error Status Notify
        case ERR_LINK_FREEZE:
                showMessage(tr("现在不是上网时间"),tr(""),QMessageBox::Ok);
                break;
        case ERR_SERVER_TIMEOUT:
                showMessage(tr("未能找到可用的服务器"),tr("可能原因：\n1.网线没插好\n2.选择的并非有线连接\n3.所在地区并不适用此认证"),QMessageBox::Ok);
                break;
        case ERR_START_DHCP:
                showMessage(tr("无法从DHCP返回IP地址"),tr("请检查该网络连接是否设置IP为自动获取，请检查您是否启动了DHCP服务"),QMessageBox::Ok);
                break;
        case ERR_ACCOUNT:
                showMessage(tr("用户名密码不正确"),tr(""),QMessageBox::Ok);
                ui->password->clear();
                break;
        case ERR_LOST_CONTACT:
                showMessage(tr("失去与服务器的连接"),tr("服务器方长时间没有反应，如果此现象出现太频繁，可能是网络不稳定，您也可以考虑在设置->网络->最大认证间隔 中调大这个值"),QMessageBox::Ok);
                disconnectNet();
                break;
        case ERR_RELEASE_IP:
                showMessage(tr("从连接中意外断开"),tr("可能是您在一个连接正在进行时重新发起了认证，或其他认证程序异常所致"),QMessageBox::Ok);
                break;
        case ERR_NO_MONEY:
                ret=showMessage(tr("服务器认为你卡里已经没钱了,是否尝试重新连接？"),tr(""),QMessageBox::Ok,true);
                if(ret==QMessageBox::Ok)//如果用户选择金额用完时重连，那么重启线程
                    linkThread->start();
                break;
        case ERR_RESPOND_TIMEOUT:
                showMessage(tr("服务器响应超时"),tr("服务器未能及时对您发起的认证进行响应，可能是服务器不稳定，您可以重新尝试一次"),QMessageBox::Ok);
                break;
        case ERR_LOCAL_KEEP:
                showMessage(tr("本地客户端响应超时"),tr("本地客户端由于出现意外，未能与服务器保持连接，请重试。\n如果此现象出现频繁，请联系作者"),QMessageBox::Ok);
                break;
        //程式错误讯息
        case ERR_OPEN_INTERFACE:
                showMessage(tr("无法打开网络接口"),tr("请确认网络连接是否被禁用"),QMessageBox::Ok);
                break;
        case ERR_FILTER_COMPILE:
                showMessage(tr("编译过滤器失败"),tr("这属于程式故障，如无法恢复正常请发邮件至tang_yanhan@126.com"),QMessageBox::Ok);
                break;
        case ERR_FILTER_INSTALL:
                showMessage(tr("安装过滤器失败"),tr("这属于程式故障，如无法恢复正常请发邮件至tang_yanhan@126.com"),QMessageBox::Ok);
                break;
        case ERR_PACKET_SEND:
                showMessage(tr("数据包发送失败"),tr("请检查是否网络连接已经被禁用"),QMessageBox::Ok);
                break;
        case ERR_CODE_BRANCH:
                showMessage(tr("程式进入了不应该进入的分支"),tr("这属于程式故障，如无法恢复正常请发邮件至tang_yanhan@126.com"),QMessageBox::Ok);
                disconnectNet();
                break;

        case ERR_DEVICE_INIT:
                showMessage(tr("初始化网络设备失败"),tr("请检查网络设备工作是否正常"),QMessageBox::Ok);
                break;
        case ERR_GET_LOCAL_MAC:
                showMessage(tr("无法获得网卡硬件地址"),tr("请检查网络设备是否工作正常，请不要在认证过程中自行更改设备状态"),QMessageBox::Ok);
                break;
        default:
                showMessage(tr("出现了程式未能预料的情况 代码：%1").arg(id),tr("这可能属于程式故障，如无法恢复正常请发邮件至tang_yanhan@126.com"),QMessageBox::Ok);
                break;
    }
    msgInUse=false;
}


void MainWindow::on_link_clicked()
{
    //如果当前接口索引值为-1，表示无可用接口,由于程序启动时已经对索引值为-1的情况进行了重加载，
    //如果此处仍为-1，表明用户可能已经禁用了所有连接或没有可用网卡
    if(-1==currentInterface){//linkThread.getConfig();
        QMessageBox::warning(this,tr("网卡无效"),tr("未发现可用网络连接，网络连接已被禁用或没有可用网卡"));
        return;
    }
    if(account.isEmpty()||password.isEmpty()){
        QMessageBox::warning(this,tr("账户无效"),tr("用户名密码不可为空"));
        return;
    }
    if(linkThread->isRunning()){
        QMessageBox::information(this,tr("无法认证"),tr("您已发起认证，请先断开连接！"));
        return;
    }
    //建立连接之前，从主线程获取必须信息
    linkThread->setLinkInfo(ifName[currentInterface],
                         account,password,
                         linkType,
                         retryNum,
                         waitRespond,
                         maxInterval);
    linkThread->start();
    timeOnline=0;
}

void MainWindow::on_detectNetLink_clicked()
{
    //首先要将原有连接清空
    ui->interfaceBox->clear();
    currentInterface=-1;
    //网卡加载
    loadNetLink();
    //将列表中的连接名字插入ComboBox中
    for(int i=0;i<ifName.size();++i)
        ui->interfaceBox->insertItem(i,description[i]);
    currentInterface=ui->interfaceBox->currentIndex();
}

/* Save all values in the board */
void MainWindow::on_apply_clicked()
{
    autoHide=ui->autoHide->isChecked();
    autoLink=ui->autoLink->isChecked();
    waitRespond=ui->waitRespond->value();
    retryNum=ui->retryTimes->value();
    currentInterface=ui->interfaceBox->currentIndex();
    this->resize(HALF_WIDTH,HEIGHT);
}

void MainWindow::on_linkType_currentIndexChanged(QString text)
{
    linkType=text;
}

/* RememberPassword is dealed when check state changed */
void MainWindow::on_rememberPassword_toggled(bool checked)
{
    rememberPassword= checked;
}

/* Settings click event will switch the panel to appear or hide */
void MainWindow::on_settings_clicked()
{
    if(this->width()==HALF_WIDTH){
        this->resize(FULL_WIDTH,HEIGHT);
    }else{
        autoHide=ui->autoHide->isChecked();
        autoLink=ui->autoLink->isChecked();
        waitRespond=ui->waitRespond->value();
        retryNum=ui->retryTimes->value();
        currentInterface=ui->interfaceBox->currentIndex();
        this->resize(HALF_WIDTH,HEIGHT);
    }
}

//如果点击了取消，那么一切界面上的显示还原，亦对任何属性做出改动
void MainWindow::on_cancel_clicked()
{
    loadSettingsToUI();
    this->resize(HALF_WIDTH,HEIGHT);
}

void MainWindow::on_exit_clicked()
{
    this->close();
}

void MainWindow::on_account_editingFinished()
{
    account=ui->account->text();
}

void MainWindow::on_password_editingFinished()
{
    password=ui->password->text();
}

void MainWindow::on_disconnect_clicked()
{
    disconnectNet();
}

void MainWindow::on_aboutAuthor_clicked()
{
    QMessageBox::about(this,tr("关于程式作者"),
                       tr("<h1>作者简介</h1>"
                          "<h2>千羽鸣，<font color=blue>tang_yanhan@126.com.</font><br>值此妇女节之际，向各位女士们致敬！！<br>2011.3.8</h2>"
                          ));
}

void MainWindow::on_about_clicked()
{
    QMessageBox::about(this,tr("关于 Linkapp 联创认证客户端"),
                       tr("<h1><font color=blue>Linkapp 联创认证客户端</font></h1>"
                          "<h2><strong><font color=blue>Linkapp联创认证客户端</font></strong>是由千羽鸣在矿大前辈对联创802.1x校园认证协议的分析基础上进一步完善而成的程式，"
                          "它参考了以往dayongxie（矿大）的linkage及jerry版本，以及HeiHer（徐工）的linkage-4-linux版本，使用Qt构建了现在的Linux平台版本"
                          "版本,并自动遵从LGPL(v2.1)开源协议，所有源代码可以从http://code.google.com/p/linkapp-for-linkage-authentication获得。"
                          "<br>您如果对我的程式有任何问题或建议，请致信 tang_yanhan@126.com<br>千羽鸣<br>2011.3.8</h2>"));
}

void MainWindow::on_whatsthis_clicked()
{
    QMessageBox::information(this,tr("帮助"),tr("您可以在这里选择认证，目前仅支持联创802.1x一种认证方式！"));
}

void MainWindow::on_autoLink_toggled(bool checked)
{
    autoLink=checked;
}
