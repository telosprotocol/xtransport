// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <array>

#include "xtransport/proto/transport.pb.h"

namespace top {

namespace base {
class TimerRepeated;
}

namespace transport {

static const uint64_t kPrintPeriod = 20ll * 1000ll * 1000ll; // 20 seconds

typedef struct TransportInfo {
public:
    std::atomic<uint32_t> send_packet {0};
    std::atomic<uint32_t> recv_packet {0};
    std::atomic<uint32_t> send_band {0};
    std::atomic<uint32_t> recv_band {0};
} TransportInfo;

static const int32_t MsgTypeMaxSize = 4096;
using ArrayInfo = std::array<TransportInfo, MsgTypeMaxSize>;


class TransportFilter {
public:
    static TransportFilter* Instance();

    bool Init();
    //bool FilterMessage(transport::protobuf::RoutingMessage& message);

public:
    bool AddData(bool send, uint32_t type, uint32_t size);
    void do_clear_and_reset();
    void Print();

private:
    TransportFilter();
    ~TransportFilter();

private:
    bool inited_{false};
    std::shared_ptr<base::TimerRepeated> print_timer_{nullptr};
    ArrayInfo aryinfo_;
};

}

}
