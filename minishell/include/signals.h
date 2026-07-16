#ifndef SIGNALS
#define SIGNALS
#include <signal.h>

extern volatile sig_atomic_t foreground_pgid;

void init_signal_handlers(void);
int wait_foreground_job(pid_t pgid);

#endif