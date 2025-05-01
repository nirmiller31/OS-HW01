#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include "signals.h"
#include <algorithm>
#include <fcntl.h>
#include <regex>

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

bool _isSpecialExternalComamnd(const char *cmd_line) {
    int cmd_length = strlen(cmd_line);
    for(int i = 0; i<cmd_length ; i++){
      if(cmd_line[i] == '*' || cmd_line[i] == '?'){
        return true;
      }
    }
    return false;
}

SmallShell::SmallShell() {
      this->m_prompt = "smash";
      this->m_plastPwd = "";
      this->m_lastCmdLine = "";
      this->m_jobsList = new JobsList();
      this->m_pid = getpid();
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

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if(SmallShell::getInstance().alias_exist(firstWord.c_str())){                     // Check a case of an alias
    string alias_name = firstWord.c_str();                                          // If exist, than it is alias's name
    cmd_line = SmallShell::getInstance().get_command_by_alias(alias_name).c_str();  // Extract the alias's command 
    cmd_s = _trim(string(cmd_line));                                                // Repeat the same process for the alias
    firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  }

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
  else if (firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line, getJobsList());
  }
  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line, getJobsList());
  }
  else if (firstWord.compare("alias") == 0) {
    return new AliasCommand(cmd_line);
  }
  else if (firstWord.compare("unalias") == 0) {
    return new UnAliasCommand(cmd_line);
  }
  else if (firstWord.compare("unsetenv") == 0) {
    return new UnSetEnvCommand(cmd_line);
  }
  else if (firstWord.compare("watchproc") == 0) {
    return new WatchProcCommand(cmd_line);
  }

  else {
    return new ExternalCommand(cmd_line);
  }

    return nullptr;
}

//****************************************************************************************************************************//
// ---------------------------------------------Small Shell method section------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void SmallShell::executeCommand(const char *cmd_line) {
  
    // TODO: Add your implementation here
    this->m_lastCmdLine = cmd_line;
    Command* cmd = CreateCommand(cmd_line);

  
    cmd->execute();

    // Please note that you must fork smash process for some commands (e.g., external commands....)
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
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ---------------------------------------------Built-in Command methods section------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

void BuiltInCommand::execute() {}                       // Maybe remove, if execute will be full

std::string ChPromtCommand::mask_chprompt(const char *cmd_line) {
  std::string tmp_string;
  tmp_string = string(cmd_line).substr(8);                                      // Take the "chprompt" out of the left side of the string
  tmp_string = _ltrim(tmp_string).length() ? _ltrim(tmp_string) : "smash";      // Take out the left spaces if exist, if empty take "smash"
  return tmp_string.substr(0, tmp_string.find_first_of(" \n"));                 // Take the first word from the string
}

//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ----------------------------------------------Change Prompt method section---------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

void ChPromtCommand::execute() {
  SmallShell::getInstance().set_prompt(m_argument);
}

//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// -------------------------------------------------Show PID methods section----------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

void ShowPidCommand::execute() {
  std::cout << "smash pid is " << getpid() << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// -----------------------------------------------Get Current Dir methods section-----------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

void GetCurrDirCommand::execute() {
  char pwd[200];
  getcwd(pwd, sizeof(pwd));
  std::cout << pwd << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ---------------------------------------------------Change Dir method section-------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

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
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// --------------------------------------------Quit Command methods section-----------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void QuitCommand::execute(){

  // pid_t main_pid = SmallShell::getInstance().get_pid();
  char* args[20]; 
  int argc = _parseCommandLine(m_cmdLine, args);
  if(argc == 1){
    exit(0);
  }
  else if(string(args[1]) == "kill"){
    int num_jobs = SmallShell::getInstance().getJobsList()->countLiveJobs();                // Verify if 0 jobs so it prints 0
    std::cout << "smash: sending SIGKILL signals to " << num_jobs << " jobs" << std::endl;
      m_jobs->printJobsListForKill();
      m_jobs->killAllJobs();
      exit(0);
  }
  else{
    // handle this senario (?) TODO
  }
}

const char* QuitCommand::take_second_arg(const char *cmd_line) {

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
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// --------------------------------------------Kill Command methods section-----------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void KillCommand::execute(){

  SmallShell::getInstance().getJobsList()->removeFinishedJobs();

  char* args[20]; 
  int argc = _parseCommandLine(m_cmdLine, args);

  int jobIsignal_num, jobId;
  JobsList::JobEntry* job_entry;

  jobIsignal_num = atoi(args[1]);
  if((argc == 3) && jobIsignal_num < 0){                                    // Verify correct number of arguments, and exsitance of '-' before the signal
    jobIsignal_num = abs(jobIsignal_num);
    jobId = atoi(args[2]);
    job_entry = SmallShell::getInstance().getJobsList()->getJobById(jobId);

    if(job_entry != nullptr){                                               // Check that a job with this ID exist
      std::cout << "signal number " << jobIsignal_num << " was sent to pid " << job_entry->getPid() << std::endl;
      SmallShell::getInstance().getJobsList()->removeJobById(jobId);
    }
    else {
      std::cout << "smash error: kill: job-id " << jobId << " does not exist" << std::endl;
    }
  }
  else{
    std::cout << "smash error: kill: invalid arguements" << std::endl;
  }
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// -------------------------------------------Alias Command methods section-----------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void AliasCommand::execute(){

  char* args[20]; 
  int argc = _parseCommandLine(m_cmdLine, args);
  std::regex pattern(R"(^alias\s+([a-zA-Z0-9_]+)='([^']*)'$)");
  std::cmatch match;

  if(argc == 1) {
    SmallShell::getInstance().print_alias();
  }
  else if (std::regex_match(m_cmdLine, match, pattern)) {

      std::string aliasStr = match[1];
      std::string commandStr = match[2];

      if(!SmallShell::getInstance().alias_is_reserved(aliasStr.c_str()) && !SmallShell::getInstance().alias_exist(aliasStr.c_str())){
        SmallShell::getInstance().add_alias(new SmallShell::Alias(aliasStr, commandStr));
      }
      else{
        std::cout << "smash error: alias: " << aliasStr << " already exists or is a reserved command" << std::endl;
      }

  } else {
      std::cout << "Invalid format.\n";
  }
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ------------------------------------------Unalias Command methods section----------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void UnAliasCommand::execute(){

  char* args[21]; 
  int argc = _parseCommandLine(m_cmdLine, args);
  if(argc == 1){
    std::cout << "smash error: unalias: not enough arguements" << std::endl;
  }
  else{
    for(int i = 1 ; i<argc ; i++){
      if(SmallShell::getInstance().alias_exist(args[i])){
        SmallShell::getInstance().delete_alias_by_name(args[i]);
      }
      else{
        std::cout << "smash error: unalias: " << args[i] << "alias does not exist" << std::endl;
      }
    }
  }
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// -----------------------------------------Unset Env Command methods section---------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void UnSetEnvCommand::execute(){

  char* args[21]; 
  int argc = _parseCommandLine(m_cmdLine, args);
  if(argc == 1){
    std::cout << "smash error: unsetenv: not enough arguements" << std::endl;
  }
  else{
    for(int i = 1 ; i<argc ; i++){
      if(is_environment_variable(args[i])){
        if (unsetenv(args[i]) != 0){                                                // it unsets the varaiable only for smash!!! not for bash. TODO check their intentions
          std::cout << "Failed to unset enviorment" << std::endl;
        }
      }
      else{
        std::cout << "smash error: unsetenv: " << args[i] << "does not exist" << std::endl;
      }
    }
  }
}

bool UnSetEnvCommand::is_environment_variable(const char* varName) {

  std::string path_to_check = "/proc/" + to_string(getpid()) + "/environ";
  int fd = open(path_to_check.c_str(), O_RDONLY);
  if (fd == -1) {
      perror("open");
      return;
  }

  const size_t bufferSize = 8192;                                       // 8 KB buffer
  char buffer[bufferSize];
  ssize_t bytesRead = read(fd, buffer, bufferSize);
  close(fd);

  string current_check = "";
  for(int i = 0 ; i<bytesRead ; i++){
    if(buffer[i] != '\0'){
      current_check += buffer[i];
    }
    else{
      std::cout << "Im checking: " << current_check << std::endl;
    }
  }
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ---------------------------------------Watch Procces Command methods section-------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void WatchProcCommand::execute(){

  char* args[21]; 
  int argc = _parseCommandLine(m_cmdLine, args);
  if(argc != 2 || atoi(args[1]) <= 0){
    std::cout << "smash error: watchproc: invalid arguements" << std::endl;
  }
  else{
    pid_t pid_to_print = atoi(args[1]);
    if(my_kill(pid_to_print, 0) == 0){                  // Process exist, and we have permission
      // TODO
    }
    else{
      std::cout << "smash error: watchproc: pid " << pid_to_print << " does not exist" << std::endl;
    }
  }
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ------------------------------------------External Command methods section---------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void ExternalCommand::execute(){

  char* args[21];                                               // To hold the cmd + [MAX]20 arguements
  
  char* shorterCmd = new char[strlen(m_cmdLine) + 1];
  strcpy(shorterCmd, m_cmdLine);                                // Create a shorter (pottentially) modifiable version

  bool background_command = _isBackgroundComamnd(m_cmdLine);    // Check if it is a background command (if "&" in the end)
  if(background_command){_removeBackgroundSign(shorterCmd);}    // Remove the "&" from shortherCmd, we don't need it anymore
  _parseCommandLine(shorterCmd, args);                          // Take the version without the "&" and divide it to an array

  pid_t pid = fork();                                           // Create a child process

  if(pid == 0){                                                 // New child process code

    if(_isSpecialExternalComamnd(shorterCmd)) {

      char* bash_exec[] = {"bash", "-c", shorterCmd, nullptr};  // Create an array to run: "bash -c "<original input>""
      if (execvp("bash", bash_exec) == -1){
        perror("execvp failed");
      }
    }
    if (execvp(args[0], args) == -1) {                          // Search for the command in PATH env, with our arguments
      perror("execvp failed");
    }
  }
  else if(pid >0){                                              // Parent process code
    if(!background_command){
      wait(nullptr);
    }
    else {
      SmallShell::getInstance().getJobsList()->addJob(this, pid); // If parent process: add the child's process Entry to jobs vector
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&TODO very important: make sure failed execv process wont get to vector!
      // std::cout << "The pid created is" << pid << std::endl;
    }
  }
  else{                                                         // Failed fork, may not need to print, but know it is here TODO
    std::cout << "Fork failed!" << std::endl;
    return;
  }
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//


bool SmallShell::alias_exist(const char* alias_name){
  for (auto it = m_aliasList.begin(); it != m_aliasList.end(); ++it) {
    if((*it) != nullptr) {
      if(string(alias_name) == ((*it)->get_name())){
        return true;
      }
    }
  }
  return false;
}

void SmallShell::print_alias(){
  for (auto it = m_aliasList.begin(); it != m_aliasList.end(); ++it) {
    if((*it != nullptr)){
      std::cout << (*it)->get_name().c_str() << "='" << (*it)->get_command().c_str() << "'" << std::endl;       // Print in the desired format
    }
  }
}

bool SmallShell::alias_is_reserved(const char* alias_name){
  std::string cmd = "command -v " + string(alias_name) + " > /dev/null 2>&1";
  int result = std::system(cmd.c_str());
  return bool(result == 0);
}

string SmallShell::get_command_by_alias(std::string alias_name){
  string result;
  for (auto it = m_aliasList.begin(); it != m_aliasList.end(); ++it) {
    if((*it) != nullptr) {
      if(alias_name == ((*it)->get_name())){
        result = (*it)->get_command();
      }
    }
  }
  return result;
}

void SmallShell::delete_alias_by_name(std::string alias_name){
  // std::vector<Alias*>* TMP_aliasList = new std::vector<Alias*>;
  for (auto it = m_aliasList.begin(); it != m_aliasList.end(); it++) {

    if((*it) != nullptr){
      if(alias_name == (*it)->get_name()){                    // If should be deleted
        // delete *it;                                           // Delete memory contant
        it = m_aliasList.erase(it);                           // Remove from vector
      }
      else{                                                   // If shouldn't be deleted
        // (*TMP_aliasList).push_back(*it);
      }
    }
  }
  // set_new_alias_list(TMP_aliasList);                          // Clean the deleted and make the list more compact
}

JobsList* SmallShell::getJobsList(){return this->m_jobsList;}

void JobsCommand::execute(){
    m_jobs->printJobsList();
}

//_____________________ Jobs List implemintation _____________________ //

JobsList::JobEntry::JobEntry(int newJobId, pid_t newJobPid, Command *cmd) : m_jobId(newJobId), m_pid(newJobPid), m_jobCmdLine(SmallShell::getInstance().getLastCmdLine()){

}

SmallShell::Alias::Alias(string new_alias_name, string new_alias_cmd) : m_aliasName(new_alias_name), m_aliasCommand(new_alias_cmd){

}

int JobsList::JobsList::countLiveJobs(){
  this->removeFinishedJobs();
  int result = 0;
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ++it) {
    if(*it) {result++;}
  }
  return result;
}

int JobsList::JobEntry::getJobId()const { return this->m_jobId;}

pid_t JobsList::JobEntry::getJobPid()const { return this->m_pid;}

pid_t JobsList::JobEntry::getPid()const {return this->m_pid;}

std::string JobsList::JobEntry::getJobCmdLine()const{return this->m_jobCmdLine;}


void JobsList::addJob(Command* cmd, pid_t pid, bool isStopped){
  this->removeFinishedJobs();
  JobEntry* new_entry = new JobEntry(this->maxJobId + 1 , pid , cmd);
  updateMaxJobID();
  jobsVector.push_back(new_entry);
}



void JobsList::printJobsList(){
  this->removeFinishedJobs();
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ++it) {
    std::cout << "[" << (*it)->getJobId() << "] " << (*it)->getJobCmdLine() << std::endl;
  }
}

void JobsList::printJobsListForKill(){
  this->removeFinishedJobs();
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ++it) {
    std::cout << (*it)->getJobPid() << ": " << (*it)->getJobCmdLine() << std::endl;
  }
  std::cout << "Linux-shell:" << std::endl;
}



void JobsList::killAllJobs(){
  this->removeFinishedJobs();
  int status;
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ++it) {
      if (*it == nullptr) continue; // skip null entries
      
      pid_t pid_to_kill = (*it)->getPid();
      my_kill(pid_to_kill, SIGKILL);
      // waitpid(pid_to_kill, nullptr, WNOHANG);   // TODO consider to use because of zombies
  }
}



//___remove all the finished jobs from the jobs list___//
void JobsList::removeFinishedJobs() {
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ) {
      int status;
      pid_t pid_to_delete = (*it)->getPid();
      pid_t result = waitpid(pid_to_delete, &status, WNOHANG);

      if (result == pid_to_delete) {
          delete *it;  // Free the memory of the JobEntry object
          it = jobsVector.erase(it); // Remove the pointer from the vector
      } else {
          ++it;
      }
  }

  // You can add other cleanup code here if needed
}



JobsList::JobEntry* JobsList::getJobById(int jobId) {
  auto it = std::find_if(jobsVector.begin(), jobsVector.end(),
      [jobId](const JobEntry* job) {
          return job->getJobId() == jobId;
      });

  if (it != jobsVector.end()) {
      return *it; // No need to take address
  }
  return nullptr;
}

  //ADD HERE 




void JobsList::removeJobById(int jobId) {
  pid_t pid_to_kill = SmallShell::getInstance().getJobsList()->getJobById(jobId)->getPid();
  if(my_kill(pid_to_kill, SIGKILL) < 0){                                        
    //handle with error with system call TODO
    std::cout << "kill failed" << std::endl;
  }
  waitpid(pid_to_kill, nullptr, 0);                             // Ensure no zombie to be created at this moment
  SmallShell::getInstance().getJobsList()->removeFinishedJobs();
}



JobsList::JobEntry* JobsList::getLastJob() {
  if(jobsVector.empty()){
    return nullptr;
  }
  
  return jobsVector.back();
}

/*
JobsList:: JobEntry *getLastStoppedJob(int *jobId) {
  //ADD HERE
}
*/

bool JobsList::empty()const {return jobsVector.empty();}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : m_jobs(jobs), m_cmdLine(cmd_line) {}

void ForegroundCommand::execute(){
  int status;                                                                                                   // Status for waitpid usage
  if(m_jobs != nullptr){
    this->m_jobs->removeFinishedJobs();

    char* args[20]; 
    int argc = _parseCommandLine(m_cmdLine, args);
    if(argc == 1){;
      if(m_jobs->empty()){
        std::cout << "smash error: fg: jobs list is empty" << std::endl;
      }
      else{
        JobsList::JobEntry* requastedJob = m_jobs->getLastJob();
        if( requastedJob != nullptr){                                                         // TODO consider writing it only once, more compact less readable
          pid_t result = waitpid(requastedJob->getPid(), &status, WNOHANG);                                                       // waitpid method
          std::cout << requastedJob->getJobCmdLine() << " " << requastedJob->getPid() << std::endl;                               // Print as declared in the PDF
          while(result != requastedJob->getPid()) {}                                                                              // Hold the process until child didn't end
        }
      }
    }

    else if(argc == 2){   //TODO consider using >2 instead of =
      int jobId = atoi(args[1]);
      if(jobId){
        JobsList::JobEntry* requastedJob = m_jobs->getJobById(jobId);
        if( requastedJob != nullptr){                                                                                              // Status for waitpid usage
          pid_t result = waitpid(requastedJob->getPid(), &status, WNOHANG);                                           // waitpid method
          std::cout << requastedJob->getJobCmdLine() << " " << requastedJob->getPid() << std::endl;                   // Print as declared in the PDF
          while(result != requastedJob->getPid()) {}                                                                  // Hold the process until child didn't end
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

