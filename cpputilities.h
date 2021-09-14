#pragma once

#include "cpputilities_global.h"
#include "signals_slots.h"
#include "threading.h"
#include "debuging.h"

//Compile time "knowledge" of the flags. Compile time data does not guarantee that an app at runtime will have the same data. Whereas here, you're sure of what you have.
namespace CppUtilities {
bool __sscal_outputs_feature();
bool __force_debug_feature();
bool __debug_all_outs_feature();
bool __sigsot_meta_use_feature();
bool __thread_name_use_feature();
}
