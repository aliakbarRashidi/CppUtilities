#pragma once

namespace CppUtilities {
void setup_sig_handle(char *argo, bool mtrack, bool dbg);
void print_cerr_thread_log();
void print_sig_path(int sig);

void print_stack_trace();
}
