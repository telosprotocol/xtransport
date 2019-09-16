//
//  xbase_thread_message_handler.h
//
//  Created by Charlie Xie 01/23/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "xbase/xpacket.h"
#include "xbase/xthread.h"
#include "xtransport/proto/transport.pb.h"
#include "xtransport/utils/transport_utils.h"
#include "xtransport/message_manager/message_manager_intf.h"

namespace top {

namespace transport {
using on_dispatch_callback_t = std::function<void(transport::protobuf::RoutingMessage& message, base::xpacket_t&)>;

class MessageHandler;
class MultiThreadHandler;

struct QueueObject {
    QueueObject(
            std::shared_ptr<transport::protobuf::RoutingMessage> msg_ptr,
            std::shared_ptr<base::xpacket_t> pkt_ptr)
            : message_ptr(msg_ptr), packet_ptr(pkt_ptr) {}
    std::shared_ptr<transport::protobuf::RoutingMessage> message_ptr;
    std::shared_ptr<base::xpacket_t> packet_ptr;
};

class ThreadHandler : public base::xiobject_t
{
public:
    explicit ThreadHandler(base::xiothread_t * raw_thread_ptr, const uint32_t raw_thread_index);
    virtual ~ThreadHandler();
private:
    ThreadHandler();
    ThreadHandler(const ThreadHandler & );
    ThreadHandler & operator = (const ThreadHandler &);
public:
    base::xiothread_t*   get_raw_thread() {return m_raw_thread;}
 
    //packet is from   send(xpacket_t & packet) or dispatch(xpacket_t & packet) of xdatabox_t
    //subclass need overwrite this virtual function if they need support signal(xpacket_t) or send(xpacket_t),only allow called internally
    virtual  bool        on_databox_open(base::xpacket_t & packet,int32_t cur_thread_id, uint64_t time_now_ms) override;

    void register_on_dispatch_callback(on_dispatch_callback_t callback);
    void unregister_on_dispatch_callback();

private:
    base::xiothread_t *  m_raw_thread;
    uint32_t raw_thread_index_;
    std::mutex callback_mutex_;
    on_dispatch_callback_t callback_;
    transport::MessageManagerIntf* message_manager_{transport::MessageManagerIntf::Instance()};
};

class MultiThreadHandler : public std::enable_shared_from_this<MultiThreadHandler>
{
public:
    MultiThreadHandler();
    ~MultiThreadHandler();

    void Init();
    void Stop();
    void HandleMessage(base::xpacket_t& packet);

    void register_on_dispatch_callback(on_dispatch_callback_t callback);
    void unregister_on_dispatch_callback();

private:
    size_t m_woker_threads_count{4};  // usally 1 worker thread to handle packet
    std::vector<ThreadHandler*> m_worker_threads;
};

}  // namespace transport

}  // namespace top

