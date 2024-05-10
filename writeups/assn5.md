Assignment 5 Writeup
=============

My name: [Minsu Sun]

My POVIS ID: [poodding397]

My student ID (numeric): [poodding397]

This assignment took me about [3] hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the NetworkInterface:
[
    **Data Structure**
    _ARP_table : mapping ip_address -> {EthernetAddress, Remaining TTL}
    _ARP_pending_ip_addresses : list of pending ARP ip address ({EthernetAddress, Reamining TTL})
    _ARP_pending_datagrams : list of pending datagrams due to ARP ({EthernetAddress, InternetDatagram})

    **Methods**
    send_datagram : send frame if next hop ip is known, if not, pend ARP and reserve datagram
    recv_frame : prune if received frame is not neither dedicated to current network interface or ARP boradcast / if frame is IPv4 type, parse and return datagram when successful parse / if not(it is ARP type), parse and when there is error or not destined to here, prune it. update ARP table and resend REPLY ARP message if needed. prune pending ARP messages and datagrams
    tick : push ARP message frames for timeout pending ARP messages with updating TTL / prune timeout ARP table rows with updating TTL
]

Implementation Challenges:
[
    Since I was using my own virtual machine, not the one provided from the course(it was borken), it was hard to debug.
    It took much time to resolve.
]

Remaining Bugs:
[
    There is no known bug.
]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
