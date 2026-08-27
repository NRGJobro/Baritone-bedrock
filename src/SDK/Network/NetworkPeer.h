#pragma once

class NetworkPeer {
public:
    using PacketRecvTimepoint = std::chrono::steady_clock::time_point;

    enum class Reliability : int {
        Reliable,
        ReliableOrdered,
        Unreliable,
        UnreliableSequenced
    };
};
