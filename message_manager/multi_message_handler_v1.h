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
#include "xtransport/proto/transport.pb.h"
#include "xtransport/utils/transport_utils.h"

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

class ThreadHandler {
public:
    ThreadHandler(
            std::shared_ptr<MultiThreadHandler> base_handler,
            std::mutex& mutex,
            std::condition_variable& cond_var);
    ~ThreadHandler();
    void Join();
    void register_on_dispatch_callback(on_dispatch_callback_t callback);
    void unregister_on_dispatch_callback();

private:
    void HandleMessage();
    std::shared_ptr<std::thread> message_handle_thread_ptr_;
    std::mutex& mutex_;
    std::condition_variable& cond_var_;
    bool destroy_;
    std::shared_ptr<MultiThreadHandler> base_handler_;
    std::mutex callback_mutex_;
    on_dispatch_callback_t callback_;
};

typedef std::shared_ptr<ThreadHandler> ThreadHandlerPtr;

class MultiThreadHandler : public std::enable_shared_from_this<MultiThreadHandler> {
public:
    MultiThreadHandler();
    ~MultiThreadHandler();
    void Init();
    void HandleMessage(base::xpacket_t& packet);
    std::shared_ptr<QueueObject> GetMessageFromQueue();

    void register_on_dispatch_callback(on_dispatch_callback_t callback);
    void unregister_on_dispatch_callback();

private:
    void Join();

    static const int kQueueObjectCount = 1024 * 1024;

    std::map<uint32_t, std::queue<std::shared_ptr<QueueObject>>> priority_queue_map_;
    std::mutex priority_queue_map_mutex_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ThreadHandlerPtr> thread_vec_;
    bool inited_{ false };
    std::mutex inited_mutex_;
};

}  // namespace transport

}  // namespace top
