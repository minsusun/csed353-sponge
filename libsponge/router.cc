#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    // Add new routing table entry
    this->_routingTable.push_back(RoutingTableEntry{route_prefix,
                                                    prefix_length,
                                                    prefix_length == 0 ? 0 : ~uint32_t(0) << (32 - prefix_length),
                                                    next_hop,
                                                    interface_num});
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    auto &header = dgram.header();

    // Drop datagram when ttl will or does hit zero
    if (header.ttl <= 1)
        return;

    const auto& dst = header.dst;

    // Longest-prefix-match Route on iterator target
    auto target = _routingTable.end();
    for (auto it = _routingTable.begin(); it < _routingTable.end(); it++) {
        if ((dst & it->_prefix_mask) == it->_route_prefix &&
            (target > it || target->_prefix_length < it->_prefix_length))
            target = it;
    }

    // When no routes matched
    if (target == _routingTable.end())
        return;

    // Decrease ttl
    header.ttl -= 1;

    // Send datagram on interface with next hop
    const auto &next_hop = target->_next_hop;
    const auto &interface_num = target->_interface_num;
    if (next_hop.has_value())
        _interfaces[interface_num].send_datagram(dgram, next_hop.value());
    else
        _interfaces[interface_num].send_datagram(dgram, Address::from_ipv4_numeric(dst));
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
