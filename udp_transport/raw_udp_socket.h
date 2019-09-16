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
#include <future>

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

namespace top {
namespace transport {

class MultiThreadHandler;
class UdpTransport;

using Xip2Header = _xip2_header;

class RawUdpSocket : protected base::udp_t, public SocketIntf {
public:
    RawUdpSocket(
            base::xcontext_t& context,
            int32_t target_thread_id,
            xfd_handle_t native_handle,
            MultiThreadHandler* message_handler);
    virtual ~RawUdpSocket() override;
    void Stop() override;
    int SendData(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) override;
    int SendData(base::xpacket_t& packet) override;
    int SendDataWithProp(
            base::xpacket_t& packet,
            UdpPropertyPtr& udp_property) override;
    int SendToLocal(base::xpacket_t& packet) override;
    int SendToLocal(const xbyte_buffer_t& data) override;
    void AddXip2Header(base::xpacket_t& packet) override;
    bool GetSocketStatus() override;
    void register_on_receive_callback(on_receive_callback_t callback) override;
    void unregister_on_receive_callback() override;
	virtual int RegisterOfflineCallback(std::function<void(const std::string& ip, const uint16_t port)> cb) override;

    virtual void StartRead() override {
        base::udp_t::start_read(0);
    }

    virtual void Close() override {
        // TODO(blueshi): how to close once?
        // if (base::udp_t::get_refcount() > 1) {
            base::udp_t::close();
        // }

        auto future = future_;
        future.get();  // block wait until closed

        // TODO(blueshi): how to assume?
        // assert(base::udp_t::get_refcount() == 1);

        // TODO(blueshi): how to release?
        // while (base::udp_t::get_refcount() > 0) {
        //     base::udp_t::release_ref();
        // }
    }

    virtual uint16_t GetLocalPort() override {
        return base::udp_t::get_local_real_port();
    }

    virtual std::string GetLocalIp() override {
        return base::udp_t::get_local_ip_address();
    }

    virtual int SendPing(
            const xbyte_buffer_t& data,
            const std::string& peer_ip,
            uint16_t peer_port) override;
    virtual int SendPing(base::xpacket_t& packet) override;

protected:
    virtual int32_t recv(
            uint64_t from_xip_addr_low,
            uint64_t from_xip_addr_high,
            uint64_t to_xip_addr_low,
            uint64_t to_xip_addr_high,
            base::xpacket_t & packet,
            int32_t cur_thread_id,
            uint64_t timenow_ms,
            xendpoint_t* from_child_end) override;
    virtual bool on_object_close() override {
        auto ret = base::udp_t::on_object_close();
        promise_.set_value();  // wake block
        return ret;
    }
    Xip2Header* ParserXip2Header(base::xpacket_t& packet);

private:
    MultiThreadHandler* multi_thread_message_handler_;

    std::mutex callback_mutex_;
    on_receive_callback_t callback_;
    std::promise<void> promise_;
    std::shared_future<void> future_{promise_.get_future()};

    DISALLOW_COPY_AND_ASSIGN(RawUdpSocket);
};


}  // namespace transport
}  // namespace top
