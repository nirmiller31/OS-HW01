// Ver: 10-4-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <sys/types.h>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define DISK_USAGE_BLOCK_SIZE (512)
#define USED_BUFFER_SIZE (8192)

class Command {
public:
    Command(const char *cmd_line) {};
    Command() {};

    virtual ~Command();

    virtual void execute() = 0;

};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line) {};
    BuiltInCommand() {};

    virtual ~BuiltInCommand();

    virtual void execute() override;
};

class ExternalCommand : public Command {
    std::string m_cmdLine;
public:
    ExternalCommand(const char *cmd_line) : Command(cmd_line){m_cmdLine = std::string(cmd_line);};

    virtual ~ExternalCommand() {};

    void execute() override;
};


class RedirectionCommand : public Command {
    
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand();

    void execute() override;

private:
    Command* m_command;
    std::string m_file_name;
    bool append_to_end;

};

class PipeCommand : public Command {
    
    
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand();

    void execute() override;

private:
    Command* m_first_command;
    Command* m_second_command;
    bool pipe_stderr;
};

class DiskUsageCommand : public Command {
private:
    const char* m_cmdLine;
public:
#define DT_DIR     4

struct linux_dirent64 {
    ino64_t        inode;
    off64_t        offset;
    unsigned short record_length;
    unsigned char  entry_type;
    char           entry_name[]; // Flexible array
};

    DiskUsageCommand(const char *cmd_line)  {m_cmdLine = cmd_line;}

    virtual ~DiskUsageCommand() {
    }

    void execute() override;

    int sum_directory_files(const char* dirPath);
};

class WhoAmICommand : public Command {
private:
    const char* m_cmdLine;
public:
    WhoAmICommand(const char *cmd_line) {m_cmdLine = cmd_line;}

    virtual ~WhoAmICommand() {
    }

    void execute() override;

    void get_user_and_path_by_uid(uid_t uid, std::string result[2]);
};

class NetInfo : public Command {
private:
    const char* m_cmdLine;
public:

    NetInfo(const char *cmd_line) {m_cmdLine = cmd_line;}

    virtual ~NetInfo() {
    }

    void execute() override;

    bool interface_exist(std::string input_interface_name);

    void print_netinfo(std::string input_interface_name);

    std::string get_IP_for_interface(std::string input_interface_name);

    std::string get_subnet_mask_for_interface(std::string input_interface_name);

    std::string get_dg_for_interface(std::string input_interface_name);

    void print_DNS_servers();
    
};

class ChangeDirCommand : public BuiltInCommand {
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
public:
    QuitCommand(const char *cmd_line, JobsList *jobs) {
        m_cmdLine = cmd_line;
        m_jobs = jobs;
    }

    virtual ~QuitCommand() {
    }

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
    UnAliasCommand(const char *cmd_line) {m_cmdLine = cmd_line;}

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
private:
    const char* m_cmdLine;
public:
    UnSetEnvCommand(const char *cmd_line) {m_cmdLine = cmd_line;}

    virtual ~UnSetEnvCommand() {
    }

    void execute() override;

    bool is_environment_variable(const char* varName);
};

class WatchProcCommand : public BuiltInCommand {
private:
    const char* m_cmdLine;
public:
    WatchProcCommand(const char *cmd_line) {m_cmdLine = cmd_line;}

    virtual ~WatchProcCommand() {
    }

    void execute() override;

    unsigned long long getProcCpuTime(pid_t pid);
    unsigned long long getTotalCpuTime();
    double getCpuUsage(pid_t pid);
    double getMemUsage(pid_t pid);
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

            bool is_stopped() {return m_stopped;}

            void set_stopped() {m_stopped = true;}
    
            std::string getJobCmdLine()const;
    
        private: 
            int m_jobId;
            pid_t m_pid;
            std::string m_jobCmdLine;
            bool m_stopped;
        };
    
    
    
    public:
        JobsList() = default;
    
        ~JobsList() = default ;
    
        void addJob(Command* cmd, pid_t pid , bool isStopped = false);

        bool job_exist(int job_ID_to_look);
    
        void printJobsList();
    
        void printJobsListForKill();
    
        void killAllJobs();
    
        void removeFinishedJobs();
    
        int countLiveJobs();
    
        JobEntry *getJobById(int jobId);
    
        JobEntry *getLastJob();
    
        JobEntry *getLastStoppedJob(int *jobId);
    
        void updateMaxJobID();
    
        bool empty()const;
        
    private:
        std::vector<JobEntry*> jobsVector;
        int maxJobId;
    
};

class SmallShell {
public:

struct linux_dirent64 {
    ino64_t        inode;
    off64_t        offset;
    unsigned short record_length;
    unsigned char  entry_type;
    char           entry_name[]; // Flexible array
};
    Command *CreateCommand(const char *input_cmd_line);

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
    std::string get_shell_pwd();
    JobsList* getJobsList();
    void executeCommand(const char *cmd_line);
    void unset_enviorment(std::string varName);

    uid_t get_shell_uid();
    pid_t get_pid() {return m_pid;}
    std::string getLastCmdLine()const;
    pid_t get_fg_pid(){return m_fg_pid;}
    void set_fg_pid(pid_t fg_pid){m_fg_pid = fg_pid;}
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
    pid_t m_fg_pid;
    
};

#endif //SMASH_COMMAND_H_