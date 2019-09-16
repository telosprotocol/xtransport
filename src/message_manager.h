// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <mutex>

#include "xbase/xpacket.h"
#include "xpbase/base/top_utils.h"
#include "xtransport/proto/transport.pb.h"
#include "xtransport/message_manager/message_manager_intf.h"

namespace top {

namespace transport {

class MessageManager : public MessageManagerIntf {
public:
    void RegisterMessageProcessor(uint32_t message_type, HandlerProc callback);
    void UnRegisterMessageProcessor(uint32_t message_type);
    void HandleMessage(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);

    MessageManager();
    ~MessageManager();

private:
    HandlerProc*    m_msg_handlers[enum_xprotocol_type_app_max + 1];

    DISALLOW_COPY_AND_ASSIGN(MessageManager);
};

}  // namespace transport

}  // namespace top
