# CppUtilities
C++ Utilities set. A library for threading, thread safe signal/slot system and binary debugging. It makes use of features available from C++11 like templates, std::function, std::bind.

## Prerequities
You need at least C++17 (but you can even with 11th, but there is "inline static"s) and libstd++

## Compile
Just use
```
$ qmake
$ make
```

## Classes and their debugging features
All debuging features can be found in cpputilities_global.h. If they are not enabled at compile time of the library, using debuging features in your application is an undefined behaviour. Classes provided when debuging enabled and not are not the same, but you can use both in their original way. Additional features can be added (like names for signals and slots). All signals are thread safe. YOU just have to put the RIGHT TARGET THREAD when constructing a ftor.
GenericFunctor, GenericExecutor and SingleLooping are classes meant of one time use. Passing a GenericFunctor or GenericExecutor to classes from the library are undefined behaviour. And ALWAYS use new () to provide a ftor or xtor as they are automatically deleted in the classes' internals.

### CppUtilities::ThreadLooping
This class lets you add routines, and can handle callbacks from signals over their callbacks and target threads.
Any callback and routine are xtors!
All callbacks are deleted after they have been called.
All routines are deleted when the thread is destroyed.

### CppUtilities::SingleLooping
This class can handle only one source function and handles callbacks too. It works the same way as std::thread(...): you create it and use it only one time.

### CppUtilities::GenericFunctor<class C, class ... Args> (ftor)
It handles a function of return type C, and arguments <Args ...>. If you want to use a member function, use std::bind and pass it as it was a basic function pointer.
This class can be passed in any signal that has the same arguments. If you make GenericFunctor<class C>, it can be passed in any signal, as in a signal, the return type of a functor does not matter. Moreover, you can connect GenericFunctor<class C> to any signal and use that as a notifier or such.
You can specify a name for it, a target CppUtilities::AbstractThread so that the function will be executed in the target thread, and not in the signal's call emit(...) thread.
+ Operator GenericExecutor<C> for GenericFunctor<C>
+ Operator GenericExecutor<void, Args ...> for GenericFunctor<C, Args ...> ---> You cannot recover the original return type
+ GenericFunctor<> ---> GenericFunctor<void> 

### CppUtilities::GenericExecutor<class C, class ... Args> (xtor)
You pass in a GenericFunctor and its arguments. Notice that it is as GenericExecutor<class C, class ... Args>(GenericFunctor<C, Args ...> *, Args ...), so you have to redefine the template for a functor. The xtor can have the name of the ftor passed in, directly use a func ptr and set a name, use std::bind when constructed. Its internal data cannot be changed after the ctor (except its name), and has no target thread.
+ Operator GenericExecutor<C> for GenericExecutor<C, Args ...>
+ Operator GenericExecutor<void> for GenericExecutor<C, Args ...> ---> You cannot recover the original return type
+ GenericExecutor<> ---> GenericExecutor<void>

## Introspection system
Each introspection systems use UID and getters, so you can get any of the supported object from its *Tracker class by ID.
  
### Signals/Slots System
Signals and slots have a tracking system. When SIGSOT_TRACKING enabled, the class AbstractSignalTracking and SignalTracker can be used. Created signals have a UID (int) to refer to the signal. You can enable tracking when any signal is registered, or on one (by passing its ID) with the class SignalTracker. If SSCALL_OUPUTS, when a callback (slot) is called (it can then create an xtor for the target thread or be directly executed) it will print a notice. This is implemented in all library's signal classes. But if you use 3rd-party implementation, they could not. When the signal is emitted, it prints a message (id and name if available).
You can get all the instanciated signals by using SignalTracker::get_availables().
Attention, SignalTracker::get() is static, if it is deleted, SignalTracker::accessible() will return false.

### Threading
Threads have an elaborated tracking system too. You can get the stopped threads, the running ones, every thread that is alive. Methods in ThreadTracking are close to the ones available in SignalTracker.

## The debuging
At your program startup, you have to pass arg 0 (char *), if mtrace is activated (bool) and if you want to enable debug at runtime (bool) in CppUtilities::setup_sig_handle. The function will setup additional data (e.g. the binary path) and the signal handlers. Its behaviour can be changed by using DEBUG_ALL_OUTS that will print data even if the event have been triggered by the user (SIGINT), or FORCE_DEBUG that will ignore if debug at runtime is enabled and print data. The data is composed of 1. The log built from addr2line results with the stack frames' addresses, and 2. The log of the stack frames' addresses, with wild binary path (on Linux, /proc/exe) or the library it comes from (e.g. glibc). Additional log is printed for running threads and sopped ones when THREAD_TRACKING is enabled. If in setup_sig_handles you have specified that mtrace is active, muntrace() will be called at end of the signal handler.
