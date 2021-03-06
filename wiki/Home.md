# Introduction

**mcfgthread** is an _experimental_ support library for MinGW targets of GCC, providing basic `HANDLE`-style [thread](https://en.wikipedia.org/wiki/Thread_(computing)) manipulation, as well as native and lightweight in-process synchronization primitives including [_mutexes_](https://en.wikipedia.org/wiki/Mutex), [_condition variables_](https://en.wikipedia.org/wiki/Condition_variable) and _once-initialization flags_. These synchronization primitives, being as _lightweight_ as those on Linux which rely on [futex](https://en.wikipedia.org/wiki/Futex), consume few resources. In particular, they consume no resources other than the bytes they reside in.

It turns out that [Microsoft has plagiarized the Linux futex somehow](https://msdn.microsoft.com/en-us/library/windows/desktop/hh706898(v=vs.85).aspx), however these APIs are available only since Windows 8.

Windows 7 has the awesome _[slim reader/writer (SRW) locks](https://msdn.microsoft.com/en-us/library/windows/desktop/aa904937(v=vs.85).aspx)_, _[condition variables](https://msdn.microsoft.com/en-us/library/windows/desktop/ms682052(v=vs.85).aspx)_ and [one-time initialization](https://msdn.microsoft.com/en-us/library/windows/desktop/aa363808(v=vs.85).aspx), which, as usual, are over-specific for Microsoft people's own usage. Their _SRW locks_ do not support timed waits and their _condition variables_ do not support any mutexes other than _SRW locks_ or _critical sections_.

Behind _SRW locks_ and _condition variables_ are the _keyed events_, a powerful but otherwise not officially documented feature described in articles from [Lockless Inc.](http://locklessinc.com/articles/keyed_events/) and [Joe Duffy's Blog](http://joeduffyblog.com/2006/11/28/windows-keyed-events-critical-sections-and-new-vista-synchronization-features/). If Microsoft people haven't implemented timed mutexes or generic condition variables, then it is we that should implement them.

In addition to the set of native interfaces (those with the common prefix `_MCFCRT_`), a set of `__gthread_*` interfaces (implementation of most threading support in GCC libraries) and another set of C11 interfaces are provided, providing full threading support for GCC.

## Notes on Integration with GCC

1. If you wish to use **mcfgthread** without hacking GCC, you can use these APIs directly. You just need to install these headers and libraries, then write your program and compile and link it with `-lmcfgthread`, as what you typically do with other libraries.

2. If you wish to have GCC make use of **mcfgthread**, you must hack GCC source and build GCC from it, which is described below. It should be noted that certain construction of GCC and GCC libraries, for example, `__thread`, the C11 `_Thread_local`, the C++11 `thread_local` and the C++ exception handler, would require a gthread library to work. This is usually done with a header that redirects `__gthread_*()` calls to other libraries such as [winpthreads](https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-libraries/winpthreads/) or the [gthr-win32.c
one in libgcc](https://gcc.gnu.org/viewcvs/gcc/trunk/libgcc/config/i386/gthr-win32.c?view=markup), and here it is **mcfgthread** that does the trick.

3. According to the C11 standard, an implementation may, but is not otherwise required to, generate `SIGABRT`, `SIGFPE`, `SIGILL`, `SIGINT`, `SIGSEGV` or `SIGTERM`, except as a result of explicit calls to the `raise()` function. With a **mcfgthread** library that uses [Windows SEH](https://en.wikipedia.org/wiki/Microsoft-specific_exception_handling_mechanisms#Structured_Exception_Handling), such signals are not guaranteed to be generated, except as a result of explicit calls to the `raise()` function. In particular, the **mingw-w64** CRT sets up a [top-level exception handler](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680634(v=vs.85).aspx) upon program startup, which translates specific exceptions caught by SEH into signals. If you replace it using the `SetUnhandledExceptionFilter()` function, these exceptions, when generated by a thread created by `__gthread_create()` or `thrd_create()`, will not be translated into signals and will result in immediate abnormal termination.

## How to Integrate with GCC

1. Depending on whether you are using a toolchain targeting x86 or x64, copy the contents of `release/mingw32` or `release/mingw64` to the prefix directory of your toolchain (for example, saying your `gcc.exe` resides in `/foo/bar/bin`, the prefix directory is `/foo/bar`). The contents of directories `bin`, `include` and `lib` should merge with those of existent ones.  
_[ N.B. The prefix directories for x86 and x64 toolchain packages from the MSYS2 project are `/mingw32` and `/mingw64`, respectively. ]_

2. Patch GCC source code. This is usually done with `git am`. Different patches have been created for different branches. Patches available are:
    1. [This patch](https://raw.githubusercontent.com/lhmouse/MINGW-packages/master/mingw-w64-gcc-git/9000-gcc-5-branch-Added-mcf-thread-model-support-from-mcfgthread.patch) applies to GCC 5.
    2. [This patch](https://raw.githubusercontent.com/lhmouse/MINGW-packages/master/mingw-w64-gcc-git/9000-gcc-6-branch-Added-mcf-thread-model-support-from-mcfgthread.patch) applies to GCC 6.
    3. [This patch](https://raw.githubusercontent.com/lhmouse/MINGW-packages/master/mingw-w64-gcc-git/9000-gcc-7-branch-Added-mcf-thread-model-support-from-mcfgthread.patch) applies to GCC 7.
    4. [This patch](https://raw.githubusercontent.com/lhmouse/MINGW-packages/master/mingw-w64-gcc-git/9000-gcc-8-branch-Added-mcf-thread-model-support-from-mcfgthread.patch) applies to GCC 8.
    

3. Configure GCC with option `--enable-threads=mcf`, but without `objc` or `obj-c++` in `--enable-languages=`, as **Objective-C** interfaces are currently unimplemented and will cause **libobjc** to fail to build.

4. `make` it as usual.

## Distribution Notes

At the moment, **mcfgthread** can't be linked statically and the file `libmcfgthread.a` is a copy of `libmcfgthread.dll.a`. Hence, if you want to distribute a program linked againast **mcfgthread**, you must always distribute `mcfgthread-*.dll` with it.

The reason why **mcfgthread** can't be linked statically is a bit complex:

1. The gthread and c11 thread interfaces use global AVL trees to translate thread IDs into `HANDLE`s upon `*_join()` or `*_detach()` calls, which would be problematic if **mcfgthread** was linked statically, since each dynamic library as well as the executable would have a separated map. As a result, calls to such functions have to be done in the same module that has created the thread, otherwise they will fail with `ESRCH` and `thrd_error` respectively.

2. It is essential to use the DLL entry point function to clean up TLS objects and invoke thread exit callbacks. Without a dynamic library (for example, when **mingw-w64** is linked statically), a TLS callback is an option but it would require an `IMAGE_TLS_DIRECTORY` to work. The **mingw-w64** CRT places it in one of the startup files (crt?.o). It is also possible to place it in a static library then reference it somewhere in the startup files (if it is not referenced, the linker will ignore it or strip it). In both cases the startup code has to be modified which requires extra inter-project work that I am not very willing to bother myself to do.

## TODO List

See [Issues](https://github.com/lhmouse/mcfgthread/issues).
