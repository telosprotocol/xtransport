// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "message_manager.h"

#include <cassert>

namespace top {

namespace transport {

MessageManager::MessageManager()
{
    memset(m_msg_handlers,0,sizeof(m_msg_handlers));
}

MessageManager::~MessageManager()
{
    for(int i = 0; i < enum_xprotocol_type_app_max; ++i)
    {
        HandlerProc * handler_ptr = m_msg_handlers[i];
        m_msg_handlers[i] = NULL;
        if(handler_ptr != NULL)
            delete handler_ptr;
    }
}

MessageManagerIntf* MessageManagerIntf::Instance() {
    static MessageManager ins;
    return &ins;
}

void MessageManager::RegisterMessageProcessor(uint32_t message_type, HandlerProc callback) {
 
    if(message_type > enum_xprotocol_type_app_max)
    {
        xerror("MessageManager::RegisterMessageProcessor,invalid message type(%d)",message_type);
        #ifdef DEBUG
        xabort("fatal error,force to quit");
        #endif
        return;
    }
    HandlerProc * old_ptr = m_msg_handlers[message_type];
    xassert(NULL == old_ptr);
    if(NULL != old_ptr)
    {
        #ifdef DEBUG
        xerror("MessageManager::RegisterMessageProcessor,messagetype(%d) already registered",message_type);
        xabort("fatal error,force to quit");
        return;
        #else
        xwarn("MessageManager::RegisterMessageProcessor,messagetype(%d) already registered",message_type);
        delete old_ptr;
        #endif        
    }
    m_msg_handlers[message_type] = new HandlerProc(callback);
}

void MessageManager::UnRegisterMessageProcessor(uint32_t message_type) {
 
    if(message_type >= enum_xprotocol_type_app_max)
    {
        xerror("MessageManager::UnRegisterMessageProcessor,invalid message type(%d)",message_type);
        #ifdef DEBUG
        xabort("fatal error,force to quit");
        #endif
        return;
    }
    HandlerProc * target_ptr = m_msg_handlers[message_type];
    m_msg_handlers[message_type] = NULL;
    if(target_ptr != NULL)
    {
        delete target_ptr;
    }
}

void MessageManager::HandleMessage(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
 
    const int32_t message_type = message.type();
    if(message_type >= enum_xprotocol_type_app_max)
    {
        xwarn("MessageManager::HandleMessage,invalid message type(%d)",message_type);
        return;
    }
    HandlerProc * target_ptr = m_msg_handlers[message_type];
    if(target_ptr != NULL)
    {
        (*target_ptr)(message,packet);
    }
    else
    {
        xwarn("MessageManager::HandleMessage,empty callback for message type(%d)",message_type);
        #ifdef DEBUG
//         xassert(0);//logic exception,throw error  Charlie(for dynamic message)
        #endif
    }
}

}  // namespace transport

}  // namespace top
