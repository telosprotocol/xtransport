// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <atomic>

#include "xtransport/udp_transport/socket_intf.h"
#include "xbase/xlog.h"
#include "xbase/xobject.h"
#include "xbase/xthread.h"
#include "xbase/xtimer.h"
#include "xbase/xdata.h"
#include "xbase/xpacket.h"
#include "xbase/xsocket.h"
#include "xbase/xutl.h"

#include "xtransport/transport.h"
#include "xpbase/base/xbyte_buffer.h"
#include "xpbase/base/top_utils.h"
#include "xkad/routing_table/node_info.h"

using namespace top;
using namespace base;

namespace top {
namespace kadmlia {
struct NodeInfo;
typedef std::shared_ptr<NodeInfo> NodeInfoPtr;
};

namespace transport {

class MultiThreadHandler;
class UdpTransport;
class SocketIntf;

enum class enum_xudp_status { 
    enum_xudp_init,
    enum_xudp_connecting,
    enum_xudp_connected,
    enum_xudp_closed,  // closed, not released
	enum_xudp_closed_released,  // closed, released
};

using Xip2Header = _xip2_header;

class xp2pudp_t;
class XudpSocket;

class UdpProperty {
public:
    ~UdpProperty();
public:
    xp2pudp_t* GetXudp() {
        return xudp_;
    }
    void SetXudp(xp2pudp_t* xudp_in);

private:
    xp2pudp_t* xudp_{nullptr};
};


class xp2pudp_t : public base::xudp_t
{
public:
    xp2pudp_t(xcontext_t & _context,xendpoint_t * parent,const int32_t target_thread_id,int64_t virtual_handle,xsocket_property & property, XudpSocket* listen_server)
     : xudp_t(_context,parent,target_thread_id,virtual_handle,property)
    {
        
        m_link_refcount = 0;
//      TOP_DBG_INFO("xp2pudp_t new,handle(%d),local address[%s : %d]-[%d : %d] vs  peer address[%s : %d]-[%d : %d]; cur_thread_id=%d,object_id(%lld) for this(%lld)\n",get_handle(), get_local_ip_address().c_str(),get_local_real_port(),get_local_logic_port(),get_local_logic_port_token(),get_peer_ip_address().c_str(),get_peer_real_port(),get_peer_logic_port(),get_peer_logic_port_token(),get_thread_id(),get_obj_id(),(uint64_t)this);
//        num = 0;
        status = top::transport::enum_xudp_status::enum_xudp_init;
        listen_server_ = listen_server;
    }
protected:
    virtual ~xp2pudp_t()
    {
        xassert(m_link_refcount == 0);
    };
private:
    xp2pudp_t();
    xp2pudp_t(const xp2pudp_t &);
    xp2pudp_t & operator = (const xp2pudp_t &);
protected:
    //notify the child endpont is ready to use when receive on_endpoint_open of error_code = enum_xcode_successful
    virtual bool             on_endpoint_open(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xendpoint_t* from_child) override;
    //when associated io-object close happen,post the event to receiver
    //error_code is 0 when it is closed by caller/upper layer
    virtual bool             on_endpoint_close(const int32_t error_code,const int32_t cur_thread_id,const uint64_t timenow_ms,xendpoint_t* from_child) override;
    //notify upper layer,child endpoint update keeplive status
    virtual bool             on_endpoint_keepalive(const std::string & _payload,const int32_t cur_thread_id,const uint64_t timenow_ms,xendpoint_t* from_child) override;
protected:

    //receive data packet that already trim header of link layer
    virtual int32_t recv(uint64_t from_xip_addr_low,uint64_t from_xip_addr_high,uint64_t to_xip_addr_low,uint64_t to_xip_addr_high,xpacket_t & packet,int32_t cur_thread_id,uint64_t timenow_ms,xendpoint_t* from_child_end) override;
public:
    int send(xpacket_t& packet);
    int32_t connect_xudp(const std::string& target_ip,const uint16_t target_port, XudpSocket* udp_server);
    enum_xudp_status GetStatus() {return status;}
    void SetStatus(enum_xudp_status s) { status = s;}
    
    int    add_linkrefcount();
    int    release_linkrefcount();
protected:
    std::atomic<int>  m_link_refcount;  // indicate how many routing table linked this socket
private:
    enum_xudp_status status;  // 0 init, 1 connecting, 2 connected, 3 closed
    XudpSocket* listen_server_;
};

class XudpSocket : protected base::xudplisten_t, public SocketIntf {
public:
    XudpSocket(
            base::xcontext_t& context,
            int32_t target_thread_id,
            xfd_handle_t native_handle,
            MultiThreadHandler* message_handler);
    virtual ~XudpSocket() override;
    void Stop() override;
    int SendData(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) override;
    int SendData(base::xpacket_t& packet) override;
    int SendDataWithProp(
            base::xpacket_t& packet,
            UdpPropertyPtr& udp_property);
    int SendToLocal(base::xpacket_t& packet) override;
    int SendToLocal(const xbyte_buffer_t& data) override;
    void AddXip2Header(base::xpacket_t& packet) override;
    bool GetSocketStatus() override;

    void register_on_receive_callback(on_receive_callback_t callback) override;
    void unregister_on_receive_callback() override;

    int AddXudp(const std::string& ip_port, xp2pudp_t* xudp);
    bool CloseXudp(xp2pudp_t* xudp);

    virtual void StartRead() override {
        base::xudplisten_t::start_read(0);
    }

    virtual void Close() override {
        // TODO(blueshi): how to close once?
        // auto ref1 = base::xudplisten_t::get_refcount();
        // if (ref1 > 1) {
            base::xudplisten_t::close();
        // }

        auto future = future_;
        future.get();  // block wait until closed

        // TODO(blueshi): how to release?
        // while (base::xudplisten_t::get_refcount() > 0) {
        //     base::xudplisten_t::release_ref();
        // }
    }

    virtual uint16_t GetLocalPort() override {
        return base::xudplisten_t::get_local_real_port();
    }

    virtual std::string GetLocalIp() override {
        return base::xudplisten_t::get_local_ip_address();
    }

    virtual int SendPing(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) override;
    virtual int SendPing(base::xpacket_t& packet) override;
	virtual int RegisterOfflineCallback(std::function<void(const std::string& ip, const uint16_t port)> cb) override;

protected:
    virtual xslsocket_t* create_xslsocket(xendpoint_t * parent,xfd_handle_t handle,xsocket_property & property, int32_t cur_thread_id,uint64_t timenow_ms) override;
    Xip2Header* ParserXip2Header(base::xpacket_t& packet);
    int send_ping_packet(
            std::string target_ip_addr,
            uint16_t target_ip_port,
            uint16_t target_logic_port,
            uint16_t target_logic_port_token,
            const std::string & _payload,
            uint16_t TTL);
    virtual int32_t on_ping_packet_recv(
            base::xpacket_t& packet_raw,
            int32_t cur_thread_id,
            uint64_t timenow_ms,
            xendpoint_t* from_child_end) override;
    virtual bool on_object_close() override {
        auto ret = base::xudplisten_t::on_object_close();
        promise_.set_value();  // wake block
        return ret;
    }

private:
    XudpSocket();
    XudpSocket(const XudpSocket &);
    XudpSocket & operator = (const XudpSocket &);

private:
    int32_t     m_xudpsocket_mgr_thread_id;    //dedicated thread to manage xudpt socket (include keepalive,lifemanagent)
private:
    on_receive_callback_t callback_;
    std::mutex callback_mutex_;
    std::promise<void> promise_;
    std::shared_future<void> future_{promise_.get_future()};

    std::map<std::string, xp2pudp_t*> xudp_client_map;  // ip+port, xp2pudp_t*
    std::recursive_mutex xudp_mutex_;
public:
    MultiThreadHandler* multi_thread_message_handler_;
	std::function<void(const std::string& ip, const uint16_t port)> m_offline_cb;
};

}  // namespace transport
}  // namespace top
