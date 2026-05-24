#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

// preventing the compiler from adding padding bytes
#pragma pack(push,1)

// unique message identifiers
enum class MsgType : uint8_t{
    
    NewOrder = 'N',
    Cancel = 'C'

};

// fixed 17-byte wire packet structure for a New Order 
struct WireNewOrder{

    int32_t orderId;
    int32_t price;
    int32_t quantity;
    uint8_t side;

};

//fixed 5-byte wire packet structure for cancel
struct WireCancelOrder{
    
    int32_t orderId;

};

// wrapper header to prefix every message on the wire
struct PacketHeader{
    
    MsgType type;
    uint32_t length;

};

#pragma pack(pop)

#endif
