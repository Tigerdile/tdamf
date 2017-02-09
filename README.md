# tdamf
Tigerdile's AMF0 and AMF3 (Adobe Message Format) Library for C++ 11 that just uses standard C++ libraries without external dependencies.

# WHY?
There are a handful of AMF libraries for C / C++.  Most are for interpreted languages such as PHP.  The ones for C / C++ are all of fairly low code quality with sparce documentation.  For a larger project, I made my own, and I tried to make it clean and well documented.

# IMPORTANT DESIGN DECISIONS
This is designed to work with an in-memory buffer, such as data received over the network.  Its designed, specifically, to work with an RTMP stream.  As such, its assumed that memory allocations should be done as infrequently as possible.

Therefore, we DO NOT copy your memory buffer.  When you give us a buffer to decode, we will make references within your buffer but we do an absolute minimum of memory allocations.  A few allocations are unavoidable, as AMF supports things such as nested objects and there's not a very clean way to do that without doing memory alloc's.  However, it's assumed your buffer is large and will live for the duration of your work.  You can delete it after you're finished with your decoded AMF.

Encoding works similarly -- we don't allocate memory, but we tell you how much memory is needed.  Its up to you to allocate or re-use; in fact, if your messages that you are encoding are always going to be less than, say, 1k, you could allocate a 1k buffer and re-use it.  We'll throw an exception if you overflow!

# THANKS TO...
Let me be very clear on this; I "cribbed" a lot from librtmp.  I read the specs and read through other libraries that all probably cribbed off librtmp as well but didn't give them the props they deserved.

librtmp is part of rtmpdump, located here: https://rtmpdump.mplayerhq.hu/

While I think they did awesome work, their AMF code is an undocumented mess.  And the library can't be installed standalone very easily.  And my library supports at least a couple things theirs does not (such as Typed Objects).

# THINGS I'M NOT SUPPORTING (YET?)
Okay ... I never say never, but there's a few things I am not planning on supporting.  These plans may change based on demand and if someone's willing to help.

* Windows Support - I don't have a windows dev environment and I have no use for this myself.  Porting this to work with Windows would be trivial and mostly tweaking the amf.hpp header and CMake files.  If there's demand I may put the effort into supporting it, but for now, I don't care.
* C++0x support - I chose C++ 11, and I'm sticking with it.  Chances are whatever developer is making something with AMF has access to a C++11 compiler.  I don't use too many C++11 constructs so "back porting" it to C++0x would be possible, but again, I have no use for it.
* File / Stream suppport - My use case works with in-memory buffers only.  Specifically, RTMP; I get chunk streams with this stuff encoded, so I always have a complete AMF structure in my memory and a fairly small one at that.  As such, my use case is to allocate my buffer once and reuse it as much as possible for performance reasons.  If you need a file stream or something that dynamically allocates memory, this library is probably not for you and there are others out there that may suit you better.  But I doubt many people are working with AMF's so big you really need to chunk it to a file in order to use it all.
* References - As of the time of this writing, I'm not supporting the AMF 'Reference' type.  This is mostly because I'm not 100% clear on how it works.  If I got some sample data showing reference types in action, I would feel more comfortable implementing them.  The code has notes as to what I've figured out so far in comments.  This seems common to most AMF libraries; the 'reference' type is an unpopular feature.
