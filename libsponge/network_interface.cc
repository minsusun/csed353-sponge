#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    // when given next hop ip is known, just send frame
    if (this->_is_ip_known(next_hop_ip))
        this->_frames_out.push(this->_generate_frame(next_hop_ip, dgram));
    else {
        // when given next hop ip is unknown, pend datagram to pending datagram list
        this->_ARP_pending_datagrams.push_back({next_hop_ip, dgram});

        bool flag = false;

        // search for pending ARP requests for given ip
        for (auto &e : this->_ARP_pending_ip_addresses)
            if (e.first == next_hop_ip)
                flag = true;

        // when there was no pending ARP request, send ARP message frame and list up the ip in pending ip list
        if (!flag) {
            this->_ARP_pending_ip_addresses.push_back({next_hop_ip, ARPConfig::DEFAULT_ARP_TIMEOUT});
            this->_frames_out.push(this->_generate_frame(
                next_hop_ip, this->_generate_ARPMessage(next_hop_ip, ARPMessage::OPCODE_REQUEST)));
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    const EthernetHeader &header = frame.header();

    // prune for inappropriate frame
    if (header.dst != this->_ethernet_address && header.dst != ETHERNET_BROADCAST)
        return nullopt;

    // IPv4 Frame
    if (header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram datagram;

        // when cannot parse the payload of frame
        if (datagram.parse(frame.payload()) != ParseResult::NoError)
            return nullopt;

        // return successfully parsed datagram
        return datagram;
    } else {
        // ARP Frame
        ARPMessage message;

        // when cannot parse the payload of frame
        if (message.parse(frame.payload()) != ParseResult::NoError)
            return nullopt;

        // when target ip is not correct
        if (message.target_ip_address != this->_ip_address.ipv4_numeric())
            return nullopt;

        const EthernetAddress mac = message.sender_ethernet_address;
        const uint32_t ip = message.sender_ip_address;

        // add entry to ARP table
        this->_ARP_table[ip] = {mac, ARPConfig::DEFAULT_ARP_TTL};

        // reply for ARP request
        if (message.opcode == ARPMessage::OPCODE_REQUEST)
            this->_frames_out.push(this->_generate_frame(ip, this->_generate_ARPMessage(ip, ARPMessage::OPCODE_REPLY)));

        // prune pending ARP requests for received ip address
        for (auto it = this->_ARP_pending_ip_addresses.begin(); it != this->_ARP_pending_ip_addresses.end();) {
            if (it->first == ip)
                it = this->_ARP_pending_ip_addresses.erase(it);
            else
                it++;
        }

        // prune pending datagrams due to ARP requests for received ip address
        for (auto it = this->_ARP_pending_datagrams.begin(); it != this->_ARP_pending_datagrams.end();) {
            if (it->first == ip) {
                this->_frames_out.push(this->_generate_frame(it->first, it->second));
                it = this->_ARP_pending_datagrams.erase(it);
            } else
                it++;
        }
    }

    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    for (auto &e : this->_ARP_pending_ip_addresses) {
        // when pending ARP requests are timeout
        // 1. re-send ARP messages
        // 2. reset the TTL of ARP pending
        if (e.second <= ms_since_last_tick) {
            this->_frames_out.push(
                this->_generate_frame(e.first, this->_generate_ARPMessage(e.first, ARPMessage::OPCODE_REQUEST)));
            e.second = ARPConfig::DEFAULT_ARP_TIMEOUT;
        } else
            e.second -= ms_since_last_tick;     // update remaining TTL when no timeout
    }

    for (auto it = this->_ARP_table.begin(); it != this->_ARP_table.end();) {
        // when ARP table entry is timeout, erase
        if (it->second.second <= ms_since_last_tick)
            it = this->_ARP_table.erase(it);
        else {
            // when no timeout, update remaining TTL
            it->second.second -= ms_since_last_tick;
            it++;
        }
    }
}

//! \brief generate ARPMessage
ARPMessage NetworkInterface::_generate_ARPMessage(const uint32_t target_ip_address, const uint16_t opcode) {
    ARPMessage message;

    message.sender_ethernet_address = this->_ethernet_address;
    message.sender_ip_address = this->_ip_address.ipv4_numeric();

    message.target_ethernet_address =
        this->_is_ip_known(target_ip_address) ? this->_ARP_table[target_ip_address].first : EthernetAddress{};
    message.target_ip_address = target_ip_address;

    message.opcode = opcode;

    return message;
}

//! \brief build header of a frame
void NetworkInterface::_build_frame_header(EthernetHeader &header,
                                           const uint32_t &dst_ip_address,
                                           const uint16_t &type) {
    header.type = type;
    header.src = this->_ethernet_address;
    header.dst = this->_is_ip_known(dst_ip_address) ? this->_ARP_table[dst_ip_address].first : ETHERNET_BROADCAST;
}

//! \brief generate frame using InternetDatagram
EthernetFrame NetworkInterface::_generate_frame(const uint32_t dst_ip_address, InternetDatagram datagram) {
    EthernetFrame frame;
    this->_build_frame_header(frame.header(), dst_ip_address, EthernetHeader::TYPE_IPv4);
    frame.payload() = datagram.serialize();
    return frame;
}

//! \brief generate frame using ARPMessage
EthernetFrame NetworkInterface::_generate_frame(const uint32_t dst_ip_address, ARPMessage message) {
    EthernetFrame frame;
    this->_build_frame_header(frame.header(), dst_ip_address, EthernetHeader::TYPE_ARP);
    frame.payload() = message.serialize();
    return frame;
}