#ifndef GLOBAL_H
#define GLOBAL_H

/** Test Switch **/

#define ON   1
#define OFF  0

#define TEST OFF

#if   TEST
#include <stdio.h>
#define   KTT printf("\nFile:%s-------Line:%d",__FILE__,__LINE__);

#else

#define KTT /**/
#define NDEBUG
#endif

#include <assert.h>

/** Codes for communication between MainWindow and LinkThread **/

#define STAT      0
#define NO_NOTIFY STAT
#define IS_STATUS(k) (k>STAT && k<=STAT_RELEASE_OK)
//Notify Code that may appear during a normal authentication
#define STAT_SEARCH_SERVER    (STAT+1)
#define STAT_SENDING_ACCOUNT  (STAT+2)
#define STAT_AUTHEN_SUCCESS   (STAT+3)
#define STAT_CONFIG_OK        (STAT+4)
#define STAT_LOGOFF           (STAT+5)
#define STAT_RELEASE_IP       (STAT+6)
#define STAT_RELEASE_OK       (STAT+7)


#define ERR      20
#define ERR_MAX  40
#define IS_ERROR(k)           (k>ERR && k<ERR_MAX)

//Error Code that appear during an authentication
#define ERR_LINK_FREEZE        (ERR+1)
#define ERR_SERVER_TIMEOUT     (ERR+2)
#define ERR_START_DHCP         (ERR+3)
#define ERR_ACCOUNT            (ERR+4)
#define ERR_LOST_CONTACT       (ERR+5)
#define ERR_RELEASE_IP         (ERR+6)
#define ERR_NO_MONEY           (ERR+7)
#define ERR_BREAK_LINK         (ERR+8)
#define ERR_RESPOND_TIMEOUT    (ERR+9)
#define ERR_LOCAL_KEEP         (ERR+10)

//Error for errors inside code piece
#define CODE_ERR 30

#define ERR_OPEN_INTERFACE     (CODE_ERR+1)
#define ERR_FILTER_COMPILE     (CODE_ERR+2)
#define ERR_FILTER_INSTALL     (CODE_ERR+3)
#define ERR_PACKET_SEND        (CODE_ERR+4)
#define ERR_CODE_BRANCH        (CODE_ERR+5)
#define ERR_DEVICE_INIT        (CODE_ERR+6)
#define ERR_GET_LOCAL_MAC      (CODE_ERR+7)
#define ERR_SYSTEM_COMMAND     (CODE_ERR+8)

#endif // GLOBAL_H
