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

#include "xpbase/base/xbyte_buffer.h"
#include "transport_fwd.h"

namespace top {
namespace transport {

using on_receive_callback_t =
    std::function<std::uint32_t(
            uint64_t,
            uint64_t,
            uint64_t,
            uint64_t,
            base::xpacket_t&,
            int32_t,
            uint64_t,
            base::xendpoint_t*)>;

class MultiThreadHandler;

class Transport {
public:
    virtual int Start(
            const std::string& local_ip,
            uint16_t local_port,
            MultiThreadHandler* message_handler) = 0;
    virtual void Stop() = 0;
    virtual int SendData(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) = 0;
    virtual int SendData(base::xpacket_t& packet) = 0;
    virtual int SendDataWithProp(
            base::xpacket_t& packet,
            UdpPropertyPtr& udp_property) = 0;
    virtual int SendToLocal(base::xpacket_t& packet) = 0;
    virtual int SendToLocal(const xbyte_buffer_t& data) = 0;

    virtual int ReStartServer() = 0;
    virtual int32_t get_handle() = 0;
    virtual int get_socket_status() = 0;
    virtual std::string local_ip() = 0;
    virtual uint16_t local_port() = 0;

    virtual void register_on_receive_callback(on_receive_callback_t callback) = 0;
    virtual void unregister_on_receive_callback() = 0;

    virtual int SendPing(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) = 0;
    virtual int SendPing(base::xpacket_t& packet) = 0;
	virtual int RegisterOfflineCallback(std::function<void(const std::string& ip, const uint16_t port)> cb) = 0;

protected:
    Transport() {}
    virtual ~Transport() {}
};

typedef std::shared_ptr<Transport> TransportPtr;

}  // namespace transport

}  // namespace top
