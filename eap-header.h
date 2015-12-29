#ifndef EAPHEADER_H
#define EAPHEADER_H

/**
   EAP Flags
**/

#define OFFSET_EAP_TYPE 0x12

#define EAP_REQUEST     0x01
#define EAP_REPLY       0x02
#define EAP_SUCCESS     0x03
#define EAP_FAILURE     0x04


//EAP-Request Flags
#define OFFSET_REQUEST_TYPE 0x16

#define REQUEST_IDENTITY    0x01
#define REQUEST_PASSWORD    0x99


//EAP-Success Flags
#define OFFSET_SUCCESS_TYPE 0x26

#define SUCCESS_AUTH        0x03 //Initial Auth Success
#define SUCCESS_KEEP        0x00 //Keep Auth Success


//EAP-Failure Flags
#define OFFSET_FAILURE_TYPE 0x26

#define FAILURE_ACCOUNT     0x97
#define FAILURE_START_DHCP  0x08
#define FAILURE_LOGOFF      0x00 //Normal Logoff
#define FAILURE_IP_RELEASE  0x01 //no instant IP-Release after loging off
#define FAILURE_FREEZE      0x03 //Link Freeze
#define FAILURE_NO_MONEY    0x04 //money used up
#define FAILURE_BREAK_LINK  0x05 //Forced to break link
#define FAILURE_LOCAL_KEEP  0x06 //Local failed to reply keep packets



#define MAX_CAP_LEN 100
#define EAP_MIN_LEN  0x30
#define EAP_MAX_LEN  MAX_CAP_LEN

#endif // EAPHEADER_H
