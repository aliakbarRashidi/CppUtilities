#include "debuging.h"
#include "cpputilities_global.h"

#ifdef THREAD_TRACKING
#include "threading.h"
#endif

#ifdef SIGSOT_TRACKING
#include "signals_slots.h"
#include <sstream>
#endif

#include <sys/types.h>

#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>

#include <execinfo.h> // for backtrace
#include <dlfcn.h>    // for dladdr
#include <cxxabi.h>   // for __cxa_demangle
#include <signal.h>
#include <mcheck.h>

const int MAX_STACK_FRAMES = 128;
bool active_mtrace = false;
bool active_debug = false;
std::string bin_path = "";

namespace CppUtilities {

std::string Backtrace(int skip = 1)
{
    void *callstack[128];
    const int nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
    char buf[1024];
    int nFrames = backtrace(callstack, nMaxFrames);
    char **symbols = backtrace_symbols(callstack, nFrames);

    std::ostringstream trace_buf;
    for (int i = skip; i < nFrames; i++) {
        Dl_info info;
        if (dladdr(callstack[i], &info) && info.dli_sname) {
            char *demangled = nullptr;
            int status = -1;
            if (info.dli_sname[0] == '_')
                demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n",
                     i, int(2 + sizeof(void*) * 2), callstack[i],
                     status == 0 ? demangled :
                     info.dli_sname == nullptr ? symbols[i] : info.dli_sname,
                     (char *)callstack[i] - (char *)info.dli_saddr);
            free(demangled);
        } else {
            snprintf(buf, sizeof(buf), "%-3d %*p %s\n",
                     i, int(2 + sizeof(void*) * 2), callstack[i], symbols[i]);
        }
        trace_buf << buf;
    }
    free(symbols);
    if (nFrames == nMaxFrames)
        trace_buf << "[truncated]\n";
    return trace_buf.str();
}

std::string get_path(char *argo)
{
        char exepath[1024 + 1] = {0};

        sprintf(argo, "/proc/%d/exe", getpid());
        readlink(argo, exepath, 1024);
        return std::string(exepath);
}


std::string execute_addr_l(std::string &addr)
{

    FILE *fp;
    std::string output;

    /* Open the command for reading. */
    std::string orig = "addr2line -C -f -e ";
    orig.append(bin_path);
    orig.append(" ");
    orig.append(addr);
    fp = popen(const_cast<char *>(orig.c_str()), "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    char path[1024];

    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        output.append(path);
    }

    /* close */
    pclose(fp);

    return output;
}

void print_cerr_thread_log()
{
#ifdef THREAD_TRACKING
    //Print specs here
    std::cerr << "\n--------[BEG] [THREAD TRACKING LOG]--------\n" << std::endl;
    std::cerr << "    > Total threads built: " << ThreadTracker::get()->next_id() -1 << std::endl;
    std::cerr << "    > Stopped ones [" << ThreadTracker::get()->get_stopped().size() << "/" << ThreadTracker::get()->get_running().size() << "] running ones" << std::endl;
    std::cerr << "\n--------[END] [THREAD TRACKING LOG]--------\n" << std::endl;
#endif
}

void print_sig_path(int sig)
{
#ifdef SIGSOT_TRACKING
    if (AbstractSignalTracking *target = dynamic_cast<AbstractSignalTracking *>(SignalTracker::get()->get_signal(sig))) {

        //Why no easier way?? ;|
        std::ostringstream stm;
        stm << (void * const)target;
        std::string addr = stm.str();

        std::cout << "SIGSOT_TRACKING > Address of [" << sig << "] [" << ((SSDSet *)target)->name() << "] : " << addr << std::endl;
        std::cout << "                  Log:\n" << execute_addr_l(addr) << std::endl;
    } else {
        std::cout << "SIGSOT_TRACKING > Was not able to find [" << sig << "] for logs." << std::endl;
    }
#endif
}

void handleSignals [[ noreturn ]] (int sig)
{
    //First of all, flush all the streams!
    fflush(stdout);
    fflush(stderr);
    fflush(stdin);

#ifndef FORCE_DEBUG
    if (active_debug) {
#endif
        print_cerr_thread_log();
#ifndef DEBUG_ALL_OUTS
        if (sig != SIGINT) { //No need to print anything as the user wanted to kill it! Normal.
#endif
            try {
                void *array[MAX_STACK_FRAMES];
                size_t size = 0;
                char **strings = nullptr;
                size_t i;
                signal(sig, SIG_DFL);
                size = static_cast<size_t>(backtrace(array, MAX_STACK_FRAMES));
                strings = backtrace_symbols(array, int(size));

                std::cerr << "\n-----------[BEG] [ADDR2LINE LOG]-----------\n" << std::endl;

                for (i = 0; i < size; ++i) {
                    std::string symbol(strings[i]);
                    size_t pos1 = symbol.find("[");
                    std::string address = symbol.substr(pos1 +1, symbol.find_last_of("]") -pos1 -1);

                    std::cerr << execute_addr_l(address) << std::endl;

                }
                std::cerr << "\n-----------[END] [ADDR2LINE LOG]-----------\n" << std::endl;

                std::cerr << "\n----------[BEG] [PROG OFFSET LOG]----------\n" << std::endl;
                std::cerr << Backtrace() << std::endl;
                std::cerr << "\n----------[END] [PROG OFFSET LOG]----------\n" << std::endl;
                free(strings);

            } catch (...) {
                /* ... can we really do smthg about it at all? ... */
            }
#ifndef DEBUG_ALL_OUTS
        }
#endif

        const char *sigVal;
        switch (sig) {
            case SIGTERM: sigVal = "SIGTERM"; break;
            case SIGILL: sigVal = "SIGILL"; break;
            case SIGSEGV: sigVal = "SIGSEGV"; break;
            case SIGINT: sigVal = "SIGINT"; break;
            case SIGABRT: sigVal = "SIGABRT"; break;
            case SIGFPE: sigVal = "SIGFPE"; break;
        }

        std::cerr << "RECEIVED SIGNAL: " << sigVal << std::endl;

        //Last flush :=[
        fflush(stdout);
        fflush(stderr);
        fflush(stdin);

#ifndef FORCE_DEBUG
    }
#endif

    if (active_mtrace) {
        muntrace();
    }

    exit(sig);
}

void setup_sig_handle(char *argo, bool mtrack, bool dbg)
{
    active_mtrace = mtrack;
    active_debug = dbg;

    bin_path = get_path(argo);

    signal(SIGTERM, handleSignals);
    signal(SIGSEGV, handleSignals);
    signal(SIGILL, handleSignals);
    signal(SIGINT, handleSignals);
    signal(SIGABRT, handleSignals);
    signal(SIGFPE, handleSignals);
}


void print_stack_trace()
{
    void *array[MAX_STACK_FRAMES];
    size_t size = 0;
    char **strings = nullptr;
    size_t i;
    size = static_cast<size_t>(backtrace(array, MAX_STACK_FRAMES));
    strings = backtrace_symbols(array, int(size));
    std::cout << "\n----------[BEG] [PROG CALL STACK]----------\n" << "\n-----------[BEG] [ADDR2LINE LOG]-----------\n" << std::endl;

    for (i = 0; i < size; ++i) {
        std::string symbol(strings[i]);
        size_t pos1 = symbol.find("[");
        std::string address = symbol.substr(pos1 +1, symbol.find_last_of("]") -pos1 -1);

        std::cerr << execute_addr_l(address) << std::endl;
    }

    std::cout << "\n-----------[END] [ADDR2LINE LOG]-----------\n" << "\n----------[BEG] [PROG OFFSET LOG]----------\n" << std::endl;
    std::cout << Backtrace() << std::endl;
    std::cout << "\n----------[END] [PROG OFFSET LOG]----------\n" << "\n----------[END] [PROG CALL STACK]----------\n" << std::endl;
}
}
