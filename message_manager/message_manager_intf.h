#pragma once

#include <functional>
#include "xtransport/transport_fwd.h"

namespace top {
namespace transport {

typedef std::function<void(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet)> HandlerProc;

class MessageManagerIntf {
public:
    static MessageManagerIntf* Instance();
    virtual ~MessageManagerIntf() {}
    virtual void RegisterMessageProcessor(uint32_t message_type, HandlerProc callback) = 0;
    virtual void UnRegisterMessageProcessor(uint32_t message_type) = 0;
    virtual void HandleMessage(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet) = 0;
};

}  // namespace transport
}  // namespace top
