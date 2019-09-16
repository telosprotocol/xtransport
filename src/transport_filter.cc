// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtransport/udp_transport/transport_filter.h"

#include <cassert>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_timer.h"
#include "xpbase/base/kad_key/kadmlia_key.h"

namespace top {

namespace transport {

TransportFilter* TransportFilter::Instance() {
    static TransportFilter ins;
    return &ins;
}

bool TransportFilter::Init() {
    if (inited_) {
        return true;
    }
#ifndef NDEBUG
    print_timer_ = std::make_shared<base::TimerRepeated>(base::TimerManager::Instance(), "TransportFilter::Print");
    print_timer_->Start(
            500ll * 1000ll,
            kPrintPeriod,
            std::bind(&TransportFilter::Print, this));
#endif

    inited_ = true;
    return true;
}

TransportFilter::TransportFilter() {}

TransportFilter::~TransportFilter() {
    print_timer_->Join();
    print_timer_ = nullptr;
}

void TransportFilter::Print() {
#ifndef NDEBUG
    static ArrayInfo last_aryinfo;
    for (auto i = 0; i < aryinfo_.size(); ++i) {
        if (aryinfo_[i].send_packet == 0 && aryinfo_[i].recv_packet == 0) {
            continue;
        }
        float time_step = static_cast<float>(kPrintPeriod / 1000.0 / 1000.0);

        uint32_t send_packet = aryinfo_[i].send_packet;
        uint32_t send_band    = aryinfo_[i].send_band;
        uint32_t recv_packet = aryinfo_[i].recv_packet;
        uint32_t recv_band   = aryinfo_[i].recv_band;

        auto send_packet_step  = (send_packet - last_aryinfo[i].send_packet)  / time_step;
        auto send_band_step    = (send_band - last_aryinfo[i].send_band) / time_step;
        auto recv_packet_step  = (recv_packet - last_aryinfo[i].recv_packet) / time_step;
        auto recv_band_step    = (recv_band - last_aryinfo[i].recv_band) / time_step;
        TOP_INFO("transportfilter: type:%d send_packet:%d recv_packet:%d send_band:%d recv_band:%d"
                " ##send_packet_step:%.2f recv_packet_step:%.2f send_band_step:%.2f recv_band_step:%.2f",
                i,
                send_packet,
                recv_packet,
                send_band,
                recv_band,
                send_packet_step,
                recv_packet_step,
                send_band_step,
                recv_band_step);

        last_aryinfo[i].send_packet = send_packet;
        last_aryinfo[i].recv_packet = recv_packet;
        last_aryinfo[i].send_band = send_band;
        last_aryinfo[i].recv_band = recv_band;
    } // end for
#endif
}

bool TransportFilter::AddData(bool send, uint32_t type, uint32_t size) {
#ifndef NDEBUG
    if (type >= MsgTypeMaxSize) {
        TOP_WARN("type:%d invalid, size:%d failed", type, size);
        return false;
    }
    if (send) {
        aryinfo_[type].send_packet += 1;
        aryinfo_[type].send_band += size;
    } else {
        aryinfo_[type].recv_packet += 1;
        aryinfo_[type].recv_band += size;
    }
#endif
    return true;
}

} // end namespace transport 

} // end namespace top
