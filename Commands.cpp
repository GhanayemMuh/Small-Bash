#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

JobsList::JobsList() {
    num_of_jobs = 0;
    max_job_id = 0;
}

SmallShell::SmallShell() : jobs_list() {
    last_job_id = 0;
    prompt = "smash";
    char tmp[COMMAND_ARGS_MAX_LENGTH+1];
    if (getcwd(tmp, 200) == nullptr) {
        perror("smash error: getcwd failed");
        return;
    }
    curr_dir = tmp;
    prev_dir = "";
}

SmallShell::~SmallShell() = default;
// TODO: add your implementation


int JobsList::findPidFromJobId(int job_id) {
    for (std::vector<JobsList::JobEntry*>::iterator it = jobs.begin() ; it != jobs.end(); ++it) {
        if ((*it)->job_id == job_id) {
            return (*it)->cmd->pid;
        }
    }
    return -1;
}


int JobsList::findJobIdFromPid(int pid) {
    for (std::vector<JobsList::JobEntry*>::iterator it = jobs.begin() ; it != jobs.end(); ++it) {
        if ((*it)->cmd->pid == pid) {
            return (*it)->job_id;
        }
    }
    return -1;
}


JobsList::JobEntry* JobsList::findJob(int job_id) {
    for (std::vector<JobsList::JobEntry*>::iterator it = jobs.begin() ; it != jobs.end(); ++it) {
        if ((*it)->job_id == job_id) {
            return *it;
        }
    }
    return nullptr;
}


int JobsList::findJobIndex(int job_id) {
    int  i = 0;
    for (std::vector<JobsList::JobEntry*>::iterator it = jobs.begin() ; it != jobs.end(); ++it) {
        if ((*it)->job_id == job_id) {
            return i;
        }
        i++;
    }
    return -1;
}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    char** args = new char*[COMMAND_MAX_ARGS];
    char t[strlen(cmd_line) + 1];
    strcpy(t, cmd_line);
    _removeBackgroundSign(t);
    strcpy(t, _trim(string(t)).c_str());
    int i = _parseCommandLine(t, args);
    string cmd_name = args[0];
    if (i == 0) {
        return nullptr;
    }
    else if (cmd_s.find('>') != string::npos) {
        Command* cmd_to_return = new RedirectionCommand(cmd_line);
        return cmd_to_return;
    }
    else if (cmd_s.find('|') != string::npos) {
        Command* cmd_to_return = new PipeCommand(cmd_line);
        return cmd_to_return;
    }
    else if (cmd_name == "chprompt") {
        if (i < 1) {
            //num enough args
            return nullptr;
        }
        Command* cmd_to_return = new chpromptCommand(cmd_line);
        return cmd_to_return;
    }
    else if(cmd_name == "showpid") {
        Command* cmd_to_return = new ShowPidCommand(cmd_line);
        return cmd_to_return;
    }
    else if (cmd_name == "pwd") {
        Command* cmd_to_return = new GetCurrDirCommand(cmd_line);
        return cmd_to_return;
    }
    else if (cmd_name == "cd") {
        if (i > 2) {
            std::cerr << "smash error: cd: too many arguments" << std::endl;
            return nullptr;
        }
        if (i < 2) {
            return nullptr;
        }
        if ((std::string)args[1] == "-" && prev_dir.empty()) {
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
            return nullptr;
        }
        Command* cmd_to_return = new ChangeDirCommand(cmd_line, nullptr);
        return cmd_to_return;
    }
    else if (cmd_name == "jobs") {
        Command* cmd_to_return = new JobsCommand(cmd_line, &jobs_list);
        return cmd_to_return;
    }
    else if (cmd_name == "fg") {
        if (i > 1) {
            try {
                stoi(args[1]);
            }
            catch (exception &err) {
                std::cerr << "smash error: fg: invalid arguments" << std::endl;
                return nullptr;
            }
            bool found = false;

            for (std::vector<JobsList::JobEntry*>::iterator it = jobs_list.jobs.begin() ; it != jobs_list.jobs.end(); ++it) {
                if ((*it)->job_id == stoi(args[1])) {
                    found = true;
                    break;
                }
            }
            if (found == false) {
                std::cerr << "smash error: fg: job-id " << args[1] << " does not exist" << std::endl;
                return nullptr;
            }
        }
        if (i > 2) {
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            return nullptr;
        }
        else {
            if (jobs_list.num_of_jobs == 0) {
                std::cerr << "smash error: fg: jobs list is empty" << std::endl;
                return nullptr;
            }
        }
        Command* cmd_to_return = new ForegroundCommand(cmd_line, &jobs_list);
        return cmd_to_return;
    }
    else if (cmd_name == "quit") {
        Command* cmd_to_return = new QuitCommand(cmd_line, &jobs_list);
        return cmd_to_return;
    }
    else if (cmd_name == "kill") {
        if (i < 2) {
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return nullptr;
        }
        try {

            stoi(args[2]);
        }
        catch (exception &err) {
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return nullptr;
        }

        bool found = false;
        for (std::vector<JobsList::JobEntry*>::iterator it = jobs_list.jobs.begin() ; it != jobs_list.jobs.end(); ++it) {
            if ((*it)->job_id == stoi(args[2])) {
                found = true;
                break;
            }
        }
        if (found == false) {
            std::cerr << "smash error: kill: job-id " << args[2] << " does not exist" << std::endl;
            return nullptr;
        }
        try {
            stoi(args[1]);
        }
        catch (exception &err) {
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return nullptr;
        }
        if ((args[1][0]) != 45) { //45 is ascii for -
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return nullptr;
        }
        if (i > 3) {
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return nullptr;
        }
        Command* cmd_to_return = new KillCommand(cmd_line, &jobs_list);
        return cmd_to_return;
    }
    else {
        CommandType type;
        bool bg_cmd = _isBackgroundComamnd(cmd_line);
        if (bg_cmd) {
            type = BGEXTERNAL;
        }
        else {
            type = FGEXTERNAL;
        }
        Command* cmd_to_return = new ExternalCommand(cmd_line, type);
        return cmd_to_return;
    }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    jobs_list.removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);
    if (cmd == nullptr) {
        return;
    }
    jobs_list.removeFinishedJobs();
    jobs_list.addJob(cmd);

    cmd->execute();
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}


//-------------------------------------------------------------------------------------------------------------------------



Command::Command(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    char** args = new char*[COMMAND_MAX_ARGS];
    char t[strlen(cmd_line) + 1];
    strcpy(t, cmd_line);
    _removeBackgroundSign(t);
    strcpy(t, _trim(string(t)).c_str());
    int i = _parseCommandLine(t, args);
    for (int j = 0; j < i; j++){
        this->args.push_back(args[j]);
    }
    this->num_of_args = i;
    this->cmd_line = cmd_line;
    pid = getpid();
}

void JobsList::addJob(Command *cmd, bool isStopped) {
    if (cmd->type != BGEXTERNAL && cmd->type != FGEXTERNAL) {
        return;
    }
    JobEntry* new_job = new JobEntry(max_job_id + 1, false, cmd);
    jobs.push_back(new_job);
    num_of_jobs++;
    max_job_id++;
}


void chpromptCommand::execute() {
    if (num_of_args == 1) {
        SmallShell::getInstance().prompt = "smash";
    }
    else{
        SmallShell::getInstance().prompt = args[1];
    }

}


void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}


void GetCurrDirCommand::execute() {
    std::cout << SmallShell::getInstance().curr_dir << std::endl;
}


void ChangeDirCommand::execute() {
    if (args[1] == "-") {
        int result = chdir(SmallShell::getInstance().prev_dir.c_str());
        if (result == -1) {
            perror("smash error: chdir failed");
            return;
        }

        SmallShell::getInstance().prev_dir = SmallShell::getInstance().curr_dir;
        char tmp[COMMAND_ARGS_MAX_LENGTH];
        char* result_c = getcwd(tmp, 200);
        if (result_c == nullptr) {
            perror("smash error: getcwd failed");
            return;
        }
        SmallShell::getInstance().curr_dir = tmp;
    }
    else if (args[1] == "..") {
        std::size_t found = args[1].find_last_of("/");
        int result = chdir(args[1].substr(0,found).c_str());
        if (result == -1) {
            perror("smash error: chdir failed");
            return;
        }
        SmallShell::getInstance().prev_dir = SmallShell::getInstance().curr_dir;
        //SmallShell::getInstance().curr_dir = args[1].substr(0,found);
        char tmp[COMMAND_ARGS_MAX_LENGTH];
        char* result_c = getcwd(tmp, 200);
        if (result_c == nullptr) {
            perror("smash error: getcwd failed");
            return;
        }
        SmallShell::getInstance().curr_dir = tmp;
    }
    else {
        int result = chdir(args[1].c_str());
        if (result == -1) {
            perror("smash error: chdir failed");
            return;
        }
        SmallShell::getInstance().prev_dir = SmallShell::getInstance().curr_dir;
        //SmallShell::getInstance().curr_dir = args[1];
        char tmp[COMMAND_ARGS_MAX_LENGTH];
        char* result_c = getcwd(tmp, 200);
        if (result_c == nullptr) {
            perror("smash error: getcwd failed");
            return;
        }
        SmallShell::getInstance().curr_dir = tmp;
    }
}


int JobsList::moveJobToFront(int job_id) {
    JobEntry* job = findJob(job_id);
    int i = findJobIndex(job_id);
    jobs.erase(std::next(jobs.begin(),i));
    jobs.push_back(job);
    job->job_id = max_job_id + 1;
    max_job_id++;
    return 0;
}



void ForegroundCommand::execute() {
    int status;
    if (num_of_args == 1) {
        JobsList::JobEntry* job = jobs->getLastJob();
        job->cmd->type = FGEXTERNAL;
        std::cout << job->cmd->cmd_line << " " << job->cmd->pid << std::endl;
        int result = waitpid(job->cmd->pid, &status, 0);
        job->toDelete = true;
        if (result == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        jobs->removeJobById(job->job_id);
    }
    else {
        int job_id = stoi(args[1]);
        JobsList::JobEntry* job = jobs->findJob(job_id);
        job->cmd->type = FGEXTERNAL;
        std::cout << job->cmd->cmd_line << " " << job->cmd->pid << std::endl;
        jobs->moveJobToFront(job_id);
        int result = waitpid(job->cmd->pid, &status, 0);
        job->toDelete = true;
        if (result == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        jobs->removeJobById(stoi(args[1]));
    }

}


void JobsCommand::execute() {

    SmallShell::getInstance().jobs_list.printJobsList();
}


void JobsList::printJobsList() {

    for (std::vector<JobsList::JobEntry*>::iterator it = jobs.begin() ; it != jobs.end(); ++it) {
        std::cout << "[" << (*it)->job_id << "] " << (*it)->cmd->cmd_line << std::endl;
    }
}


void QuitCommand::execute() {
    jobs->removeFinishedJobs();
    if (num_of_args == 1) {
        exit(0);
    }
    else if (args[1] == "kill") {
        std::cout << "smash: sending SIGKILL signal to " << jobs->num_of_jobs << " jobs:" << std::endl;
        for (std::vector<JobsList::JobEntry *>::iterator it = jobs->jobs.begin(); it != jobs->jobs.end(); ++it) {
            std::cout << (*it)->cmd->pid << ": " << (*it)->cmd->cmd_line << std::endl;
            if (kill((*it)->cmd->pid, SIGKILL) == -1) {
                perror("smash error: kill failed");
                return;
            }
        }
        jobs->removeFinishedJobs();
        exit(0);
    }
}


void KillCommand::execute() {
    int signal = stoi(args[1].substr(1).c_str());
    int pid = jobs->findPidFromJobId(stoi(args[2]));
    if (pid == -1) {
        return;
    }
    if (kill(pid, signal) == -1) {
        perror("smash error: kill failed");
        return;
    }
    std::cout << "signal number " << signal << " was sent to pid " << pid << std::endl;
}


bool isCommandComplex(std::string cmd) {
    bool found1 = true, found2= true;
    if(cmd.find_first_of("?") == string::npos) {
        found1 = false;
    }
    if(cmd.find_first_of("*") == string::npos) {
        found2 = false;
    }
    if (found1 || found2) {
        return true;
    }
    else {
        return false;
    }
}


void ExternalCommand::execute() {
    int pid = fork();
    if(pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (type == FGEXTERNAL && isCommandComplex(cmd_line) == false) {
        if (pid == 0) {
            setpgrp();
            char *Args[num_of_args + 1];
            Args[num_of_args] = NULL;
            for (int i = 0; i < num_of_args; i++) {
                Args[i] = new char[args[i].length()];
                strcpy(Args[i], args[i].c_str());
            }
            execvp(Args[0], Args);
            perror("smash error: execvp failed");
            for (int i = 0; i < num_of_args; i++) {
                delete[] Args[i];
            }
            exit(1);
        }
        else {
            this->pid = pid;
            int status;
            waitpid(pid,&status,0);
            SmallShell::getInstance().jobs_list.removeJobById(SmallShell::getInstance().jobs_list.getLastJob()->job_id);
        }
    }
    else if (type == BGEXTERNAL && isCommandComplex(cmd_line) == false) {
        if (pid == 0) {
            setpgrp();
            char *Args[num_of_args + 1];
            Args[num_of_args] = NULL;
            for (int i = 0; i < num_of_args; i++) {
                Args[i] = new char[args[i].length() + 1];
                char t[args[i].length() + 1];
                strcpy(t, args[i].c_str());
                _removeBackgroundSign(t);
                strcpy(Args[i], _trim(t).c_str());
            }
            execvp(Args[0], Args);
            perror("smash error: execvp failed");
            for (int i = 0; i < num_of_args; i++) {
                delete[] Args[i];
            }
            exit(1);
        }
        else {
            this->pid = pid;
        }
    }
    else if (type == FGEXTERNAL && isCommandComplex(cmd_line) == true) {
        if (pid == 0) {
            setpgrp();
            char t[cmd_line.length()];
            strcpy(t, cmd_line.c_str());
            _removeBackgroundSign(t);
            char *Args[] = {(char *)"/bin/bash", (char *)"-c", t, NULL};
            if(execv("/bin/bash", Args) == -1) {
                perror("smash error: execv failed");
                exit(1);
            }
            exit(0);
        }
        else {
            this->pid = pid;
            waitpid(pid,nullptr,0);
            SmallShell::getInstance().jobs_list.removeJobById(SmallShell::getInstance().jobs_list.getLastJob()->job_id);
        }
    }
    else if (type == BGEXTERNAL && isCommandComplex(cmd_line) == true) {
        if (pid == 0) {
            setpgrp();
            char t[cmd_line.length()];
            strcpy(t, cmd_line.c_str());
            _removeBackgroundSign(t);
            char *Args[] = {(char *)"/bin/bash", (char *)"-c", t, NULL};
            if(execv("/bin/bash", Args) == -1) {
                perror("smash error: execv failed");
                exit(1);
            }
            exit(0);
        }
        else {
            this->pid = pid;
        }
    }
}


void JobsList::removeFinishedJobs() {
    int p, status;
    for (std::vector<JobsList::JobEntry*>::iterator it = jobs.begin() ; it != jobs.end(); ++it) {
        p = waitpid((*it)->cmd->pid, &status, WNOHANG);
        if (p > 0 && (WIFEXITED(status) || WIFSIGNALED(status))) {
            int job_id = findJobIdFromPid(p);
            removeJobById(job_id);
            it--;
            continue;
        }
        if ((*it)->toDelete) {
            int job_id = findJobIdFromPid((*it)->cmd->pid);
            removeJobById(job_id);
            it--;
            continue;
        }
    }
}


void JobsList::removeJobById(int jobId) {
    JobEntry* job = findJob(jobId);
    int prev_job_id = 0;
    for (std::vector<JobsList::JobEntry*>::iterator it = jobs.begin() ; it != jobs.end(); ++it) {
        if ((*it)->job_id == jobId) {
            if (jobId == max_job_id) {
                max_job_id = prev_job_id;
            }
            num_of_jobs--;
            jobs.erase(it);
            break;
        }
        prev_job_id = (*it)->job_id;
    }
    delete(job);
}


void JobsList::setJobPid(Command *cmd, int pid) {
    for (std::vector<JobsList::JobEntry*>::iterator it = jobs.begin() ; it != jobs.end(); ++it) {
        if (&((*it)->cmd) == &cmd) {
            (*it)->cmd->pid = pid;
            break;
        }
    }
}

JobsList::JobEntry* JobsList::getLastJob() {
    if (num_of_jobs == 0) {
        return nullptr;
    }
    return findJob(SmallShell::getInstance().jobs_list.max_job_id);
}


void RedirectionCommand::execute() {
    if (cmd_line.find(">>") == string::npos) {
        int pos = cmd_line.find_first_of('>');
        std::string cmd = _trim(cmd_line.substr(0, pos));
        std::string file_name = _trim(cmd_line.substr(pos + 1));
        int fd = open(file_name.c_str(), O_TRUNC | O_CREAT | O_WRONLY, 0666);
        if (fd == -1) {
            perror("smash error: open failed");
            return;
        }
        int s_fd = dup(1);
        if (s_fd == -1) {
            perror("smash error: dup failed");
            return;
        }
        if (dup2(fd, 1) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        SmallShell::getInstance().executeCommand(cmd.c_str());
        if (close(fd) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (dup2(s_fd, 1) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(s_fd) == -1) {
            perror("smash error: close failed");
            return;
        }
    } else {
        int pos = cmd_line.find(">>");
        std::string cmd = _trim(cmd_line.substr(0, pos));
        std::string file_name = _trim(cmd_line.substr(pos + 2));
        int fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd == -1) {
            perror("smash error: open failed");
            return;
        }
        int s_fd = dup(1);
        if (s_fd == -1) {
            perror("smash error: dup failed");
            return;
        }
        if (dup2(fd, 1) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        SmallShell::getInstance().executeCommand(cmd.c_str());
        if (close(fd) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (dup2(s_fd, 1) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(s_fd) == -1) {
            perror("smash error: close failed");
            return;
        }
    }
}

void PipeCommand::execute() {
    int idx = cmd_line.find("|&");
    string inp_cmd, outp_cmd;
    int myPipe[2];
    int fd;

    if(idx == (signed)string::npos){
        inp_cmd = cmd_line.substr(cmd_line.find("|")+1);
        outp_cmd = cmd_line.substr(0,cmd_line.find("|"));
        fd = 1; // stdout
    }else{
        inp_cmd = cmd_line.substr(cmd_line.find("|")+2);
        outp_cmd = cmd_line.substr(0,cmd_line.find("|"));
        fd = 2; // stderr
    }
    pid_t PPid = pid;
    pid_t son = fork();
    if(son == -1){
        perror("smash error: fork failed");
        return;
    }
    if(son == 0) {
        setpgrp();
        pid = getppid();
        if (pipe(myPipe) == -1) {
            perror("smash error: pipe failed");
            exit(1);
        }

        pid_t Reader = fork();
        if (Reader == -1) {
            perror("smash error: fork failed");
            exit(1);
        }
        if (Reader == 0) { // Reads
            if (close(myPipe[1]) == -1) {
                perror("smash error: close failed");
                exit(1);
            }
            if (dup2(myPipe[0], 0) == -1) {
                perror("smash error: dup2 failed");
                exit(1);
            }
            if (close(myPipe[0]) == -1) {
                perror("smash error: close failed");
                exit(1);
            }
            Command *CMD = SmallShell::getInstance().CreateCommand(inp_cmd.c_str());
            CMD->pid = PPid;
            CMD->execute();
            exit(0);
        }
        pid_t Writer = fork();
        if (Writer == -1) { // Writes
            perror("smash error: fork failed");
            exit(1);
        }
        if (Writer == 0) {
            if (close(myPipe[0]) == -1) {
                perror("smash error: close failed");
                exit(1);
            }
            if (dup2(myPipe[1], fd) == -1) {
                perror("smash error: dup2 failed");
                exit(1);
            }
            if (close(myPipe[1]) == -1) {
                perror("smash error: close failed");
                exit(1);
            }
            Command *CMD = SmallShell::getInstance().CreateCommand(outp_cmd.c_str());
            CMD->pid = PPid;
            CMD->execute();
            exit(0);
        }
        if (close(myPipe[0] == -1)) {
            perror("smash error: close failed");
            return;
        }
        if (close(myPipe[1]) == -1) {
            perror("smash error: close failed");
            return;
        }
        waitpid(Reader, nullptr,WUNTRACED);
        waitpid(Writer, nullptr,WUNTRACED);
        exit(0);
    }
    waitpid(son, nullptr,WUNTRACED);
}

void ChmodCommand::execute() {
    if (this->args.size() != 3) {
        cerr << "smash error: chmod: invalid arguments" << endl;
        return;
    }

    const string& newModeStr = args[1];
    const string& pathToFile = args[2];

    int newMode;
    try {
        newMode = stoi(newModeStr, nullptr, 8);
    } catch (...) {
        cerr << "smash error: chmod: invalid arguments" << endl;
        return;
    }

    if (chmod(pathToFile.c_str(), newMode) == -1) {
        perror("smash error: chmod");
        return;
    }
}
