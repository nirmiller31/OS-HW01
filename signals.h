#ifndef SMASH__SIGNALS_H_
#define SMASH__SIGNALS_H_

#define SIGKILL 9
#define SIGTERM 15
#define SIGINT  2

int my_kill(pid_t pid, int sig);

void ctrlCHandler(int sig_num);

#endif //SMASH__SIGNALS_H_
