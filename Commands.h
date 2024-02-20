#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>


#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define WHITESPACE " "
//aaaaaaaaaaaaaaaaaa
class SmallShell;


enum CommandType {
    FGEXTERNAL,
    BGEXTERNAL,
    BUILTIN,
    PIPE,
    REDIRECTION
};

class Command {
// TODO: Add your data members
public:
    Command(const char* cmd_line);
    virtual ~Command() {};
    virtual void execute() = 0;
    std::vector<std::string> args;
    int num_of_args;
    std::string cmd_line;
    CommandType type;
    int pid;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line) : Command(cmd_line) { this->type = BUILTIN;};
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char* cmd_line, CommandType type) : Command(cmd_line) { this->type = type;};
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line) : Command(cmd_line) { this->type = PIPE;};
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line) : Command(cmd_line) {this->type = REDIRECTION;};
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line) {};
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class chpromptCommand : public BuiltInCommand {
public:
    chpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {};
    JobsList* jobs;
    virtual ~QuitCommand() {}
    void execute() override;
};




class JobsList {
public:
    class JobEntry {
    public:
        enum STATUS{
            RUNNING,
            STOPPED,
            FINISHED
        };
        int job_id;
        bool bg, toDelete;
        Command* cmd;
        time_t startTime;
        JobEntry(int job_id, bool bg, Command* cmd) : job_id(job_id), bg(bg), toDelete(false), cmd(cmd) {}
        // TODO: Add your data members
    };
    // TODO: Add your data members
public:
    std::vector<JobEntry*> jobs;
    int num_of_jobs, max_job_id;
    JobsList();
    ~JobsList() = default;
    void addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    void removeJobById(int jobId);
    JobEntry * getLastJob();
    JobEntry *getLastStoppedJob(int *jobId);
    int findPidFromJobId(int job_id);
    int findJobIdFromPid(int pid);
    JobEntry* findJob(int job_id);
    int findJobIndex(int job_id);
    void setJobPid(Command* cmd, int pid);
    int moveJobToFront(int job_id);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {};
    JobsList* jobs;
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs) :  BuiltInCommand(cmd_line), jobs(jobs) {};
    JobsList* jobs;
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs) :  BuiltInCommand(cmd_line), jobs(jobs) {};
    JobsList* jobs;
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class ChmodCommand : public BuiltInCommand {
public:
    ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
    virtual ~ChmodCommand() {}
    void execute() override;
};


class SmallShell {
private:
    // TODO: Add your data members

public:
    SmallShell();
    int last_job_id;
    JobsList jobs_list;
    std::string prompt;
    std::string curr_dir;
    std::string prev_dir;
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
