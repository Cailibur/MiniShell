#define _POSIX_C_SOURCE 200112L
#include <signal.h> 
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "signals.h"
#include "job.h"
#include "shell.h"

//To store current working process group.
volatile sig_atomic_t foreground_pgid = 0;

static void handle_sigint(int sig) {
    (void)sig;
    printf("\n");
    if (foreground_pgid > 0) {
        kill(-foreground_pgid, SIGINT);
    }
}

static void handle_sigtstp(int sig) {
    (void)sig;
    printf(" Stopped progress %d\n", sig); 
    int tmp = add_job(foreground_pgid, line);
    stop_job(tmp);
    if (foreground_pgid > 0) {
        kill(-foreground_pgid, SIGTSTP);
    }
}

void init_signal_handlers(void) {
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal SIGINT");
    }

    if (signal(SIGTSTP, handle_sigtstp) == SIG_ERR) {
        perror("signal SIGTSTP");
    }
}

