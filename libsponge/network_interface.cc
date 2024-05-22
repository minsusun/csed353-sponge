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
void DUMMY_CODE(Targs &&... /* unused */) {}

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
    const uint32_t nextHopIP = next_hop.ipv4_numeric();

    EthernetFrame frame;
    frame.header().type = EthernetHeader::TYPE_IPv4;
    frame.header().src = _ethernet_address;
    frame.payload().append(dgram.serialize());

    // if destination Ethernet address is already known, send it right away
    // create an Ethernet frame and set payload to be serialized datagram
    // set source and destination addresses
    if (_arpTable.count(nextHopIP) != 0 &&
        _timer < _arpTable[nextHopIP].ttl + 30000) {  // remember mapping only for 30 seconds
        frame.header().dst = _arpTable[nextHopIP].mac;
        _frames_out.push(frame);
    }

    // if destination Ethernet address is UNKNOWN, broadcast an ARP request for next hop's Ethernet address
    // and queue IP datagram so it can be sent after ARP reply is received
    else {
        _waitingFrames[nextHopIP].push_back(frame);
        // if there is already ARP request for same IP address in last 5 seconds, do not send a second request
        if (_waitingFrames[nextHopIP].size() > 1 && _waitingTimer[nextHopIP] + 5000 >= _timer) {
            return;
        }

        ARPMessage arp;
        arp.opcode = ARPMessage::OPCODE_REQUEST;
        arp.sender_ethernet_address = _ethernet_address;
        arp.sender_ip_address = _ip_address.ipv4_numeric();
        arp.target_ip_address = nextHopIP;

        EthernetFrame arpFrame;
        arpFrame.header().type = EthernetHeader::TYPE_ARP;
        arpFrame.header().src = _ethernet_address;
        arpFrame.header().dst = ETHERNET_BROADCAST;
        arpFrame.payload().append(arp.serialize());

        _waitingTimer[nextHopIP] = _timer;
        _frames_out.push(arpFrame);
    }
    return;
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // (1)
    // ignore frames not destined for network interface
    // 1. if ethernet destination is broadcast address
    // 2. if ethernet destination is interface's own Ethernet address
    if (frame.header().dst != ETHERNET_BROADCAST && frame.header().dst != _ethernet_address) {
        return {};
    }

    // (2)
    // if inbound frame is IPv4, parse payload as InternetDatagram
    // if successful, return resulting InternetDatagram to the caller
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError) {
            return dgram;
        } else {
            return {};  // unsuccessful parse result
        }
    }

    // (3)
    // if inbound frame is ARP, parse payload as ARPMessage
    // if successful, remember mapping between sender's IP address and Ethernet address for 30 seconds
    // if it's an ARP request asking for our IP address, send an ARP reply
    else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp;
        if (arp.parse(frame.payload()) != ParseResult::NoError) {
            return {};  // quit on unsuccessful parse result
        }

        // learn the mapping (mac, ttl)
        _arpTable[arp.sender_ip_address] = {arp.sender_ethernet_address, _timer};

        // if it's an ARP request asking for our IP address, send an ARP reply
        if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == _ip_address.ipv4_numeric()) {
            ARPMessage reply;
            reply.opcode = ARPMessage::OPCODE_REPLY;
            reply.sender_ethernet_address = _ethernet_address;
            reply.sender_ip_address = _ip_address.ipv4_numeric();
            reply.target_ethernet_address = arp.sender_ethernet_address;
            reply.target_ip_address = arp.sender_ip_address;

            EthernetFrame replyFrame;
            replyFrame.header().type = EthernetHeader::TYPE_ARP;
            replyFrame.header().src = _ethernet_address;
            replyFrame.header().dst = arp.sender_ethernet_address;
            replyFrame.payload().append(reply.serialize());
            _frames_out.push(replyFrame);
        }

        // update associated waiting frames destination address and send them
        if (_waitingFrames.count(arp.sender_ip_address) != 0) {
            for (EthernetFrame &waitingFrame : _waitingFrames[arp.sender_ip_address]) {
                waitingFrame.header().dst = frame.header().src;
                _frames_out.push(waitingFrame);
            }
            _waitingFrames.erase(arp.sender_ip_address);
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { _timer += ms_since_last_tick; }
