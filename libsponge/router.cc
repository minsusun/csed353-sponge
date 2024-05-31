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
void DUMMY_CODE(Targs &&... /* unused */) {}

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

    // fill in the entry with appropriate values and push it to the forwarding table
    // ForwardingTableEntry fte;
    // fte.route_prefix = route_prefix;
    // fte.prefix_length = prefix_length;
    // fte.next_hop = next_hop;
    // fte.interface_num = interface_num;
    ForwardingTableEntry fte(route_prefix, prefix_length, next_hop, interface_num);
    forwarding_table.push_back(fte);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    uint8_t longest_prefix_length = 0;
    int longest_match_idx = -1;
    int table_size = forwarding_table.size();

    // 1. The "match"
    for (int i = 0; i < table_size; i++) {
        // mask the prefix of each fte and compare with the masked destination address
        uint32_t mask = forwarding_table[i].get_mask();
        if ((forwarding_table[i].get_route_prefix() & mask) == (dgram.header().dst & mask)) {
            // if prefix match, compare the length with existing longest-match and update
            if ((forwarding_table[i].get_prefix_length() > longest_prefix_length) || longest_match_idx == -1) {
                longest_match_idx = i;
                longest_prefix_length = forwarding_table[i].get_prefix_length();
            }
        }
    }

    // if no match, drop the datagram (dropping done by route()'s pop on queue)
    if (longest_match_idx == -1)
        return;

    // if TTL was zero already or hits zero after the decrement, drop the datagram
    if (dgram.header().ttl <= 1)
        return;

    // decrement ttl
    dgram.header().ttl--;

    // 2. The "action"
    ForwardingTableEntry matchingEntry = forwarding_table[longest_match_idx];
    // if next_hop has value, move to next router by following the path
    if (matchingEntry.get_next_hop().has_value()) {
        Address next_hop_addr = matchingEntry.get_next_hop().value();
        interface(matchingEntry.get_interface_num()).send_datagram(dgram, next_hop_addr);
    }
    // if next_hop is an empty optional, it means directly connected to destination address
    else {
        Address next_hop_addr = Address::from_ipv4_numeric(dgram.header().dst);
        interface(matchingEntry.get_interface_num()).send_datagram(dgram, next_hop_addr);
    }
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
