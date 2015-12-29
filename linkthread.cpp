#include "linkthread.h"
#include "global.h"
#include "md5.h"
#include <pcap.h>
#include <sys/types.h>
#include "eap-header.h"

AuthStatus status;

#define SYSTEM_UP_INTERFACE(command)   {sprintf(command,"ifconfig %s up",devName);}
#define SYSTEM_DOWN_INTERFACE(command) {sprintf(command,"ifconfig %s down",devName);}


/* Pcap开始认证时使用的过滤器模板*/
#define PCAP_START_FILTER "ether dst not ff:ff:ff:ff:ff:ff and ether dst %02x:%02x:%02x:%02x:%02x:%02x and ether proto 0x888e"

/* pcap在进入认证保持阶段时使用的过滤器模板 */
#define PCAP_KEEP_FILTER "ether dst not ff:ff:ff:ff:ff:ff and ether dst %02x:%02x:%02x:%02x:%02x:%02x and ether src %02x:%02x:%02x:%02x:%02x:%02x and ether proto 0x888e"

/* 从外界获取各种参数 */
int LinkThread::setLinkInfo(QString _deviceName,
                       QString _account,
                       QString _password,
                       QString _linkType,
                       int _retryNum,
                       int _waitRespond,
                       int _maxInterval)
{
    assert(!(_deviceName.isEmpty()));
    assert(!(_account.isEmpty()));
    assert(!(_password.isEmpty()));
    assert(!(_linkType.isEmpty()));
    
    strcpy(devName,_deviceName.toAscii());

    _account+=_linkType;
    strcpy(account,_account.toAscii());
    strcpy(password,_password.toAscii());
    memset(destMAC,0x00,6);

    retryNum=_retryNum;
    waitRespond=_waitRespond;
    maxInterval=_maxInterval;
    listenDevice=NULL;
    sendDevice=NULL;

    return 0;
}


int LinkThread::initSendDevice()
{
    char command[100];
    SYSTEM_UP_INTERFACE(command)
    char errBuf[LIBNET_ERRBUF_SIZE];
    if((sendDevice=libnet_init(LIBNET_LINK,devName,errBuf))==NULL){
        emit notify(ERR_DEVICE_INIT);
        return -1;
    }

    return 0;
}

/* Initialize the device with given time out and filter
 * NB: here timeout count by second,yet by millisecond
 * for pcap_open_live() */
int LinkThread::initListenDevice(const int timeOut,char *filter)
{
    char errBuffer[80];

    listenDevice=pcap_open_live(devName,65536,//So all data will be listened
                                0,(timeOut*1000),errBuffer);
    if(!listenDevice){
        emit notify(ERR_OPEN_INTERFACE);
        return ERR_OPEN_INTERFACE;
    }

    int ret=installFilter(filter);

    return ret;
}

/* Install a specified filter for the sendDevice */
int LinkThread::installFilter(char *filter)
{
    assert(filter);

    //Structure to store filter code
    struct  bpf_program filterProgram;

    if(pcap_compile(listenDevice,&filterProgram,filter,0,0)==-1){
        emit notify(ERR_FILTER_COMPILE);
        return ERR_FILTER_COMPILE;
    }
    if(pcap_setfilter(listenDevice,&filterProgram)==-1){
        emit notify(ERR_FILTER_INSTALL);
        return ERR_FILTER_INSTALL;
    }
    //Release the filter code
    pcap_freecode(&filterProgram);

    return 0;
}


/* Body of the thread.
 *
 * Stop Condition: 1.stopped==true
 *                 2.Inner error occured
 * If an inner error occurred,it will emit a signal
 * carring the error code before return directly
 * signals are also emitted to notify the MainWindow
 * class during a normal authentication
 */
void LinkThread::run()
{
    struct pcap_pkthdr *header;
    const Octet       *packet=NULL;
    char  filter[150];
    struct libnet_ether_addr *etherAddress;

    int deviceHandle=0;
    fd_set readSet;
    struct timespec timeOut;
    
    {
        QMutexLocker locker(&mutex);
        stopped=false;
        status=Start;
    }

    if(initSendDevice()){
        emit notify(ERR_DEVICE_INIT);
        return;
    }

    if((etherAddress=libnet_get_hwaddr(sendDevice))==NULL){
        emit notify(ERR_GET_LOCAL_MAC);
        return;
    }
    memcpy(localMAC,etherAddress,sizeof(localMAC));

    //Make a filter for the start of authentication
    strcpy(filter,PCAP_START_FILTER);
    sprintf(filter,PCAP_START_FILTER,
            localMAC[0],localMAC[1],localMAC[2],localMAC[3],localMAC[4],localMAC[5]);

    if(initListenDevice(waitRespond,filter)){
        emit notify(ERR_DEVICE_INIT);
        return;
    }
    
    emit notify(STAT_SEARCH_SERVER);
    sendStartPacket();
    
    //Begin listening packets
    int ret=0;
    int notifyID=0;
    deviceHandle = pcap_fileno(listenDevice);
    FD_ZERO(&readSet);
    FD_SET(deviceHandle,&readSet);
    timeOut.tv_sec=waitRespond;
    timeOut.tv_nsec=0;

    while(!stopped){
        ret=pselect(deviceHandle+1,&readSet,NULL,NULL,&timeOut,NULL);
        switch(ret){
            case -1:
                emit notify(ERR_DEVICE_INIT);
                goto ClearUpThread;
            case 0:
            if(!(retryNum--)){
                emit notify(ERR_SERVER_TIMEOUT);
                goto ClearUpThread;
            }else{
                sendStartPacket();
                continue;
            }
        }

        ret=pcap_next_ex(listenDevice,&header,&packet);

        notifyID=analyzePacket(header,packet);

        if(notifyID!=NO_NOTIFY)  emit notify(notifyID);

        if(IS_ERROR(notifyID))   goto ClearUpThread;

       //If authentication succeeded,then goto the next period
        if(STAT_AUTHEN_SUCCESS==notifyID)   break;
    }

    if(stopped)
        goto ClearUpThread;

    status=Keep;

    startDHCP();
    msleep(700); //Must wait until the interface is available
    emit notify(STAT_CONFIG_OK);

    strcpy(filter,PCAP_KEEP_FILTER);
    sprintf(filter,PCAP_KEEP_FILTER,
            localMAC[0],localMAC[1],localMAC[2],localMAC[3],localMAC[4],localMAC[5],
            destMAC[0],destMAC[1],destMAC[2],destMAC[3],destMAC[4],destMAC[5]);


    pcap_close(listenDevice);
    libnet_destroy(sendDevice);
    listenDevice=NULL;
    sendDevice  =NULL;

    if(initSendDevice() ||
           initListenDevice(maxInterval,filter)){
        emit notify(ERR_DEVICE_INIT);
            return;
        }
KTT
    //Link has been established,now it's the time to keep the auth.
    deviceHandle = pcap_fileno(listenDevice);
    FD_ZERO(&readSet);
    FD_SET(deviceHandle,&readSet);
    timeOut.tv_sec=maxInterval;
    timeOut.tv_nsec=0;

    while(1){
        ret=pselect(deviceHandle+1,&readSet,NULL,NULL,&timeOut,NULL);
        switch(ret){
            case -1:
                emit notify(ERR_DEVICE_INIT);
                goto ClearUpThread;
            case 0:
                emit notify(ERR_LOST_CONTACT);
                goto ClearUpThread;
        }
        ret=pcap_next_ex(listenDevice,&header,&packet);

        notifyID=analyzePacket(header,packet);KTT
        //如果遇到了异常情况或者正常退出，那么清空线程后退出
        if(notifyID == STAT_LOGOFF ||
           IS_ERROR(notifyID)){
            emit notify(notifyID);KTT
            break;
        }
    }

    ClearUpThread:
    if(listenDevice)    pcap_close(listenDevice);
    if(sendDevice)      libnet_destroy(sendDevice);
}

/* turn the threadStatus flag to stopped */
void LinkThread::stop()
{    
    QMutexLocker locker(&mutex);
    stopped=true;
    if(Keep==status){
        sendLogoffPacket();
        emit notify(STAT_RELEASE_IP);
        releaseIP();
    }
}

/* analyze the packet received */
inline int LinkThread::analyzePacket(const pcap_pkthdr *header, const Octet *packet)
{
    assert(header);
    assert(packet);

    int result=NO_NOTIFY;
    if(header->len < EAP_MIN_LEN ||
       header->len > EAP_MAX_LEN ){
        result= NO_NOTIFY;
        goto RET_VALUE;
    }

    switch(packet[ OFFSET_EAP_TYPE ]){
            case EAP_REQUEST:{
                switch(packet[OFFSET_REQUEST_TYPE]){
                    case REQUEST_IDENTITY:
                        replyIdentityPacket(packet,header->len);
                        result = STAT_SENDING_ACCOUNT;
                        break;
                    case REQUEST_PASSWORD:
                         replyPasswordPacket(packet);
                         result= NO_NOTIFY;
                         break;
                    /* This part is commented because there's always
                    a RSA Public Key Request after a successful authentication
                    Yet we don't need to deal with such situation
                    default:{KTT
                         return ERR_CODE_BRANCH;
                    }
                    */
                }
                break;  //missing of this break e(maxInterval,filter))
                //causes hours of work!
            }
            case EAP_SUCCESS:{
                switch(packet[OFFSET_SUCCESS_TYPE]){
                    case SUCCESS_AUTH:
                        result = STAT_AUTHEN_SUCCESS;
                        break;
                    case SUCCESS_KEEP:
                        result = NO_NOTIFY;
                        break;
                    default:
                        result = ERR_CODE_BRANCH;
                        break;
                }
                break;
                }
                break;
            case EAP_FAILURE:{
                switch(packet[OFFSET_FAILURE_TYPE]){
                    case FAILURE_ACCOUNT:
                        result = ERR_ACCOUNT;
                        break;
                    case FAILURE_START_DHCP://Failed to start DHCP Process in time
                        result = ERR_START_DHCP;
                        break;
                    case FAILURE_NO_MONEY:
                        result = ERR_NO_MONEY;
                        break;
                    case FAILURE_LOGOFF://Disconnect from the net normally
                        result = STAT_LOGOFF;
                        break;
                    //Regard IP-Release Failure as normal logoff
                    case FAILURE_IP_RELEASE:
                        result = STAT_LOGOFF;
                        break;
                    case FAILURE_BREAK_LINK:
                        result = ERR_BREAK_LINK;
                        break;
                    case FAILURE_FREEZE:
                        result = ERR_LINK_FREEZE;
                        break;
                    case FAILURE_LOCAL_KEEP:
                        result = ERR_LOCAL_KEEP;
                        break;
                    default:
                        printf("\nUnknownFailure:%02x",packet[OFFSET_FAILURE_TYPE]);
                        result= ERR_CODE_BRANCH;
                        break;
                }
            }
            break;
    }

    RET_VALUE:
    return result;
}

/* Send EAPOL-Start Packet to start an authentication */
void LinkThread::sendStartPacket()
{
    assert(sendDevice);

   //EAPOL-Start Packet Frame for linkage 802.1x
    static uint8_t broadPacket[30] = {
            0xff,0xff,0xff,0xff,0xff,0xff,
            0x00,0x00,0x00,0x00,0x00,0x00,
            0x88,0x8E,0x01,0x01,
            0x00,0x00,
            0x6C,0x69,0x6E,0x6B,0x61,0x67,0x65, //"linkage"
            0x00,0x00,0x00,0x00,0x00};

    //Fill the MAC Frame
   memcpy( broadPacket+6,localMAC, 6 );

   if(libnet_write_link(sendDevice,broadPacket,30)!=30){
        emit notify(ERR_PACKET_SEND);
    }
}

/* Edit a copy of the EAP-Identity packet from the server and reply it! */
void LinkThread::replyIdentityPacket(const Octet *_identityPacket,int packetLen)
{
    assert(sendDevice);

    Octet *identityPacket=new Octet[packetLen];
    memcpy(identityPacket,_identityPacket,packetLen);
    uint8_t linkage[16]={
            0x6C,0x69,0x6E,0x6B,0x61,0x67,0x65,0x00,0x34,0xDE,0xB6,0x78,0x00,0xE7,0x00,0xE7};

    //The first 3 octets of MAC Address is 
    //enough to tell whether destMAC has been assigned with an address
    if(!(destMAC[0]|destMAC[1]|destMAC[2]))
        memcpy(destMAC,identityPacket+6,6);

    identityPacket[OFFSET_EAP_TYPE]=EAP_REPLY;

    static unsigned char md5_code[0x38]={
        '0','1','2','3','4','5','6','7','8','9',
        '0','1','2','3','4','5','6','7','8','9',
        '0','1','2','3','4','5','6','7','8','9',
        '0','1','2','3','4','5','6','7','8','9',
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    unsigned char md[16];
    MD5_CTX md5_contex;
    int accountLen,i;

    accountLen=strlen(account);
    memcpy(identityPacket,destMAC,6);
    memcpy(identityPacket+6,localMAC,6);
    memcpy(md5_code+40,identityPacket+0x23,4);
    MD5Init(&md5_contex);
    MD5Update(&md5_contex,md5_code,0x38);
    MD5Final(md,&md5_contex);
    for(i=0; i<8; i++)
    {
            linkage[8+i]^=md[i];
    }
    memcpy(identityPacket+0x17+accountLen,linkage,16);
    *(short *)(identityPacket+0x10) = htons((short)(5+accountLen));//len
    *(short *)(identityPacket+0x14) = *(short *)(identityPacket+0x10);//len
    memcpy(identityPacket+0x17,account,accountLen);//fill the user name
    if((libnet_write_link(sendDevice,identityPacket,packetLen)!=packetLen)){
        emit notify(ERR_PACKET_SEND);
    }
}

/* Send EAP-Challenge Packet */
void LinkThread::replyPasswordPacket(const Octet *challengePacket)
{
    assert(sendDevice);

    static uint8_t ackPackage[0x38] = {
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x88,0x8E,0x01,0x00,
            0x00,0x16,0x02,0x6A,0x00,0x16,0x99,0x12,
            0xC2,0x7B,0xA6,0xB5,0x75,0x08,0x0F,0xE2,
            0x16,0xBB,0x26,0xCD,0xAA,0x66,0x70,0x65,
            0x6C,0x69,0x6E,0x6B,0x61,0x67,0x65,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    static unsigned char varPublic[0x100] = {
         0x75, 0x73, 0x65, 0x20, 0x6D, 0x64, 0x35, 0x20,
         0x65, 0x72, 0x72, 0x6F, 0x72, 0x21, 0x21, 0x00,
         0x8B, 0xF4, 0x6A, 0x01, 0xA1, 0x00, 0x2C, 0x43,
         0x00, 0x50, 0xFF, 0x15, 0x00, 0x68, 0x43, 0x00,
         0x3B, 0xF4, 0xE8, 0x70, 0xB7, 0xFF, 0xFF, 0x8B,
         0xF4, 0x6A, 0x01, 0xA1, 0x8C, 0x2D, 0x43, 0x00,
         0x50, 0xFF, 0x15, 0x00, 0x68, 0x43, 0x00, 0x3B,
         0xF4, 0xE8, 0x59, 0xB7, 0xFF, 0xFF, 0x8B, 0xF4,
         0x6A, 0x01, 0xA1, 0x98, 0x30, 0x43, 0x00, 0x50,
         0xFF, 0x15, 0x00, 0x68, 0x43, 0x00, 0x3B, 0xF4,
         0xE8, 0x42, 0xB7, 0xFF, 0xFF, 0x8B, 0xF4, 0x6A,
         0x01, 0xA1, 0x10, 0x2C, 0x43, 0x00, 0x50, 0xFF,
         0x15, 0x00, 0x68, 0x43, 0x00, 0x3B, 0xF4, 0xE8,
         0x2B, 0xB7, 0xFF, 0xFF, 0x8B, 0xF4, 0x6A, 0x01,
         0xA1, 0x04, 0x2C, 0x43, 0x00, 0x50, 0xFF, 0x15,
         0x00, 0x68, 0x43, 0x00, 0x3B, 0xF4, 0xE8, 0x14,
         0xB7, 0xFF, 0xFF, 0x8B, 0xF4, 0x6A, 0x01, 0xA1,
         0x90, 0x2D, 0x43, 0x00, 0x50, 0xFF, 0x15, 0x00,
         0x68, 0x43, 0x00, 0x3B, 0xF4, 0xE8, 0xFD, 0xB6,
         0xFF, 0xFF, 0x8B, 0xF4, 0x6A, 0x01, 0xA1, 0xF0};
    unsigned char md5_code[17];

    unsigned char  md5Dig[16];       //result of md5 sum
    MD5_CTX md5_contex;
    int i,sum=0,di;
    int passwordLen;
    passwordLen=strlen(password);

    memcpy(ackPackage,destMAC,6);
    memcpy(ackPackage+6,localMAC,6);

    ackPackage[OFFSET_EAP_TYPE] = EAP_REPLY;
    ackPackage[0x13]=challengePacket[0x13];

    *(ackPackage+0x16) = *(challengePacket+0x16);
    *(short *)(ackPackage+0x10) = htons((short)( 22)); //len
    *(short *)(ackPackage+0x14) = *(short *)( ackPackage+0x10 );

    memcpy(md5_code,challengePacket+0x17,16);
    md5_code[16] = challengePacket[0x13];
    MD5Init(&md5_contex);
    MD5Update(&md5_contex,md5_code,17);
    MD5Final(md5Dig,&md5_contex);
    for (i=0;i<0x10;i++)
            sum+=challengePacket[0x17+i];
    di = sum%10;
    di<<=4;
    for (i=0;i<16;i++)
        md5Dig[i] ^= varPublic[di+i];

    for (i=0; i<passwordLen; i++)
        md5Dig[i] ^= password[i];
    ackPackage[0x17]=18;
    memcpy(ackPackage+0x18,md5Dig,16);
    if((libnet_write_link(sendDevice,ackPackage,0x38))!=0x38){
        emit notify(ERR_PACKET_SEND);
    }

}

/* Send EAPOL-Logoff to end an authentication */
void LinkThread::sendLogoffPacket()
{
    assert(sendDevice);

    //EAPOL-Start Packet Frame for linkage 802.1x
    static uint8_t LogoffPacket[34]={
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x88, 0x8E, 0x01, 0x02,
         0x00, 0x00, 0x6C, 0x69, 0x6E, 0x6B, 0x61, 0x67,
         0x65, 0x00, 0x00, 0xC9, 0x00, 0x19, 0x30, 0x00,
         0x00, 0x01};

    //Fill the MAC Frame
    memcpy( LogoffPacket,destMAC, 6);
    memcpy( LogoffPacket+6,localMAC, 6 );

    if(libnet_write_link(sendDevice,LogoffPacket,34)!=34){
        emit notify(ERR_PACKET_SEND);
    }
}

/* Start DHCP procedure after a successful authentication
 * It has been proved that there's a higher oppotunity for
 * DHCP Success using fork() instead direct system call
 */
void LinkThread::startDHCP()
{
    if(fork()==0){
        system("service network-manager restart");
    }
}

/* Release IP Address after an authentication ended */
void LinkThread::releaseIP()
{
    emit notify(STAT_RELEASE_IP);
    char command[100];
    sprintf(command,"dhclient -r %s",devName);
    system(command);
    emit notify(STAT_RELEASE_OK);
}
