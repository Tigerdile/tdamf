# tdamf
Tigerdile's AMF0 and AMF3 (Adobe Message Format) Library for C++

# WHY?
There are a handful of AMF libraries for C / C++.  Most are for interpreted languages such as PHP.  The ones for C / C++ are all of fairly low code quality with sparce documentation.  For a larger project, I made my own, and I tried to make it clean and well documented.

# IMPORTANT DESIGN DECISIONS
This is designed to work with an in-memory buffer, such as data received over the network.  Its designed, specifically, to work with an RTMP stream.  As such, its assumed that memory allocations should be done as infrequently as possible.

Therefore, we DO NOT copy your memory buffer.  When you give us a buffer to decode, we will make references within your buffer but we do an absolute minimum of memory allocations.  A few allocations are unavoidable, as AMF supports things such as nested objects and there's not a very clean way to do that without doing memory alloc's.  However, it's assumed your buffer is large and will live for the duration of your work.  You can delete it after you're finished with your decoded AMF.

# THANKS TO...
Let me be very clear on this; I "cribbed" a lot from librtmp.  I read the specs and read through other libraries that all probably cribbed off librtmp as well but didn't give them the props they deserved.

librtmp is part of rtmpdump, located here: https://rtmpdump.mplayerhq.hu/

While I think they did awesome work, their AMF code is an undocumented mess.

