#include <QtGui/QApplication>
#include <QTextCodec>
#include <QDesktopWidget>
#include <unistd.h>
#include <sys/types.h>
#include "mainwindow.h"
#include "qtsingleapplication.h"

int main(int argc, char *argv[])
{
    //Set all ASCII string wrapped by tr() decode as UTF-8
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QApplication::setOrganizationName("tang_yanhan@126.com");
    QApplication::setApplicationName("linkapp");

    QtSingleApplication a(argc, argv);

    if(getuid()!=0){
        QMessageBox::critical(NULL,QApplication::tr("权限不够"),QApplication::tr("程序运行需要root权限！！"));
        exit(0);
    }
    if(a.isRunning()){
        QMessageBox::warning(NULL,QApplication::tr("程序已经启动"),QApplication::tr("已经有客户端在运行了!"),QMessageBox::Ok);
        exit(0);
    }
    MainWindow w;
    w.show();
    //Show the window in the middle of the screen
    w.move ((QApplication::desktop()->width() - w.width())/2,(QApplication::desktop()->height() - w.height())/2);
    return a.exec();
}
