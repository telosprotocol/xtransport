// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtransport/udp_transport/raw_udp_socket.h"

#include <stdio.h>
#include <mutex>
#include <iostream>

#include "xbase/xcontext.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_log.h"
#include "xtransport/utils/transport_utils.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xmetrics/xmetrics.h"

namespace top {

namespace transport {

RawUdpSocket::RawUdpSocket(
        base::xcontext_t & _context,
        int32_t target_thread_id,
        xfd_handle_t native_handle,
        MultiThreadHandler* message_handler)
        : base::udp_t(_context, NULL, target_thread_id, native_handle),
          multi_thread_message_handler_(message_handler) {}

RawUdpSocket::~RawUdpSocket() {
    // Call to virtual function during destruction will not dispatch to derived class
    // Stop();
}

int32_t RawUdpSocket::recv(
        uint64_t from_xip_addr_low,
        uint64_t from_xip_addr_high,
        uint64_t to_xip_addr_low,
        uint64_t to_xip_addr_high,
        base::xpacket_t & packet,
        int32_t cur_thread_id,
        uint64_t timenow_ms,
        xendpoint_t* from_child_end) {
    if ((size_t)packet.get_size() <= enum_xip2_header_len) {
        TOP_ERROR("<blueshi> packet.size(%d) invalid", (int)packet.get_size());
        return 0;
    }

    multi_thread_message_handler_->HandleMessage(packet);
    return 0;
}

void RawUdpSocket::register_on_receive_callback(on_receive_callback_t callback) {
    std::unique_lock<std::mutex> lock(callback_mutex_);
    assert(callback_ == nullptr);
    callback_ = callback;
}

void RawUdpSocket::unregister_on_receive_callback() {
    std::unique_lock<std::mutex> lock(callback_mutex_);
    callback_ = nullptr;
}

void RawUdpSocket::Stop() {}

int RawUdpSocket::SendData(
        const xbyte_buffer_t& data,
        const std::string& peer_ip,
        uint16_t peer_port) {
    uint8_t local_buf[kUdpPacketBufferSize];
    base::xpacket_t packet(base::xcontext_t::instance(), local_buf, sizeof(local_buf), 0,0, false);
    packet.get_body().push_back((uint8_t*)data.data(), data.size());  // NOLINT
    packet.set_to_ip_addr(peer_ip);
    packet.set_to_ip_port(peer_port);
    AddXip2Header(packet);
    return SendData(packet);
}

int RawUdpSocket::SendToLocal(base::xpacket_t& packet) {
    if ((size_t)packet.get_size() <= enum_xip2_header_len) {
        TOP_ERROR("<blueshi> packet.size(%d) invalid", (int)packet.get_size());
        return kTransportFailed;
    }
    multi_thread_message_handler_->HandleMessage(packet);
    return kTransportSuccess;
}

int RawUdpSocket::SendToLocal(const xbyte_buffer_t& data) {
    uint8_t local_buf[kUdpPacketBufferSize];
    base::xpacket_t packet(base::xcontext_t::instance(), local_buf, sizeof(local_buf), 0, 0,false);
    packet.get_body().push_back((uint8_t*)data.data(), data.size());  // NOLINT
    return kTransportSuccess;
}

int RawUdpSocket::SendData(base::xpacket_t& packet) {
    Xip2Header* xip2_header = ParserXip2Header(packet);
    if (!xip2_header) {
        return kTransportFailed;
    }

    TOP_DEBUG("sendto %d bytes to %s:%d", packet.get_size(), packet.get_to_ip_addr().c_str(), packet.get_to_ip_port());
    if (this->send(
            xip2_header->from_xaddr_low,
            xip2_header->from_xaddr_high,
            xip2_header->to_xaddr_low,
            xip2_header->to_xaddr_high,
            packet,
            0,
            0,
            NULL) != enum_xcode_successful) {
        TOP_ERROR("send xpacket failed!packet size is :%d\n",packet.get_size());
        return kTransportFailed;
    }

    XMETRICS_COUNTER_INCREMENT(metrics::xmetrics::e_transport_packet_send, 1);
    XMETRICS_COUNTER_INCREMENT(metrics::xmetrics::e_transport_bandwidth_send, packet.get_body().size());

    return kTransportSuccess;
}

int RawUdpSocket::SendDataWithProp(
        base::xpacket_t& packet,
        UdpPropertyPtr& udp_property) {
    return SendData(packet);
}

// parse xip2_header from packet
Xip2Header* RawUdpSocket::ParserXip2Header(base::xpacket_t& packet)
{
    if((size_t)packet.get_size() < enum_xip2_header_len) {
        TOP_WARN("xip2_header_len invalid, packet_size(%d) smaller than enum_xip2_header_len(%d)",
                packet.get_body().size(),
                enum_xip2_header_len);
        return nullptr;
    }
    if(packet.get_header().size() > 0)
        return (Xip2Header*)(packet.get_header().data());
    else
        return (Xip2Header*)(packet.get_body().data());
}


void RawUdpSocket::AddXip2Header(base::xpacket_t& packet) {
    _xip2_header header;
    memset(&header, 0, sizeof(header));  // TODO(blueshi): init header
    packet.get_body().push_front((uint8_t*)&header, enum_xip2_header_len);
}

bool RawUdpSocket::GetSocketStatus() {
    if (this->get_socket_status() != top::base::enum_socket_status_connected) {
        return false;
    }
    return true;
}

int RawUdpSocket::SendPing(
        const xbyte_buffer_t& data,
        const std::string& peer_ip,
        uint16_t peer_port) {
    uint8_t local_buf[kUdpPacketBufferSize];
    base::xpacket_t packet(base::xcontext_t::instance(), local_buf, sizeof(local_buf), 0,0, false);
    packet.get_body().push_back((uint8_t*)data.data(), data.size());  // NOLINT
    packet.set_to_ip_addr(peer_ip);
    packet.set_to_ip_port(peer_port);
    AddXip2Header(packet);
    return SendPing(packet);
}

int RawUdpSocket::SendPing(base::xpacket_t& packet) {
    return SendData(packet);
}

int RawUdpSocket::RegisterOfflineCallback(std::function<void(const std::string& ip, const uint16_t port)> cb) {
	return kTransportSuccess;
}

}  // namespace transport

}  // namespace top
