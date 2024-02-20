#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    SmallShell::getInstance().jobs_list.removeFinishedJobs();
    cout << "smash: got ctrl-C" << endl;
    JobsList::JobEntry* currJob = SmallShell::getInstance().jobs_list.getLastJob();
    if (!currJob) {
        return;
    }
    Command* currCmd = currJob->cmd;
    if (currCmd == NULL) return;
    if (currCmd->type == FGEXTERNAL){
        int pid = currCmd->pid;
        if (kill(pid, SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << pid << " was killed" << endl;
        currJob->toDelete = true;
    }

    currCmd = NULL;
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}

