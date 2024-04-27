Assignment 4 Writeup
=============

My name: [Minsu Sun]

My POVIS ID: [poodding397]

My student ID (numeric): [20220848]

This assignment took me about [12] hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Your benchmark results (without reordering, with reordering): [1.13, 0.94]

Program Structure and Design of the TCPConnection:
[
    **TCPConnection::segment_received**
    - reset the time since the last segment received
    - report as error if segment's header contains rst flag
    - ignore the segment when the connection is inactive
    - check if the segment if keep-alive segment and if it is, then send ack
    - else, alarm receiver about segment
        - if segment has ack, alarm sender about ack
        - else, fill window(for when segment ahs ack, window will be filled by ack_received)
        - if segment has data, send ack
        - if current connection's receiver ended but sender didn't end with unsent fin flag, disable lingering
    - send segments

    **TCPConneciton::write**
    - write data in sender's stream
    - fill window and send segments

    **TCPConneciton::tick**
    - update _time_since_last_segment_received and alarm sender about tick
    - if there were too many retransmissions, report as error
    - if the connection can be closed but the time passed since last segment received is greater than 10 * rt_timeout, disable lingering
    - send segments

    **TCPConneciton::connect**
    - just fill window and send segments

    **TCPConneciton::_send**
    - until the sender's segments_out queue is empty repeat below:
        - keep track of fin flag of segment
        - mark ack and ackno in header from receiver
        - mark win and rst
        - push segments into _segments_out queue
]

Implementation Challenges:
[
    - Finding any bugs from previous submissions is the most challenging thing on doing this assignment.
    - Situation of controlling both sender and receiver at once was challenging.
    - Finding conditions of certain point or status was challenging.
]

Remaining Bugs:
[
    There is no known bugs currently.
]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
