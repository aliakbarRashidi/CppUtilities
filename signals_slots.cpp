#include "signals_slots.h"

namespace CppUtilities {

#ifdef SIGSOT_TRACKING

static SignalTracker *inst = nullptr;
bool has_been_created = false;

SignalTracker::~SignalTracker()
{
    inst = nullptr;
}

SignalTracker *SignalTracker::get()
{
    if (inst)
        return inst;

    if (!has_been_created) {
        inst = new SignalTracker;
        return inst;
    }

    return nullptr;
}

bool SignalTracker::accessible()
{
    return inst != nullptr;
}

#endif

/* Template type predefs */
template class GenericFunctor<void>;
template class GenericExecutor<void>;
template class SignalMulti<void>;

}
