#ifndef CUSTOM_STRUCT_H
#define CUSTOM_STRUCT_H

// Header structure
struct Header {
    unsigned int version : 9;
    unsigned int type : 7;
    int length;
};

// Message Attribute structure
struct MessageAttribute {
    int type;
    int length;
    char payload[512];
};

// Message structure
struct Message {
    struct Header header;
    struct MessageAttribute attribute[2];
};

// Client information structure
struct ClientInfo {
    int fd;
    char username[16];
    int ClientCount;  // Add this member
};

#endif  // CUSTOM_STRUCT_H
