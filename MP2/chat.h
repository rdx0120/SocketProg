#include<string.h> 

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

#define SBCP_PROTOCOL_VERSION   3   // protocol version is 3

typedef enum {
    ATTR_TYPE_REASON = 1,
    ATTR_TYPE_USER_NAME = 2,
    ATTR_TYPE_CLIENT_COUNT = 3,
    ATTR_TYPE_MESSAGE = 4,
    ATTR_TYPE_INVALID = 0x7F,     // 7 bits max.
} AttrType;

typedef enum {
    MSG_TYPE_JOIN = 2,
    MSG_TYPE_FWD = 3,
    MSG_TYPE_SEND = 4,
    // Bonus Types
    MSG_TYPE_NAK = 5,
    MSG_TYPE_OFFLINE = 6,
    MSG_TYPE_ACK = 7,
    MSG_TYPE_ONLINE = 8,
    MSG_TYPE_IDLE = 9,
    MSG_TYPE_INVALID = 0xFFFF,   // 9 bits max. -> assign 2 bytes.
} MsgType;

typedef struct _SBCP_Attribute{
    int type;
    int length;
    char payload[512]; 
}SBCP_Attribute;

typedef struct _SBCP_Message{
    unsigned int vrsn : 9;
    unsigned int type : 7;
    int length;
    SBCP_Attribute stAttribute1;
    SBCP_Attribute stAttribute2;
}SBCP_Message;

// Macros
#if 0
#define GET_MESSAGE_VRSN(stMessage) ((stMessage.vrsn_and_type) >> 7 & (UINT16)(0x1FF))
#define GET_MESSAGE_TYPE(stMessage) ((stMessage.vrsn_and_type) & (UINT16)(0x7F))
#define SET_MESSAGE_VRSN(pMessage, set_value) (((SBCP_Message*)(pMessage))->vrsn_and_type |= (UINT16)((set_value & (0x1FF)) << 7))
#define SET_MESSAGE_TYPE(pMessage, set_value) (((SBCP_Message*)(pMessage))->vrsn_and_type |= (UINT16)((set_value & (0x7F))))
#define CLEAR_MESSAGE_VRSN(pMessage) (((SBCP_Message*)(pMessage))->vrsn_and_type &= (UINT16)(0x7F))
#define CLEAR_MESSAGE_TYPE(pMessage) (((SBCP_Message*)(pMessage))->vrsn_and_type &= (UINT16)(0x1FF << 7))

#define GET_MESSAGE_VRSN(stMessage) ((stMessage.vrsn_and_type) >> 8 & (UINT16)(0xFF))
#define GET_MESSAGE_TYPE(stMessage) ((stMessage.vrsn_and_type) & (UINT16)(0xFF))
#define SET_MESSAGE_VRSN(pMessage, set_value) (((SBCP_Message*)(pMessage))->vrsn_and_type |= (UINT16)((set_value & (0xFF)) << 8))
#define SET_MESSAGE_TYPE(pMessage, set_value) (((SBCP_Message*)(pMessage))->vrsn_and_type |= (UINT16)((set_value & (0xFF))))
#define CLEAR_MESSAGE_VRSN(pMessage) (((SBCP_Message*)(pMessage))->vrsn_and_type &= (UINT16)(0xFF))
#define CLEAR_MESSAGE_TYPE(pMessage) (((SBCP_Message*)(pMessage))->vrsn_and_type &= (UINT16)(0xFF << 8))
#endif

#if 0
void set_sbcp_attribute(SBCP_Attribute* _pAttribute, AttrType _eAttrType, UINT16 _length, char* _payload){
    _pAttribute->length = _length;
    _pAttribute->type = _eAttrType;
    strcpy(_pAttribute->payload, _payload);
}

void set_sbcp_message(SBCP_Message* _pSbcpMessage, UINT16 _nVrsn, MsgType _eMessageType, UINT16 _nLength, SBCP_Attribute _stAttribute1, SBCP_Attribute _stAttribute2){
    CLEAR_MESSAGE_VRSN(_pSbcpMessage);
    CLEAR_MESSAGE_TYPE(_pSbcpMessage);
    SET_MESSAGE_VRSN(_pSbcpMessage, _nVrsn);
    SET_MESSAGE_TYPE(_pSbcpMessage, _eMessageType);
    _pSbcpMessage->length = _nLength;
    _pSbcpMessage->stAttribute1 = _stAttribute1;
    _pSbcpMessage->stAttribute2 = _stAttribute2;
    // printf("Message Type: %d\n", _eMessageType);
    // printf("version and type: 0x%x\n", _pSbcpMessage->vrsn_and_type);
    // printf("Message Len: %d\n", _pSbcpMessage->length);
    // printf("Attr Type: %d, Attr Len: %d", _pSbcpMessage->stAttribute1.type, _pSbcpMessage->stAttribute1.length);
}

#endif