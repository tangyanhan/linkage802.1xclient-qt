#ifndef LINKTHREAD_H
#define LINKTHREAD_H

#include "global.h"
#include <pcap.h>
#include <libnet.h>
#include <QThread>
#include <QMutex>

typedef struct pcap_pkthdr PacketHeader;
typedef u_char Octet;

#define ACCOUNT_MAX  30
#define PASSWORD_MAX 20

//To store current state of Authentication.Only Two states
//in the run() method,it's changed from Start to Keep after
//a successful authentication
enum AuthStatus{
    Start,
    Keep
};

class LinkThread:public QThread
{
    Q_OBJECT

public:
    int setLinkInfo(QString deviceName,
                   QString account,QString password,QString linkType,//Account info
                   int retryNum,
                   int waitRespond,//seconds
                   int maxInterval //seconds
                   );
    void run();
    void stop();
    int initSendDevice();
    int initListenDevice(const int timeOut,char *filter);
    int installFilter(char *filter);

    inline int analyzePacket(const pcap_pkthdr *header,const Octet *packet);

    void sendStartPacket();
    void replyIdentityPacket(const Octet*,int);
    void replyPasswordPacket(const Octet*);
    void sendLogoffPacket();

    void startDHCP();
    void releaseIP();
signals:
    void notify(int num);
private:
    volatile bool stopped;
    //Initial basic Settings
    char account[ACCOUNT_MAX];
    char password[PASSWORD_MAX];
    int  waitRespond;//Seconds
    int  maxInterval;//Seconds
    int  retryNum;

    //Link Settings
    Octet localMAC[6];
    Octet destMAC[6];

    pcap_t *listenDevice;
    libnet_t *sendDevice;
    char devName[100];

    AuthStatus status;
    QMutex mutex;
};

#endif // LINKTHREAD_H
