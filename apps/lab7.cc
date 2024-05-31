#include "address.hh"
#include "arp_message.hh"
#include "bidirectional_stream_copy.hh"
#include "router.hh"
#include "tcp_over_ip.hh"
#include "tcp_sponge_socket.cc"
#include "util.hh"

#include <cstdlib>
#include <iostream>
#include <thread>

#include <array>
#include <cstring>
#include <sstream>
#include <iomanip>

using namespace std;

auto rd = get_random_generator();

class SHA256 {
  private:
    // SHA256 context variables
    uint8_t _data[64];
    uint8_t _blockLen;
    uint64_t _bitLen;
    uint32_t _state[8];

    static constexpr array<uint32_t, 64> K = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    static uint32_t ROT(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); };
    static uint32_t CH(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); };
    static uint32_t EP0(uint32_t x) { return SHA256::ROT(x, 2) ^ SHA256::ROT(x, 13) ^ SHA256::ROT(x, 22); }
    static uint32_t EP1(uint32_t x) { return SHA256::ROT(x, 6) ^ SHA256::ROT(x, 11) ^ SHA256::ROT(x, 25); }
    static uint32_t MAJ(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (y & z) ^ (z & x); };
    static uint32_t SIG0(uint32_t x) { return SHA256::ROT(x, 7) ^ SHA256::ROT(x, 18) ^ (x >> 3); };
    static uint32_t SIG1(uint32_t x) { return SHA256::ROT(x, 17) ^ SHA256::ROT(x, 19) ^ (x >> 10); };

    void _update() {
        uint32_t aux[8], c[64];

        for (uint8_t i = 0; i < 8; i++)
            aux[i] = _state[i];

        for (uint8_t i = 0; i < 16; i++)
            c[i] = (_data[4 * i] << 24) | (_data[4 * i + 1] << 16) | (_data[4 * i + 2] << 8) | _data[4 * i + 3];

        for (uint8_t i = 16; i < 64; i++)
            c[i] = SHA256::SIG1(c[i - 2]) + c[i - 7] + SHA256::SIG0(c[i - 15]) + c[i - 16];

        for (uint8_t i = 0; i < 64; i++) {
            uint32_t t1 = aux[7] + SHA256::EP1(aux[4]) + SHA256::CH(aux[4], aux[5], aux[6]) + K[i] + c[i];
            uint32_t t2 = SHA256::EP0(aux[0]) + SHA256::MAJ(aux[0], aux[1], aux[2]);

            aux[7] = aux[6];
            aux[6] = aux[5];
            aux[5] = aux[4];
            aux[4] = aux[3] + t1;
            aux[3] = aux[2];
            aux[2] = aux[1];
            aux[1] = aux[0];
            aux[0] = t1 + t2;
        }

        for (uint8_t i = 0; i < 8; i++)
            _state[i] += aux[i];
    };

  public:
    SHA256()
        : _blockLen(0)
        , _bitLen(0)
        , _state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19} {};

    array<uint8_t, 32> digest(const string &input) {
        array<uint8_t, 32> hash;

        for (auto &e : input) {
            _data[_blockLen++] = static_cast<uint8_t>(e);
            if (_blockLen == 64) {
                _bitLen += 512;
                _blockLen = 0;
                _update();
            }
        }

        // Put padding to remaining blocks if needed
        // Make sure there's reserve space for length information
        uint8_t idx = _blockLen;
        uint8_t end = _blockLen < 56 ? 56 : 64;

        _data[idx++] = 0x80;
        while (idx < end)
            _data[idx++] = 0x00;

        if (_blockLen >= 56) {
            _update();
            memset(_data, 0, 56);
        }

        // Length information
        _bitLen += 8 * _blockLen;
        _data[63] = _bitLen;
        _data[62] = _bitLen >> 8;
        _data[61] = _bitLen >> 16;
        _data[60] = _bitLen >> 24;
        _data[59] = _bitLen >> 32;
        _data[58] = _bitLen >> 40;
        _data[57] = _bitLen >> 48;
        _data[56] = _bitLen >> 56;

        _update();

        // Revert the hash table: SHA is big-endian
        for (uint8_t i = 0; i < 4; i++) {
            for (uint8_t j = 0; j < 8; j++)
                hash[i + 4 * j] = (_state[j] >> (24 - 8 * i)) & 0xFF;
        }

        return hash;
    };

    static string toString(const array<uint8_t, 32> &hash, const size_t len = 5) {
        stringstream ss;
        for (uint8_t i = 0; i < 32; i++)
            ss << setfill('0') << setw(2) << hex
               << +hash[i];  // uint8_t is alias of unsigned char: https://stackoverflow.com/a/23575662
        string s = ss.str();
        return s.substr(s.size() - len);
    };
};

EthernetAddress random_host_ethernet_address() {
    EthernetAddress addr;
    for (auto &byte : addr) {
        byte = rd();  // use a random local Ethernet address
    }
    addr.at(0) |= 0x02;  // "10" in last two binary digits marks a private Ethernet address
    addr.at(0) &= 0xfe;

    return addr;
}

EthernetAddress random_router_ethernet_address() {
    EthernetAddress addr;
    for (auto &byte : addr) {
        byte = rd();  // use a random local Ethernet address
    }
    addr.at(0) = 0x02;  // "10" in last two binary digits marks a private Ethernet address
    addr.at(1) = 0;
    addr.at(2) = 0;

    return addr;
}

string summary(const EthernetFrame &frame) {
    string ret;
    ret += frame.header().to_string();
    switch (frame.header().type) {
        case EthernetHeader::TYPE_IPv4: {
            InternetDatagram dgram;
            if (dgram.parse(frame.payload().concatenate()) == ParseResult::NoError) {
                ret += " " + dgram.header().summary();
                if (dgram.header().proto == IPv4Header::PROTO_TCP) {
                    TCPSegment tcp_seg;
                    if (tcp_seg.parse(dgram.payload().concatenate(), dgram.header().pseudo_cksum()) ==
                        ParseResult::NoError) {
                        ret += " " + tcp_seg.header().summary();
                    }
                }
            } else {
                ret += " (bad IPv4)";
            }
        } break;
        case EthernetHeader::TYPE_ARP: {
            ARPMessage arp;
            if (arp.parse(frame.payload()) == ParseResult::NoError) {
                ret += " " + arp.to_string();
            } else {
                ret += " (bad ARP)";
            }
        }
        default:
            break;
    }
    return ret;
}

class NetworkInterfaceAdapter : public TCPOverIPv4Adapter {
  private:
    NetworkInterface _interface;
    Address _next_hop;
    pair<FileDescriptor, FileDescriptor> _data_socket_pair = socket_pair_helper(SOCK_DGRAM);

    void send_pending() {
        while (not _interface.frames_out().empty()) {
            _data_socket_pair.first.write(_interface.frames_out().front().serialize());
            _interface.frames_out().pop();
        }
    }

  public:
    NetworkInterfaceAdapter(const Address &ip_address, const Address &next_hop)
        : _interface(random_host_ethernet_address(), ip_address), _next_hop(next_hop) {}

    optional<TCPSegment> read() {
        EthernetFrame frame;
        if (frame.parse(_data_socket_pair.first.read()) != ParseResult::NoError) {
            return {};
        }

        // Give the frame to the NetworkInterface. Get back an Internet datagram if frame was carrying one.
        optional<InternetDatagram> ip_dgram = _interface.recv_frame(frame);

        // The incoming frame may have caused the NetworkInterface to send a frame
        send_pending();

        // Try to interpret IPv4 datagram as TCP
        if (ip_dgram) {
            return unwrap_tcp_in_ip(ip_dgram.value());
        }

        return {};
    }
    void write(TCPSegment &seg) {
        _interface.send_datagram(wrap_tcp_in_ip(seg), _next_hop);
        send_pending();
    }
    void tick(const size_t ms_since_last_tick) {
        _interface.tick(ms_since_last_tick);
        send_pending();
    }
    NetworkInterface &interface() { return _interface; }
    queue<EthernetFrame> frames_out() { return _interface.frames_out(); }

    operator FileDescriptor &() { return _data_socket_pair.first; }
    FileDescriptor &frame_fd() { return _data_socket_pair.second; }
};

class TCPSocketLab7 : public TCPSpongeSocket<NetworkInterfaceAdapter> {
    Address _local_address;

  public:
    TCPSocketLab7(const Address &ip_address, const Address &next_hop)
        : TCPSpongeSocket<NetworkInterfaceAdapter>(NetworkInterfaceAdapter(ip_address, next_hop))
        , _local_address(ip_address) {}

    void connect(const Address &address) {
        FdAdapterConfig multiplexer_config;

        _local_address = Address{_local_address.ip(), uint16_t(random_device()())};
        cerr << "DEBUG: Connecting from " << _local_address.to_string() << "...\n";
        multiplexer_config.source = _local_address;
        multiplexer_config.destination = address;

        TCPSpongeSocket<NetworkInterfaceAdapter>::connect({}, multiplexer_config);
    }

    void bind(const Address &address) {
        if (address.ip() != _local_address.ip()) {
            throw runtime_error("Cannot bind to " + address.to_string());
        }
        _local_address = Address{_local_address.ip(), address.port()};
    }

    void listen_and_accept() {
        FdAdapterConfig multiplexer_config;
        multiplexer_config.source = _local_address;
        TCPSpongeSocket<NetworkInterfaceAdapter>::listen_and_accept({}, multiplexer_config);
    }

    NetworkInterfaceAdapter &adapter() { return _datagram_adapter; }
};

void program_body(bool is_client, const string &bounce_host, const string &bounce_port, const bool debug) {
    UDPSocket internet_socket;
    Address bounce_address{bounce_host, bounce_port};

    /* let bouncer know where we are */
    internet_socket.sendto(bounce_address, "");
    internet_socket.sendto(bounce_address, "");
    internet_socket.sendto(bounce_address, "");

    /* set up the router */
    Router router;

    unsigned int host_side, internet_side;

    if (is_client) {
        host_side = router.add_interface({random_router_ethernet_address(), {"192.168.0.1"}});
        internet_side = router.add_interface({random_router_ethernet_address(), {"10.0.0.192"}});
        router.add_route(Address{"192.168.0.0"}.ipv4_numeric(), 16, {}, host_side);
        router.add_route(Address{"10.0.0.0"}.ipv4_numeric(), 8, {}, internet_side);
        router.add_route(Address{"172.16.0.0"}.ipv4_numeric(), 12, Address{"10.0.0.172"}, internet_side);
    } else {
        host_side = router.add_interface({random_router_ethernet_address(), {"172.16.0.1"}});
        internet_side = router.add_interface({random_router_ethernet_address(), {"10.0.0.172"}});
        router.add_route(Address{"172.16.0.0"}.ipv4_numeric(), 12, {}, host_side);
        router.add_route(Address{"10.0.0.0"}.ipv4_numeric(), 8, {}, internet_side);
        router.add_route(Address{"192.168.0.0"}.ipv4_numeric(), 16, Address{"10.0.0.192"}, internet_side);
    }

    /* set up the client */
    TCPSocketLab7 sock =
        is_client ? TCPSocketLab7{{"192.168.0.50"}, {"192.168.0.1"}} : TCPSocketLab7{{"172.16.0.100"}, {"172.16.0.1"}};

    atomic<bool> exit_flag{};

    /* set up the network */
    thread network_thread([&]() {
        try {
            EventLoop event_loop;
            // Frames from host to router
            event_loop.add_rule(sock.adapter().frame_fd(), Direction::In, [&] {
                EthernetFrame frame;
                if (frame.parse(sock.adapter().frame_fd().read()) != ParseResult::NoError) {
                    return;
                }
                if (debug) {
                    SHA256 sha;
                    auto hash = sha.digest(frame.serialize().concatenate());
                    cerr << "---------------------------------------------------------------------------------------------------------------" << "\n";
                    cerr << "    Host->router: " << summary(frame) << "\n";
                    cerr << "     SHA256 HASH: " << SHA256::toString(hash) << "\n";
                    cerr << "---------------------------------------------------------------------------------------------------------------" << "\n";
                }
                router.interface(host_side).recv_frame(frame);
                router.route();
            });

            // Frames from router to host
            event_loop.add_rule(sock.adapter().frame_fd(),
                                Direction::Out,
                                [&] {
                                    auto &f = router.interface(host_side).frames_out();
                                    if (debug) {
                                        SHA256 sha;
                                        auto hash = sha.digest(f.front().serialize().concatenate());
                                        cerr << "---------------------------------------------------------------------------------------------------------------" << "\n";
                                        cerr << "    Host->router: " << summary(f.front()) << "\n";
                                        cerr << "     SHA256 HASH: " << SHA256::toString(hash) << "\n";
                                        cerr << "---------------------------------------------------------------------------------------------------------------" << "\n";
                                    }
                                    sock.adapter().frame_fd().write(f.front().serialize());
                                    f.pop();
                                },
                                [&] { return not router.interface(host_side).frames_out().empty(); });

            // Frames from router to Internet
            event_loop.add_rule(internet_socket,
                                Direction::Out,
                                [&] {
                                    auto &f = router.interface(internet_side).frames_out();
                                    if (debug) {
                                        SHA256 sha;
                                        auto hash = sha.digest(f.front().serialize().concatenate());
                                        cerr << "---------------------------------------------------------------------------------------------------------------" << "\n";
                                        cerr << "Router->Internet: " << summary(f.front()) << "\n";
                                        cerr << "     SHA256 HASH: " << SHA256::toString(hash) << "\n";
                                        cerr << "---------------------------------------------------------------------------------------------------------------" << "\n";
                                    }
                                    internet_socket.sendto(bounce_address, f.front().serialize());
                                    f.pop();
                                },
                                [&] { return not router.interface(internet_side).frames_out().empty(); });

            // Frames from Internet to router
            event_loop.add_rule(internet_socket, Direction::In, [&] {
                EthernetFrame frame;
                if (frame.parse(internet_socket.read()) != ParseResult::NoError) {
                    return;
                }
                if (debug) {
                    SHA256 sha;
                    auto hash = sha.digest(frame.serialize().concatenate());
                    cerr << "---------------------------------------------------------------------------------------------------------------" << "\n";
                    cerr << "Internet->router: " << summary(frame) << "\n";
                    cerr << "     SHA256 HASH: " << SHA256::toString(hash) << "\n";
                    cerr << "---------------------------------------------------------------------------------------------------------------" << "\n";
                }
                router.interface(internet_side).recv_frame(frame);
                router.route();
            });

            while (true) {
                if (EventLoop::Result::Exit == event_loop.wait_next_event(50)) {
                    cerr << "Exiting...\n";
                    return;
                }
                router.interface(host_side).tick(50);
                router.interface(internet_side).tick(50);
                if (exit_flag) {
                    return;
                }
            }
        } catch (const exception &e) {
            cerr << "Thread ending from exception: " << e.what() << "\n";
        }
    });

    try {
        if (is_client) {
            sock.connect({"172.16.0.100", 1234});
        } else {
            sock.bind({"172.16.0.100", 1234});
            sock.listen_and_accept();
        }

        bidirectional_stream_copy(sock);
        sock.wait_until_closed();
    } catch (const exception &e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    cerr << "Exiting... ";
    exit_flag = true;
    network_thread.join();
    cerr << "done.\n";
}

void print_usage(const string &argv0) {
    cerr << "Usage: " << argv0 << " client HOST PORT [debug]\n";
    cerr << "or     " << argv0 << " server HOST PORT [debug]\n";
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        if (argc != 4 and argc != 5) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        if (argv[1] != "client"s and argv[1] != "server"s) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        program_body(argv[1] == "client"s, argv[2], argv[3], argc == 5);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
