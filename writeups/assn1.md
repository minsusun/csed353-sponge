Assignment 1 Writeup
=============

My name: [Sun Minsu]

My POVIS ID: [poodding397]

My student ID (numeric): [20220848]

This assignment took me about [12] hours to do (including the time on studying, designing, and writing the code).

Program Structure and Design of the StreamReassembler:
[
    Basically, StreamReassembler manage data with map data structure STL provided in C++
    map data structure has uint64_t as key and std::string as value
    each byte of data will be inserted to map with each index as key

    for each substring entered into streamreassembler, divide them into bytes and handle each to proper position
    after inserting each byte to map, search for bytes that could be re-assembled and go out to the byte stream.
]

Implementation Challenges:
[
    There were a lot of undescribed constraints to handle
    + the prioirty of overlapped data is higher than previous data
    + reassembler do not make any situation where the stream is stuck
        + stream does not get full when after getting full, the stream will be interlocked
        + stream do not accept bytes which make stream unavailble for streaming

    all the challenges were cleared out after checking out the test codes and analyzing them
]

Remaining Bugs:
[
    - Time consuming a lot
    All the other tests passed within 1 seconds but Test #24 t_strm_reassem_win timeouts.
    Occasionally, that test pass but it is not consistent.
]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
