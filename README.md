Stones of Nvwa
==============

The Name
--------

Nvwa ("v" is pronounced like the French "u") is one of the most ancient
Chinese goddesses.  She was said to have created human beings, and, when
the evil god Gong-gong crashed himself upon one of the pillars that
supported the sky and made a hole in it, she mended the sky with
five-coloured stones.

I thought of the name Nvwa by analogy with Loki.  Since it is so small a
project and it contains utilities instead of a complete framework, I
think "stones" a good metaphor.


Code Organization
-----------------

Nvwa versions prior to 1.0 did not use a namespace.  It looked to me
overkill to use a namespace in such a small project.  However, having
had more project experience and seen more good examples, I changed my
mind.  All nvwa functions and global variables were moved into the
namespace `nvwa`.

The changes do not mean anything to new users.  You just need to follow
the modern C++ way.  You should add the root directory of this project
to your include directory (there should be a *nvwa* directory inside it
with the source files).  To include the header file, use
`#include <nvwa/...>`.


Contents
--------

A brief introduction follows.  Check the Doxygen documentation for
more (technical) details.

*aligned\_memory.cpp*  
*aligned\_memory.h*

Two function for cross-platform aligned memory allocation and
deallocation.  This is a thin layer, and it is needed only because the
C++17/C11 `aligned_alloc` pairs with `free`, which does not work on
Microsoft Windows.

*bool\_array.cpp*  
*bool\_array.h*

A replacement of `std::vector<bool>`.  I wrote it before I knew of
`vector<bool>`, or I might not have written it at all.  However, it is
faster than many `vector<bool>` implementations (your mileage may vary),
and it has members like `at`, `set`, `reset`, `flip`, and `count`.  I
personally find `count` very useful.

*c++\_features.h*

Detection macros for certain modern C++ features that might be of
interest to code/library writers.  I have used existing online resources,
and I have tested them on popular platforms and compilers that I have
access to (Windows, Linux, and macOS; MSVC, GCC, and Clang), but of
course not all versions or all combinations.  Patches are welcome for
corrections and amendments.

*class\_level\_lock.h*

The Loki `ClassLevelLockable` adapted to use the `fast_mutex` layer.
One minor divergence from Loki is that the template has an additional
template parameter `_RealLock` to boost the performance in non-locking
scenarios.  Cf. *object\_level\_lock.h*.

*cont\_ptr\_utils.h*

Utility functors for containers of pointers adapted from Scott Meyers'
*Effective STL*.

*context.cpp*  
*context.h*

Utilities for setting up and using thread-local context stacks.  It
provides a generic `NVWA_CONTEXT_CHECKPOINT` macro, and several
functions to work with the contexts.  They are used by
*memory\_trace.cpp*.  Also, `print_exception_contexts` can be used in a
`catch` handler to show the call path (independent of memory tracing).

*debug\_new.cpp*  
*debug\_new.h*

A cross-platform, thread-safe memory leak detector.  It is a
light-weight one designed only to catch unmatched pairs of new/delete.
I know there are already many existing memory leak detectors, but as far
as I know, free solutions are generally slow, memory-consuming, and
quite complicated to use.  This solution is very easy to use, and has
very low space/time overheads.  Just link in *debug\_new.cpp* for
leakage report, and include *debug\_new.h* for additional file/line
information.  It will automatically switch on multi-threading when the
appropriate option of a recognized compiler is specified.  Check
*fast\_mutex.h* for more threading details.

Special support for gcc/binutils has been added to *debug\_new* lately.
Even if the header file *debug\_new.h* is not included, or
`_DEBUG_NEW_REDEFINE_NEW` is defined to 0 when it is included, file/line
information can be displayed if debugging symbols are present in the
executable, since *debug\_new* stores the caller addresses of memory
allocation/deallocation routines and they will be converted with
`addr2line` (or `atos` on macOS) on the fly.  This makes memory tracing
much easier.

*Note for Linux/macOS users:* Nowadays GCC and Clang create
*position-independent executables* (PIEs) by default so that the OS can
apply *address space layout randomization* (ASLR), loading a PIE at a
random address each time it is executed and making it difficult to
convert the address to symbols.  The simple solution is to use the
command-line option `-no-pie` on Linux, or `-Wl,-no_pie` on macOS.

With an idea from Greg Herlihy's post in comp.lang.c++.moderated, the
implementation was much improved in 2007.  The most significant result
is that placement `new` can be used with *debug\_new* now!  Full support
for `new(std::nothrow)` is provided, with its null-returning error
semantics (by default).  Memory corruption will be checked on freeing
the pointers and checking the leaks, and a new function
`check_mem_corruption` is added for your on-demand use in debugging.
You may also want to define `_DEBUG_NEW_TAILCHECK` to something like 4
for past-end memory corruption check, which is off by default to ensure
performance is not affected.

An article on its design and implementation is available at

[A Cross-Platform Memory Leak Detector][lnk_leakage]

Cf. *memory\_trace.cpp* and *memory\_trace.h*.

*fast\_mutex.h*

The threading transparent layer simulating a non-recursive mutex.  It
supports C++11 mutex, POSIX mutex, and Win32 critical section, as well
as a no-threads mode.  Unlike Loki and some other libraries, threading
mode is not to be specified in code, but detected from the environment.
It will automatically switch on multi-threading mode (inter-thread
exclusion) when the `-MT`/`-MD` option of MSVC, the `-mthreads` option
of MinGW GCC, or the `-pthread` option of GCC under POSIX environments,
is used.  One advantage of the current implementation is that the
construction and destruction of a static object using a static
`fast_mutex` not yet constructed or already destroyed are allowed to
work (with lock/unlock operations ignored), and there are re-entry
checks for lock/unlock operations when the preprocessing symbol `_DEBUG`
is defined.

*fc\_queue.h*

A queue that has a fixed capacity (maximum number of allowed items).
All memory is pre-allocated when the queue is initialized, so memory
consumption is well controlled.  When the queue is full, inserting a new
item will discard the oldest item in the queue.  Care is taken to ensure
this class template provides the strong exception safety guarantee.

This class template is a good exercise for me to make a STL-type
container.  Depending on your needs, you may find `circular_buffer` in
Boost more useful, which provides similar functionalities and a richer
API.  However, the design philosophies behind the two implementations
are quite different.  `fc_queue` is modelled closely on `std::queue`,
and I optimize mainly for a lockless producer-consumer pattern—when
there is one producer and one consumer, and the producer does not try to
queue an element when the queue is already full, no lock is needed for
the queue operations.

I have recently also switched to using C++11 atomic variables in
`fc_queue`.  Previously the code worked for me, but at least
theoretically it was not safe: compilers may optimize the code in
unexpected ways, and multiple processors may cause surprises too.  Using
the old interface is not optimal now in the producer-consumer scenario,
though, and two new read/write functions are provided to make it work
more efficiently.

*file\_line\_reader.cpp*  
*file\_line\_reader.h*

This is one of the line reading classes I implemented modelling the
Python approach.  They make reading lines from a file a simple loop.
This implementation allows reading from a traditional `FILE*`.  Cf.
*istream\_line\_reader.h*, *mmap\_byte\_reader.h*, and
*mmap\_line\_reader.h*.

See the following blog for the motivation and example code:

[Performance of My Line Readers][lnk_line_readers]

*fixed\_mem\_pool.h*

A memory pool implementation that requires initialization (allocates a
fixed-size chunk) prior to its use.  It is simple and makes no memory
fragmentation, but the memory pool size cannot be changed after
initialization.  Macros are provided to easily make a class use pooled
`new`/`delete`.

*functional.h*

This is my test bed for functional programming.  If you are into
functional programming, check it out.  It includes functional
programming patterns like:

- map
- reduce
- compose
- fixed-point combinator
- curry
- optional

My blogs on functional programming may be helpful:

[Study Notes: Functional Programming with C++][lnk_functional_note]  
[*Y* Combinator and C++][lnk_y_combinator]  
[Type Deduction and My Reference Mistakes][lnk_type_deduction]  
[Generic Lambdas and the `compose` Function][lnk_generic_lambdas]

*istream\_line\_reader.h*

This is one of the line reading classes I implemented modelling the
Python approach.  They make reading lines from a file a simple loop.
This implementation allows reading from an input stream.
Cf. *file\_line\_reader.h* and *mmap\_line\_reader.h*.

See the following blogs for the motivation and example code:

[Python `yield` and C++ Coroutines][lnk_coroutines]  
[Performance of My Line Readers][lnk_line_readers]

*malloc\_allocator.h*

An allocator that invokes `malloc`/`free` instead of operator
`new`/`delete`.  It is necessary in the global operator `new`/`delete`
replacement code, like *memory\_trace.cpp*.

*mem\_pool\_base.cpp*  
*mem\_pool\_base.h*

A class solely to be inherited by memory pool implementations.  It is
used by *static\_mem\_pool* and *fixed\_mem\_pool*.

*memory\_trace.cpp*  
*memory\_trace.h*

This is a new version of the memory leak detector, designed to use
stackable memory checkpoints, instead of redefined `new` to record the
context information.  Like *debug\_new*, it is quite easy to use, and
has very low space/time overheads.  One needs to link in
*memory\_trace.cpp*, *aligned\_memory.cpp*, and *context.cpp* for
leakage report, and include *memory\_trace.h* for adding a new
checkpoint with the macro `NVWA_MEMORY_CHECKPOINT()`.

See the following blog for its design:

[Contextual Memory Tracing][lnk_memory_trace]

*mmap\_byte\_reader.h*

This file contains the byte reading class template I implemented
modelling the Python approach.  It makes reading characters from a
file a simple loop.  This implementation uses memory-mapped file I/O.
Cf. *mmap\_line\_reader.h*.

*mmap\_line\_reader.h*

This file contains the line reading class template I implemented
modelling the Python approach.  It makes reading lines from a file a
simple loop.  This implementation uses memory-mapped file I/O.  Cf.
*istream\_line\_reader.h*, *file\_line\_reader.h*, and
*mmap\_byte\_reader.h*.

See the following blogs for the motivation and example code:

[Performance of My Line Readers][lnk_line_readers]  
[C/C++ Performance, `mmap`, and `string_view`][lnk_memory_trace]

*mmap\_line\_view.h*

A `mmap_line_view` is similar to `mmap_line_reader_sv` defined in
*mmap\_line\_reader.h*, but it has gone an extra mile to satisfy the
`view` concept (to be introduced in C++20).  It can be trivially copied.

*mmap\_reader\_base.cpp*  
*mmap\_reader\_base.h*

A class that wraps the difference of memory-mapped file I/O between Unix
and Windows.  It is used by `mmap_byte_reader` and `mmap_line_reader`.

*object\_level\_lock.h*

The Loki `ObjectLevelLockable` adapted to use the `fast_mutex` layer.
The member function `get_locked_object` does not exist in Loki, but is
also taken from Mr Alexandrescu's article.  Cf. *class\_level\_lock.h*.

*pctimer.h*

A function to get a high-resolution timer for Win32/Cygwin/Unix.  It is
useful for measurement and optimization, and can be easier to use than
`std::chrono::high_resolution_clock` after the advent of C++11.

*set\_assign.h*

Utility routines to make up for the fact that STL only has `set_union`
(+) and `set_difference` (-) algorithms but no corresponding += and -=
operations available.

*split.h*

Implementation of a split routine that allows efficient and lazy split
of a `string` (or `string_view`).  It takes advantage of the C++17
`string_view`, and the split result can be either used on the fly or
returned as a vector.

While the ranges library provides a similar function, compiling with
ranges is typically much slower—of course, it is much more powerful as
well.

*static\_mem\_pool.cpp*  
*static\_mem\_pool.h*

A memory pool implementation to pool memory blocks according to
compile-time block sizes.  Macros are provided to easily make a class
use pooled new/delete.

An article on its design and implementation is available at

[Design and Implementation of a Static Memory Pool][lnk_static_mem_pool]

*stdio\_wostream.h*

A workaround solution to address the limitation that `std::cout` and
`std::wcout` may conflict on some platforms.  It is used by `wcout` and
`wcerr`.

*trace\_stack.h*

A stack-like container adaptor, with the additional capability of
showing the last popped "stack trace".  It is used by *context.cpp*.

*tree.h*

A generic tree class template along with traversal utilities.  Besides
the usual template argument of value type, it has an additional argument
of storage policy, which can be either *unique* or *shared*.  Traversal
utility classes are provided so that traversing a tree can be simply
done in a range-based for loop.  The test code, *test/test\_tree.cpp*,
shows its basic usage.

*wcerr.cpp*  
*wcerr.h*

This provides a working `nvwa::wcerr` object that can coexist with
`std::cerr`.

*wcout.cpp*  
*wcout.h*

This provides a working `nvwa::wcout` object that can coexist with
`std::cout`.


[lnk_leakage]:         http://wyw.dcweb.cn/leakage.htm
[lnk_static_mem_pool]: http://wyw.dcweb.cn/static_mem_pool.htm
[lnk_functional_note]: https://yongweiwu.wordpress.com/2014/12/07/study-notes-functional-programming-with-cplusplus/
[lnk_y_combinator]:    https://yongweiwu.wordpress.com/2014/12/14/y-combinator-and-cplusplus/
[lnk_type_deduction]:  https://yongweiwu.wordpress.com/2014/12/29/type-deduction-and-my-reference-mistakes/
[lnk_generic_lambdas]: https://yongweiwu.wordpress.com/2015/01/03/generic-lambdas-and-the-compose-function/
[lnk_coroutines]:      https://yongweiwu.wordpress.com/2016/08/16/python-yield-and-cplusplus-coroutines/
[lnk_line_readers]:    https://yongweiwu.wordpress.com/2016/11/12/performance-of-my-line-readers/
[lnk_memory_trace]:    https://yongweiwu.wordpress.com/2017/09/14/c-cxx-performance-mmap-and-string_view/
[lnk_memory_trace]:    https://yongweiwu.wordpress.com/2022/01/26/contextual-memory-tracing/


<!--
vim:autoindent:expandtab:formatoptions=tcqlm:textwidth=72:
-->
