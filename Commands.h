// Ver: 10-4-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
    // TODO: Add your data members
public:
    Command(const char *cmd_line) {};
    Command() {};

    virtual ~Command();

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line) {};
    BuiltInCommand() {};

    virtual ~BuiltInCommand();

    virtual void execute() override;
};

class ExternalCommand : public Command {
    const char* m_cmdLine;
public:
    ExternalCommand(const char *cmd_line) {
        m_cmdLine = cmd_line;
    };

    virtual ~ExternalCommand() {
    }

    void execute() override;
};


class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {
    }

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class DiskUsageCommand : public Command {
public:
    DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class NetInfo : public Command {
    // TODO: Add your data members **BONUS: 10 Points**
public:
    NetInfo(const char *cmd_line);

    virtual ~NetInfo() {
    }

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    // TODO: Add your data members public:
public:
    ChangeDirCommand(const char *cmd_line, char** plastPwd) {
        m_newDir = take_second_arg(cmd_line);
        m_lastDir = plastPwd;
    };

    virtual ~ChangeDirCommand() {}

    const char* take_second_arg(const char *cmd_line);

    void execute() override;
private:
    const char* m_newDir;
    char** m_lastDir;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand() = default;

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ChPromtCommand : public BuiltInCommand {
public:
    ChPromtCommand(const char *cmd_line) {
        m_argument = mask_chprompt(cmd_line);
    };

    virtual ~ChPromtCommand() {}

    std::string mask_chprompt(const char *cmd_line);
    void execute() override;
private:
    std::string m_argument;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand() = default;

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

class SmallShell;

class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members public:
public:
    QuitCommand(const char *cmd_line, JobsList *jobs) {
        m_cmdLine = cmd_line;
        m_jobs = jobs;
    }

    virtual ~QuitCommand() {
    }

    const char* take_second_arg(const char *cmd_line);

    void execute() override;

private:
    const char* m_cmdLine;
    JobsList* m_jobs;
};

class JobsCommand : public BuiltInCommand {
   
public:
    JobsCommand(const char *cmd_line, JobsList *jobs) {m_jobs = jobs;}

    virtual ~JobsCommand() {
    }

    void execute() override;

private:
    JobsList* m_jobs;

};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs){
        m_cmdLine = cmd_line;
        m_jobs = jobs;
    }

    virtual ~KillCommand() {
    }

    void execute() override;

private:
    const char* m_cmdLine;
    JobsList* m_jobs;
};

class ForegroundCommand : public BuiltInCommand {
   
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;

private:
    JobsList* m_jobs;
    const char* m_cmdLine;

};

class AliasCommand : public BuiltInCommand {
private:
    const char* m_cmdLine;
public:
    AliasCommand(const char *cmd_line) {m_cmdLine = cmd_line;}

    virtual ~AliasCommand() {
    }

    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
private:
    const char* m_cmdLine;
public:
    UnAliasCommand(const char *cmd_line) {m_cmdLine = cmd_line;};

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
public:
    UnSetEnvCommand(const char *cmd_line);

    virtual ~UnSetEnvCommand() {
    }

    void execute() override;
};

class WatchProcCommand : public BuiltInCommand {
public:
    WatchProcCommand(const char *cmd_line);

    virtual ~WatchProcCommand() {
    }

    void execute() override;
};

class JobsList {

    public:
        class JobEntry {
        public:
            JobEntry(int newJobId, pid_t newJobPid, Command *cmd);
    
            ~JobEntry(){}
    
            int getJobId()const;
    
            int getJobPid()const;
    
            pid_t getPid()const;
    
            std::string getJobCmdLine()const;
    
        private: 
            int m_jobId;
            pid_t m_pid;
            std::string m_jobCmdLine;
        };
    
    
    
    public:
        JobsList() = default;
    
        ~JobsList() = default ;
    
        void addJob(Command* cmd, pid_t pid , bool isStopped = false);
    
        void printJobsList();
    
        void printJobsListForKill();
    
        void killAllJobs();
    
        void removeFinishedJobs();
    
        int countLiveJobs();
    
        JobEntry *getJobById(int jobId);
    
        void removeJobById(int jobId);
    
        JobEntry *getLastJob();
    
        JobEntry *getLastStoppedJob(int *jobId);
    
        void updateMaxJobID() {maxJobId++;}
    
        bool empty()const;
    
        // TODO: Add extra methods or modify exisitng ones as needed
    
    private:
        std::vector<JobEntry*> jobsVector;
        int maxJobId;
    
};

class SmallShell {
public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    class Alias {
        private:
            std::string m_aliasName;
            std::string m_aliasCommand;
        public:
            Alias(std::string new_alias_name, std::string new_alias_cmd);
            ~Alias(){}
            std::string get_name() {return m_aliasName;}
            std::string get_command() {return m_aliasCommand;}
        };

    std::string get_prompt() {return this->m_prompt;}
    void set_prompt(std::string new_prompt) {this->m_prompt = new_prompt;}
    char** get_last_dir();
    void set_last_dir(std::string plastPwd) {this->m_plastPwd = plastPwd;}
    JobsList* getJobsList();
    void executeCommand(const char *cmd_line);
    pid_t get_pid() {return m_pid;}
    std::string getLastCmdLine()const;

    //---------------------------Alias Used----------------------------
    void add_alias(Alias* new_alias) {m_aliasList.push_back(new_alias);}
    bool alias_exist (const char* alias_name);
    bool alias_is_reserved(const char* alias_name);
    std::string get_command_by_alias(std::string alias_name);
    void delete_alias_by_name(std::string alias_name);
    void set_new_alias_list(std::vector<Alias*>* new_aliasList) {m_aliasList = *new_aliasList;}
    void print_alias();
    //-----------------------------------------------------------------

    private:
  
    SmallShell();

    std::string m_plastPwd;
    std::string m_prompt;
    std::string m_lastCmdLine;
    JobsList* m_jobsList;
    pid_t m_pid;
    std::vector<Alias*> m_aliasList;

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_