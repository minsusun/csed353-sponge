Assignment 3 Writeup
=============

My name: [Sun Minsu]

My POVIS ID: [poodding397]

My student ID (numeric): [20220848]

This assignment took me about [12] hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPSender:
[
    **Outstanding segments**
    - Outstanding segments saved in TCPSender::_outstanding_segments queue as a segment pushed in to the TCPSender::_segments_out queue.
    - These segments will be retransmitted when tick triggered timeout.
    - These segments will be popped when ack received.

    **Timer**
    - Retrasmission timer is controlled by the _timer_elapsed and _is_timer_on member variables mainly.
    - Every call to TCPSender::tick(), the timer information updates and every time ack receive or fill window, TCPSedner considers the timer should be on or resetted or off and controls it.

    **Trasmission**
    - Everytime when ack received, TCPSender updates the acked seqno and prune the arrived outstanding segments.
    - Everytime tick called, TCPSender updates the timer and retransmit the first outstanding segment if needed.
    - Everytime fill window called, TCPSender retrieve data from byte stream and make a segments and push to the TCPSender::_segments_out to transmit.
]

Implementation Challenges:
[
    - The realization of idea that TCP sender should keep track of receivers remaining window with estimation took a while.
    - Determining the actual payload size and the nubmer of segments to be sent was little tricky to think.
]

Remaining Bugs:
[
    There is no known bugs currently.
]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
