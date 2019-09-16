// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

namespace top {
namespace transport {

class UdpConfig {
public:
    static UdpConfig* Instance() {
        static UdpConfig ins;
        return &ins;
    }

    void UseXudp(bool use_xudp) {
        use_xudp_ = use_xudp;
    }

    bool UseXudp() {
        return use_xudp_;
    }

private:
    bool use_xudp_{true};
};

}  // namespace transport
}  // namespace top
