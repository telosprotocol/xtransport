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
#include "socket_intf.h"

using namespace top;
using namespace base;

namespace top {
namespace transport {

class MultiThreadHandler;
using Xip2Header = _xip2_header;

class UdpTransport : public Transport, public std::enable_shared_from_this<UdpTransport>  {
public:
    UdpTransport();
    virtual ~UdpTransport() override;
    virtual int Start(
            const std::string& local_ip,
            uint16_t local_port,
            MultiThreadHandler* message_handler) override;
    virtual void Stop() override;
    virtual int SendData(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) override;
    virtual int SendData(base::xpacket_t& packet) override;
    virtual int SendDataWithProp(
            base::xpacket_t& packet,
            UdpPropertyPtr& udp_property) override;
    virtual int SendToLocal(base::xpacket_t& packet) override;
    virtual int SendToLocal(const xbyte_buffer_t& data) override;

    virtual int ReStartServer() override;
    virtual int32_t get_handle() override{ return static_cast<int32_t>(udp_handle_); }
    virtual int get_socket_status() override;
    virtual std::string local_ip() override{
        return local_ip_;
    }
    virtual uint16_t local_port() override{
        return local_port_;
    }

    virtual void register_on_receive_callback(on_receive_callback_t callback) override;
    virtual void unregister_on_receive_callback() override;

    virtual int SendPing(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) override;
    virtual int SendPing(base::xpacket_t& packet) override;
	virtual int RegisterOfflineCallback(std::function<void(const std::string& ip, const uint16_t port)> cb) override;

private:
    void SetOptBuffer();

private:
    base::xiothread_t* io_thread_;
    SocketIntf* udp_socket_;
    std::string local_ip_;
    uint16_t local_port_;
    bool socket_connected_;
    xfd_handle_t udp_handle_;
    MultiThreadHandler* message_handler_;
    std::mutex restart_xbase_udp_server_mutex_;

    DISALLOW_COPY_AND_ASSIGN(UdpTransport);
};

typedef std::shared_ptr<UdpTransport> UdpTransportPtr;

}  // namespace transport
}  // namespace top
