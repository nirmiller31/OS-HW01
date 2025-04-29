#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
}

int my_kill(pid_t pid, int sig) {
    return ::kill(pid, sig);
}
