// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtransport/message_manager/multi_message_handler.h"

#include <iostream>
#include <atomic>

#include "xbase/xutl.h"
#include "xpbase/base/rand_util.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_string_util.h"
#include "xtransport/message_manager/message_manager_intf.h"
#include "xmetrics/xmetrics.h"
#include "xpbase/base/top_utils.h"

namespace top {

namespace transport {


ThreadHandler::ThreadHandler(base::xiothread_t * raw_thread_ptr, const uint32_t raw_thread_index)
    :base::xiobject_t(base::xcontext_t::instance(),raw_thread_ptr->get_thread_id(),base::enum_xobject_type_thread)
{
    xassert(raw_thread_ptr != NULL);
    raw_thread_ptr->add_ref(); //must be valid
    m_raw_thread = raw_thread_ptr;
    raw_thread_index_ = raw_thread_index;
    
    //create_databox(-1, -1, 65535); //create dedicate databox for packet
    create_databox(2048, 10240, 131070); //create dedicate databox for packet
    TOP_INFO("m_ptr_databox:%p",get_databox());
}

    
ThreadHandler::~ThreadHandler()
{
    TOP_INFO("ThreadHandler destroy");
    get_databox()->close(false);
    m_raw_thread->close();
    m_raw_thread->release_ref();
}

//packet is from   send(xpacket_t & packet) or dispatch(xpacket_t & packet) of xdatabox_t
//subclass need overwrite this virtual function if they need support signal(xpacket_t) or send(xpacket_t),only allow called internally
bool  ThreadHandler::on_databox_open(base::xpacket_t & packet,int32_t cur_thread_id, uint64_t time_now_ms)
{
    transport::protobuf::RoutingMessage pro_message;
    if (!pro_message.ParseFromArray((const char*)packet.get_body().data() + enum_xip2_header_len,packet.get_body().size() - enum_xip2_header_len))
    {
        TOP_ERROR("Message ParseFromString from string failed!");
        return true;
    }
    pro_message.set_hop_num(pro_message.hop_num() + 1);

    if (pro_message.hop_num() > 10) {
        TOP_WARN2("transport_hop_num_error(%d), msg.type(%d), is_broadcast(%d), msg.id(%d), is_root(%d)",
                pro_message.hop_num(),
                pro_message.type(),
                (pro_message.has_broadcast() && pro_message.broadcast()),
                pro_message.id(),
                pro_message.is_root());
    }
    if (callback_) {
        callback_(pro_message, packet);
    } else {
        message_manager_->HandleMessage(pro_message, packet);
    }
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE2("transport handled", pro_message,cur_thread_id);
    TOP_INFO("ThreadHandler filter type:%d thread_index:%d,hop:%d size:%d",
            pro_message.type(),
            raw_thread_index_,
            pro_message.hop_num(),
            packet.get_size());
    return true;
}

void ThreadHandler::register_on_dispatch_callback(on_dispatch_callback_t callback) {
    std::unique_lock<std::mutex> lock(callback_mutex_);
    assert(callback_ == nullptr);
    callback_ = callback;
}

void ThreadHandler::unregister_on_dispatch_callback() {
    std::unique_lock<std::mutex> lock(callback_mutex_);
    callback_ = nullptr;
}
 
MultiThreadHandler::MultiThreadHandler()
{
}

void MultiThreadHandler::Init()
{
    m_worker_threads.resize(m_woker_threads_count);
    for(size_t i = 0; i < m_woker_threads_count; ++i)
    {
        base::xiothread_t * raw_thread_ptr = base::xiothread_t::create_thread(base::xcontext_t::instance(),base::xiothread_t::enum_xthread_type_private,-1);
        m_worker_threads[i] = new ThreadHandler(raw_thread_ptr, i);

        TOP_INFO("starting thread(ThreadHandler)-index:%d and thread_id:%d", (int)i,raw_thread_ptr->get_thread_id());
    }
}

MultiThreadHandler::~MultiThreadHandler()
{
    Stop();
}

void MultiThreadHandler::Stop() {
    for(auto& thread_handler : m_worker_threads)
    {
        if(thread_handler != nullptr)
        {
            thread_handler->close();
            thread_handler->release_ref();
        }
        TOP_INFO("thread(ThreadHandler) stopped");
    }
    m_worker_threads.clear();
}

void MultiThreadHandler::register_on_dispatch_callback(on_dispatch_callback_t callback) {
    for (auto& th : m_worker_threads) {
        th->register_on_dispatch_callback(callback);
    }
}

void MultiThreadHandler::unregister_on_dispatch_callback() {
    for (auto& th : m_worker_threads) {
        th->unregister_on_dispatch_callback();
    }
}

void MultiThreadHandler::HandleMessage(base::xpacket_t & packet)
{
    if((size_t)packet.get_body().size() < enum_xip2_header_len) //filter empty packet
    {
        TOP_WARN("HandleMessage Recv invalid packet with size: %d, from:%s:%d", packet.get_body().size(),packet.get_from_ip_addr().c_str(), packet.get_from_ip_port());
        return;
    }
    else
    {
        #ifdef DEBUG
        TOP_DEBUG("HandleMessage Recv size: %d, from:%s:%d", packet.get_body().size(),packet.get_from_ip_addr().c_str(), packet.get_from_ip_port());
        #endif
    }
 
    uint32_t index = 0;
    if(m_worker_threads.size() == 1u) //optimize,direct post
    {
        m_worker_threads[0]->get_databox()->send_packet(packet);
        index = 0;
	}
    else
    {
        _xip2_header * _raw_head = (_xip2_header*)packet.get_body().data();//incoming packet store data at body always
        const int priority_level = get_xpacket_priority_type(_raw_head->flags);
        if(priority_level >= enum_xpacket_priority_type_flash) //priority packet
        {
            m_worker_threads[0]->get_databox()->send_packet(packet);
            index = 0;
        }
        else if(m_worker_threads.size() == 2u)
        {
            m_worker_threads[1]->get_databox()->send_packet(packet);
            index = 1;
        }
        else
        {
            uint32_t msg_hash = 0;
            if(0 == _raw_head->sesssion_id)
            {
                //route packet based on source ip address, which may ensure  order of packet from same source
                const std::string src_ip_address = packet.get_from_ip_rawaddr();
                msg_hash = base::xhash32_t::digest(src_ip_address);
            }
            else //dispatch based on session_id if have
            {
                msg_hash = _raw_head->sesssion_id;
            }
            const uint32_t th_index = (msg_hash % (m_worker_threads.size() - 1)) + 1;//thread 0 is reserved for priority packets
            m_worker_threads[th_index]->get_databox()->send_packet(packet);
            index = th_index;
        }
    } // end  if(enum_const_woker_threads_count == 1) //optimize,direct post

    #ifdef DEBUG
    static std::atomic<uint32_t> packet_count(0);
    ++packet_count;
    if (packet_count % 64 == 0) {
        int64_t in = 0;
        int64_t out = 0;
        const int32_t total_holding = m_worker_threads[index]->get_databox()->count_packets(in, out);
        if(total_holding > 8192)//too much packets pending in the queues
        {
            TOP_WARN("TOO MUCH PENDING,packet_count: thread_index:%d,thread_id:%d, packets(in:%lld out:%lld hold:%d)", index, m_worker_threads[index]->get_thread_id(), in, out, total_holding);
        }
        else if(packet_count % 1024 == 0)
        {
            TOP_INFO("packet_count: thread_index:%d,thread_id:%d, packets(in:%lld out:%lld hold:%d)", index, m_worker_threads[index]->get_thread_id(), in, out, total_holding);
        }
    }
    #endif
}

}  // namespace transport

}  // namespace top

