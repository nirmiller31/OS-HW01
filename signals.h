#ifndef SMASH__SIGNALS_H_
#define SMASH__SIGNALS_H_

#define SIGKILL 9
#define SIGTERM 15
#define SIGINT  2

void ctrlCHandler(int sig_num);

#endif //SMASH__SIGNALS_H_
