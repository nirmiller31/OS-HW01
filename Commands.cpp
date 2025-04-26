#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <algorithm>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
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

SmallShell::SmallShell() {
      this->m_prompt = "smash";
      this->m_plastPwd = "";
      this->m_lastCmdLine = "";
      this->m_jobsList =new JobsList();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
delete m_jobsList;
}

std::string SmallShell::getLastCmdLine()const {return this->m_lastCmdLine;}

Command::~Command() {}


BuiltInCommand::~BuiltInCommand() {}
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("chprompt") == 0) {
    return new ChPromtCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand();
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand();
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, get_last_dir());
  }
  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line, getJobsList());
  }
   else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line, getJobsList());
  }

  else {
    return new ExternalCommand(cmd_line);
  }

    return nullptr;
}


void SmallShell::executeCommand(const char *cmd_line) {
  
    // TODO: Add your implementation here
    this->m_lastCmdLine = cmd_line;
    Command* cmd = CreateCommand(cmd_line);

  
    cmd->execute();

    // Please note that you must fork smash process for some commands (e.g., external commands....)
}


void BuiltInCommand::execute() {}                       // Maybe remove, if execute will be full

std::string ChPromtCommand::mask_chprompt(const char *cmd_line) {
  std::string tmp_string;
  tmp_string = string(cmd_line).substr(8);                                      // Take the "chprompt" out of the left side of the string
  tmp_string = _ltrim(tmp_string).length() ? _ltrim(tmp_string) : "smash";      // Take out the left spaces if exist, if empty take "smash"
  return tmp_string.substr(0, tmp_string.find_first_of(" \n"));                 // Take the first word from the string
}


void ChPromtCommand::execute() {
  SmallShell::getInstance().set_prompt(m_argument);
}

void ShowPidCommand::execute() {
  std::cout << "smash pid is " << getpid() << std::endl;
}

void GetCurrDirCommand::execute() {
  char pwd[200];
  getcwd(pwd, sizeof(pwd));
  std::cout << pwd << std::endl;
}

const char* ChangeDirCommand::take_second_arg(const char *cmd_line) {

  const char* tmp_cmd_line;
    
    while (*cmd_line && std::isspace(*cmd_line)) ++cmd_line;                // Skip leading spaces
    while (*cmd_line && !std::isspace(*cmd_line)) ++cmd_line;               // Skip the first word
    while (*cmd_line && std::isspace(*cmd_line)) ++cmd_line;                // Skip spaces after the first word

    tmp_cmd_line = cmd_line;
//TODO verify that path is coherent!!!!!!!!!!
    while (*tmp_cmd_line && !std::isspace(*tmp_cmd_line)) ++tmp_cmd_line;   // Skip the given path
    while (*tmp_cmd_line && std::isspace(*tmp_cmd_line)) ++tmp_cmd_line;    // Skip the spaces after the path
    if(*tmp_cmd_line) {                                                     // Check if something left after: [cd][spaces][<path>][spaces][---something?---]
      std::cout << "smash error: cd: too many arguements" << std::endl;
    }
    return *cmd_line ? cmd_line : nullptr;
}

void ChangeDirCommand::execute() {

  char pwd[200];
  getcwd(pwd, sizeof(pwd));
  std::string old_string = std::string(pwd);

  if (std::string(m_newDir).compare("-") == 0) {                            // Check if special key activated
    const char* last_dir = *m_lastDir;                                      // Only use for the previous path
    if(std::string(last_dir).compare("") == 0) {
      std::cout << "smash error: cd: OLDPWD not set" << std::endl;
      return;
    }
    else if(chdir(last_dir) != 0){                                          // Change dir to previous saved path
      perror("smash error: chdir failed");
    }
    else {
      SmallShell::getInstance().set_last_dir(old_string);                   // Succesful change, current path will be the old one
    }
  }
  else{                                                                     // Special key wasnt activated
    if(chdir(m_newDir) != 0){
      perror("smash error: chdir failed");
      }
    else{
      SmallShell::getInstance().set_last_dir(old_string);                   // Succesful change, current path will be the old one
    }
  }
}

char** SmallShell::get_last_dir() {
  
  size_t last_path_size = m_plastPwd.length();                              // Get path's length
  char** result = new char*[2];                                             // Pointer gen for string & nullptr
  result[0] = new char[last_path_size + 1];                                 // Allocte for string
  for(size_t i=0 ; i<last_path_size ; i++){
    result[0][i] = m_plastPwd[i];
  }
  result[0][last_path_size] = '\0';
  result[1] = nullptr;                                                      // Close with nullptr

  return result;
}


JobsList* SmallShell::getJobsList(){return this->m_jobsList;}


void JobsCommand::execute(){
    m_jobs->printJobsList();
}



//_____________________ Jobs List implemintation _____________________ //

JobsList::JobEntry::JobEntry(int newJobId, pid_t newJobPid, Command *cmd) : m_jobId(newJobId), m_pid(newJobPid), m_jobCmdLine(SmallShell::getInstance().getLastCmdLine()){}

int JobsList::JobEntry::getJobId()const { return this->m_jobId;}

pid_t JobsList::JobEntry::getPid()const {return this->m_pid;}

std::string JobsList::JobEntry::getJobCmdLine()const{return this->m_jobCmdLine;}

/*
void JobsList::addJob(Command *cmd, bool isStopped){
  this->removeFinishedJobs();
  jobsVector.push_back(new JobEntry(this->maxJobId + 1 ,  , cmd))

  //ADD HERE
}

*/

void JobsList::printJobsList(){
  this->removeFinishedJobs();
  for (auto it = jobsVector.begin(); it != jobsVector.end(); it++){
    std::cout << "[" << it->getJobId() << "]" << it->getJobCmdLine() << std::endl;
  }
  //ADD HERE
}


/*
void JobsList::killAllJobs(){
  //ADD HERE
}
*/


//___remove all the finished jobs from the jobs list___//
void JobsList::removeFinishedJobs(){
   for (auto it = jobsVector.begin(); it != jobsVector.end(); ){
    int status;
    pid_t result = waitpid(it->getPid(), &status, WNOHANG);
    
    if(result == it->getPid()){
      it = jobsVector.erase(it);
    }
    else{
      ++it;
    }
   } 

  //ADD HERE 
}  




JobsList::JobEntry* JobsList::getJobById(int jobId){
  std::cout << "got here" << std::endl;
  auto it = std::find_if(jobsVector.begin(), jobsVector.end(),[jobId](const JobEntry& job){return job.getJobId() == jobId;});
  if(it != jobsVector.end()){
    return &(*it);
  }
    return nullptr;
}
  //ADD HERE 



/*
void JobsList::removeJobById(int jobId) {
  //ADD HERE
}

*/
JobsList::JobEntry* JobsList::getLastJob() {
  if(jobsVector.empty()){
    return nullptr;
  }
  
  return &jobsVector.back();
}

/*
JobsList:: JobEntry *getLastStoppedJob(int *jobId) {
  //ADD HERE
}
*/

bool JobsList::empty()const {return jobsVector.empty();}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : m_jobs(jobs), m_cmdLine(cmd_line) {}

void ForegroundCommand::execute(){
  if(m_jobs != nullptr){
    this->m_jobs->removeFinishedJobs();

    char* args[20]; 
    int argc = _parseCommandLine(m_cmdLine, args);
    if(argc == 1){;
      if(m_jobs->empty()){
        std::cout << "smash error: fg: jobs list is empty" << std::endl;
      }
      // ADD CODE FG WITH MAX ID JOB
      else{
        std::cout << m_jobs->getLastJob()->getPid() << m_jobs->getLastJob()->getPid() << std::endl;
      }
    }

    else if(argc == 2){
      int jobId = atoi(args[1]);
      if(jobId){
        JobsList::JobEntry* requastedJob = m_jobs->getJobById(jobId);
        if( requastedJob != nullptr){
          //ADD CODE FOR REGULAR FG WITH PID
        }
        else{
          std::cout << "smash error: fg: job-id " << jobId << " does not exist" << std::endl;
        }
        
      }
      else{
        std::cout << "smash error: fg: invalid arguments" << std::endl;
      }
    }

    else{
      std::cout << "smash error: fg: invalid arguments" << std::endl;
    }
  }
}

void ExternalCommand::execute(){
  char* args[20]; 
  _parseCommandLine(m_cmdLine, args);
  for(int i = 1 ; i<20 ; i++){
    std::cout << "ExternalCommand" << args[i] << std::endl;
  }
}
