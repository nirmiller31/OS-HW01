#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    
    pid_t fg_pid = SmallShell::getInstance().get_fg_pid();

    std::cout<<"smash: got ctrl-C"<<std::endl;
    if(SmallShell::getInstance().get_pid() == fg_pid){
        return;
    }
    
    if(my_kill(fg_pid, SIGKILL) != 0){
        perror("smash error: kill");
    }

    std::cout<<"smash: process "<< fg_pid << " was killed" << std::endl;
    return;
}

int my_kill(pid_t pid, int sig) {
    return ::kill(pid, sig);
}
