#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"



#include <sys/wait.h>


#include <regex>
#include <sys/utsname.h>

#include <sys/stat.h>
#include <pwd.h>


using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    string str = "smash: got ctrl-C\n";
    write(2,str.c_str(),str.length());

    pid_t pid = SmallShell::getInstance().get_Pid();

    if (pid > 0) {
        kill(pid, SIGKILL);

        string str = "smash: process " + to_string(pid) + " was killed\n";
        write(2,str.c_str(),str.length());
    }
}
