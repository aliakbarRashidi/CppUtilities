#include "cpputilities.h"

namespace CppUtilities {
bool __sscal_outputs_feature() {
#ifdef SSCALL_OUTPUTS
    return true;
#else
    return false;
#endif
}
bool __force_debug_feature() {
#ifdef FORCE_DEBUG
    return true;
#else
    return false;
#endif
}
bool __debug_all_outs_feature() {
#ifdef DEBUG_ALL_OUTS
    return true;
#else
    return false;
#endif
}
bool __sigsot_meta_use_feature() {
#ifdef SIGSOT_META_USE
    return true;
#else
    return false;
#endif
}
bool __thread_name_use_feature() {
#ifdef THREAD_NAME_USE
    return true;
#else
    return false;
#endif
}
}
