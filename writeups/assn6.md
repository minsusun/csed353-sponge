Assignment 6 Writeup
=============

My name: [Minsu SUn]

My POVIS ID: [poodding397]

My student ID (numeric): [20220848]

This assignment took me about [2] hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the Router:
[
    **Routing Table**
    - `vector<RoutingTableEntry>`
    - `struct RoutingTableEntry` : `_route_prefix`, `_prefix_length`, `_prefix_mask`, `_next_hop`, `_interface_num`
        - `_prefix_mask` is used when longest prefix match route. Mask will be pre-computed before table entry pushed to the table
    - `add_route`
        - add entry to the routing table
        - compute mask to use computing longest-prefix-match route. (eq: `prefix_length == 0 ? 0 : ~uint32_t(0) << (32 - prefix_length)`)
    - `route_one_datagram` : route given datagram to appropriate interface
        - drop datagram on ttl expires
        - find longest-prefix-match route
        - drop datagram on no prefix-match route
        - update datagram ttl
        - send datagram on interface, considering the next hop
            - when no next hop(`next_hop.has_value() == false`) : `Address:from_ipv4_numeric(dst)`
            - else : `next_hop.value()`
]

Implementation Challenges:
[
    Selecting which data structure to store routing table entry and trying to implement as clean as possible with good performance were challenging.
]

Remaining Bugs:
[
    There is no known bug.
]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
