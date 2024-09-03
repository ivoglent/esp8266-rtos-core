#pragma once
#include "../../Core.h"
#include "mdns.h"

struct RegisterServiceAddress {
    ip4_addr_t* address;
    int port;
    RegisterServiceAddress(ip4_addr_t* ip, int port) {
        address = ip;
        port = port;
    }
};

class MdnsService {
private:
public:
    explicit MdnsService();
    void setup();
    RegisterServiceAddress* lookUp();
};
