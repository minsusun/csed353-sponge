Assignment 2 Writeup
=============

My name: [Sun Minsu]

My POVIS ID: [poodding397]

My student ID (numeric): [20220848]

This assignment took me about [8] hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:
[
    ** segment_received **
    1. Verify the SYN state. Once Receiver received SYN flag, then it means it is ready to receive data. In other case, it is not.
    2. Update SYN state by verifying SYN flag from segment header
    3. No SYN received even after verifying the header, abort.
    4. Retreive data from payload and calculate stream index by unwrapping from seqno in header. Unwrapping is based on ISN and first unassembeld index as checkpoint
    5. Push substring to reassembler with calculated stream index.

    ** ackno **
    1. return nullopt when no SYN received.
    2. calculate absolute seqno with considering SYN and FIN.
    3. return wrapped ackno with absolute seqno and ISN.
]

Implementation Challenges:
[
    It was little hard to do math calculations. :)
]

Remaining Bugs:
[
    There was no bugs I noticed.
]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
