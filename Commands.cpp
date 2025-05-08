#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iomanip>
#include "Commands.h"
#include "signals.h"
#include <algorithm>
#include <fcntl.h>
#include <regex>
#include <iomanip>

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


bool _isSpecialExternalComamnd(const char *cmd_line) {
    int cmd_length = strlen(cmd_line);
    for(int i = 0; i<cmd_length ; i++){
      if(cmd_line[i] == '*' || cmd_line[i] == '?'){
        return true;
      }
    }
    return false;
}


bool _isRediractionCommand(const char *cmd_line){
  std::string str_cmd_line(cmd_line);
  
  if(((str_cmd_line.find(">>")) != std::string::npos) || ((str_cmd_line.find(">")) != std::string::npos)){
    return true;
  }  
  else{
    return false;
  }
}
  


bool _isPipeCommand(const char *cmd_line){
  std::string str_cmd_line(cmd_line);
  
  if((str_cmd_line.find("|")) != std::string::npos){
    return true;
  }  
  else{
    return false;
  }
}
  


SmallShell::SmallShell() {
      this->m_prompt = "smash";
      this->m_plastPwd = "";
      this->m_lastCmdLine = "";
      this->m_jobsList = new JobsList();
      this->m_pid = getpid();
      this->m_fg_pid = m_pid;
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

  SmallShell::getInstance().getJobsList()->removeFinishedJobs();

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if(SmallShell::getInstance().alias_exist(firstWord.c_str())){                     // Check a case of an alias
    string alias_name = firstWord.c_str();                                          // If exist, than it is alias's name
    cmd_line = SmallShell::getInstance().get_command_by_alias(alias_name).c_str();  // Extract the alias's command 
    cmd_s = _trim(string(cmd_line));                                                // Repeat the same process for the alias
    firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  }
  if (firstWord.compare("alias") == 0) {
    return new AliasCommand(cmd_line);
  }
  else if(_isRediractionCommand(cmd_line)){
    return new RedirectionCommand(cmd_line);
  }
  else if(_isPipeCommand(cmd_line)){
    return new PipeCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
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
  else if (firstWord.compare("unalias") == 0) {
    return new UnAliasCommand(cmd_line);
  }
  else if (firstWord.compare("unsetenv") == 0) {
    return new UnSetEnvCommand(cmd_line);
  }
  else if (firstWord.compare("watchproc") == 0) {
    return new WatchProcCommand(cmd_line);
  }

  else if (firstWord.compare("whoami") == 0) {
    return new WhoAmICommand(cmd_line);
  }
  else if (firstWord.compare("du") == 0) {
    return new DiskUsageCommand(cmd_line);
  }
  else if (firstWord.compare("netinfo") == 0) {
    return new NetInfo(cmd_line);
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
  
    char* args[20]; 
    int argc = _parseCommandLine(cmd_line, args);
    if(argc <= 0){
    return;
    }
    else {
      this->m_lastCmdLine = cmd_line;
      Command* cmd = CreateCommand(cmd_line);
      cmd->execute();
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
  getcwd(pwd, sizeof(pwd));       // &&&&&&&&&&&&&&&& TODO probebly fixx
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
    while (*tmp_cmd_line && !std::isspace(*tmp_cmd_line)) ++tmp_cmd_line;   // Skip the given path
    while (*tmp_cmd_line && std::isspace(*tmp_cmd_line)) ++tmp_cmd_line;    // Skip the spaces after the path
    if(*tmp_cmd_line) {                                                     // Check if something left after: [cd][spaces][<path>][spaces][---something?---]
      std::cerr << "smash error: cd: too many arguements" << std::endl;
    }
    return *cmd_line ? cmd_line : nullptr;
}

void ChangeDirCommand::execute() {

  char pwd[200];
  getcwd(pwd, sizeof(pwd));       // &&&&&&&&&&&&&&&& TODO probebly fixx
  std::string old_string = std::string(pwd);

  if (std::string(m_newDir).compare("-") == 0) {                            // Check if special key activated
    const char* last_dir = *m_lastDir;                                      // Only use for the previous path
    if(std::string(last_dir).compare("") == 0) {
      std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
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

  char* args[20]; 
  int argc = _parseCommandLine(m_cmdLine, args);
  if(argc == 1){
    exit(0);
  }
  else if(string(args[1]) == "kill"){
    int num_jobs = SmallShell::getInstance().getJobsList()->countLiveJobs();
    std::cout << "smash: sending SIGKILL signal to " << num_jobs << " jobs:" << std::endl;
      m_jobs->printJobsListForKill();
      m_jobs->killAllJobs();
      exit(0);
  }
  else{
    // handle this senario (?) TODO
  }
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

  if((argc != 3) || (atoi(args[1]) >= 0) || (atoi(args[2]) <= 0)){
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
  }
  else{                                    // Verify correct number of arguments, and exsitance of '-' before the signal
    jobIsignal_num = atoi(args[1]);
    jobId = atoi(args[2]);
    jobIsignal_num = abs(jobIsignal_num);
    job_entry = SmallShell::getInstance().getJobsList()->getJobById(jobId);

    if(job_entry != nullptr){                                               // Check that a job with this ID exist
      std::cout << "signal number " << jobIsignal_num << " was sent to pid " << job_entry->getPid() << std::endl;
      if(kill(job_entry->getPid(), jobIsignal_num) != 0){
        std::cerr << "smash error: kill failed: Invalid argument" << std::endl;                                      // Probably still get the print!!
      }
    }
    else {
      std::cerr << "smash error: kill: job-id " << jobId << " does not exist" << std::endl;
    }
  }
  SmallShell::getInstance().getJobsList()->removeFinishedJobs();
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
        std::cerr << "smash error: alias: " << aliasStr << " already exists or is a reserved command" << std::endl;
      }

  } else {
      std::cerr << "Invalid format.\n";
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
    std::cerr << "smash error: unalias: not enough arguements" << std::endl;
  }
  else{
    for(int i = 1 ; i<argc ; i++){
      if(SmallShell::getInstance().alias_exist(args[i])){
        SmallShell::getInstance().delete_alias_by_name(args[i]);
      }
      else{
        std::cerr << "smash error: unalias: " << args[i] << "alias does not exist" << std::endl;
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
    std::cerr << "smash error: unsetenv: not enough arguements" << std::endl;
  }
  else{
    for(int i = 1 ; i<argc ; i++){
      if(is_environment_variable(args[i])){
        SmallShell::getInstance().unset_enviorment(args[i]);
      }
      else{
        std::cerr << "smash error: unsetenv: " << args[i] << "does not exist" << std::endl;
      }
    }
  }
}

void SmallShell::unset_enviorment(string varName) {

  for (char **env = __environ; *env != nullptr; ++env) {
        std::string env_var(*env);
        
        string string_to_search = varName + "=";                  // Check if the current environment variable starts with the desired key
        size_t pos = env_var.find(string_to_search.c_str());
        if (pos == 0) {                                           // If it's the variable we want to remove
            
            char **shift = env;                                   // Shift all following elements to remove this one
            while (*shift != nullptr) {
              *(shift) = *(shift + 1);
              shift++;
            }
            break;                                                // We found and removed the variable, so we can stop
        }
  }
}

bool UnSetEnvCommand::is_environment_variable(const char* varName) {
  

  std::string path_to_check = "/proc/" + to_string(SmallShell::getInstance().get_pid()) + "/environ";
  int fd = open(path_to_check.c_str(), O_RDONLY);
  if (fd == -1) {
      perror("smash error: open failed");
      return false;
  }

  const size_t bufferSize = 8192;                                       // 8 KB buffer
  char buffer[bufferSize];
  ssize_t bytesRead = read(fd, buffer, bufferSize);
  close(fd);

  string current_check = "";
  string debug_str = "";
  bool equal_flag = false;

  for(int i = 0 ; i<bytesRead ; i++){
    if(buffer[i] != '\0'){
      if(buffer[i] == '=') {equal_flag = true;}
      if(!equal_flag) {current_check += buffer[i];}
    }
    else{
      if(string(varName) == current_check) {
        return true;
      }
      equal_flag = false;                                                 // Reset the search
      current_check = "";
    }
  }
  return false;
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ---------------------------------------Watch Procces Command methods section-------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

 unsigned long long WatchProcCommand::getProcCpuTime(pid_t pid){ //!!!!!!check if return 0 is not an available value and if it is change the error return
  std::string stat_path = "/proc/" + to_string(pid) + "/stat";
  int fd = open(stat_path.c_str(), O_RDONLY );
  if(fd == -1){
    perror("smash error: open failed"); //check this might need to change it
    return 0.0;
  }

  char buffer[1024]; //check if enough
  ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
  close(fd);
  if (bytesRead <=0){
    perror("smash error: read failed"); //check this might need to change it
    return 0.0;
  }

  int i = 0;
  while((i < bytesRead) && (buffer[i] != ')')){
    i++;
  }
  if (i == bytesRead){
    return 0.0;
  }

  i += 2;
  int stat_field_number = 3;
  while(i < bytesRead && stat_field_number < 14){
    if (buffer[i] == ' '){
      stat_field_number++;
    }
    i++;
  }

  unsigned long long utime = 0;
  while (i < bytesRead && buffer[i] >= '0' && buffer[i] <= '9'){
    utime = (utime * 10) + (buffer[i] - '0'); //shifiting utime and adding the current digit (after converting char to int) - adding digit by digit form left to right 
    i++;
  }

  while (i < bytesRead && buffer[i] == ' '){
    i++;
  }
  
  unsigned long long stime = 0;
  while (i < bytesRead && buffer[i] >= '0' && buffer[i] <= '9'){
    stime = (stime * 10) + (buffer[i] - '0'); //shifiting utime and adding the current digit (after converting char to int) - adding digit by digit form left to right 
    i++;
  }
  
  return (stime + utime); //return the process with the given pid kernel mode time and user mode time 

};

unsigned long long WatchProcCommand::getTotalCpuTime(){
  int fd = open("/proc/stat", O_RDONLY );
  if(fd == -1){
    perror("smash error: open failed"); //check this might need to change it
    return 0.0;
  }

  char buffer[1024]; //check if enough
  ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
  close(fd);
  if (bytesRead <=0){
    perror("smash error: read failed"); //check this might need to change it
    return 0.0;
  }
  buffer[bytesRead] = '\0';
  const char* target_word = "cpu";
  static const int target_len = 3;
  int i = 0;

  while((i < bytesRead) && (strncmp(&buffer[i], target_word, target_len) != 0)){ 
    i++;
  }
  if (i == bytesRead){
    return 0.0;
  }

  while(buffer[i] < '0' || buffer[i] > '9'){
    i++; //skip the 'cpu' lable 
  }
   
  
  unsigned long long total_cpu_time = 0;
  unsigned long long curr_field_time = 0;
  int curr_field_number = 0;

  while(i < bytesRead && curr_field_number < 10){
    while(buffer[i] >= '0' && buffer[i] <= '9'){
      if (i >= bytesRead){
        return 0.0; //might need to change
      }
      curr_field_time = (curr_field_time * 10) + (buffer[i] - '0');
      i++;
    }
    total_cpu_time += curr_field_time;
    curr_field_time = 0;
    curr_field_number++;

    while (i < bytesRead && buffer[i] == ' '){
    i++;
    } 
  }
  return total_cpu_time;
};



double WatchProcCommand::getCpuUsage(pid_t pid){

  unsigned long long process_time1 = getProcCpuTime(pid);
  //std::cout << "Process time 1: " << process_time1 << std::endl;
  unsigned long long total_time1 = getTotalCpuTime();
  //std::cout << "Total time 1: " << total_time1 << std::endl;
  sleep(1);

  unsigned long long process_time2 = getProcCpuTime(pid);
  //std::cout << "Process time 2: " << process_time2 << std::endl;
  unsigned long long total_time2 = getTotalCpuTime();
  //std::cout << "Total time 2: " << total_time2 << std::endl;
  
  unsigned long long delta_process = process_time2 - process_time1;
  unsigned long long delta_total = total_time2 -total_time1;
  
  if (delta_total == 0){
    return 0.0;
  }
  return (double)delta_process/delta_total * 100.0;
};



double WatchProcCommand::getMemUsage(pid_t pid){
  std::string status_path = "/proc/" + to_string(pid) + "/status";
    int fd = open(status_path.c_str(), O_RDONLY );
    if(fd == -1){
      perror("smash error: open failed"); //check this might need to change it
      return 0;
    }

    char buffer[4048]; //check if enough
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytesRead <=0){
      perror("smash error: read failed"); 
      return 0;
    }
  buffer[bytesRead] = '\0';
  const char* target_line = "VmRSS:";
  size_t target_line_len = 6;
  size_t i = 0;
  while(i < (size_t)bytesRead){
    if(strncmp(&buffer[i], target_line, target_line_len) == 0){
      i += target_line_len;
      while(i < (size_t)bytesRead && buffer[i] == ' '){
        i++;
      }
      size_t start_index = i;
      while(i < (size_t)bytesRead && buffer[i] != '\n'){
        i++;
      }
      if (i == (size_t)bytesRead){
        return 0;
      }
      std::string return_str(&buffer[start_index],  i - start_index);
      // size_t first_num = return_str.find_first_of("0123456789");
      // size_t last_num = return_str.find_last_of("0123456789");
      return_str.find_first_of("0123456789");
      return_str.find_last_of("0123456789");
      double number = 0;
      for(size_t i = 0 ; i < return_str.size() ; i++){
          if(return_str.at(i) < '0' || return_str.at(i)> '9'){
            continue;
          }
          number = 10*number + (return_str.at(i) -'0');
      }
      //_trim(return_str)
       
      return number / 1024;
    }
    i++;
  }
  return 0;
};


void WatchProcCommand::execute(){

  char* args[21]; 
  int argc = _parseCommandLine(m_cmdLine, args);
  if(argc != 2 || atoi(args[1]) <= 0){
    std::cerr << "smash error: watchproc: invalid arguements" << std::endl;
  }
  else{
    pid_t pid_to_print = atoi(args[1]);
    if(kill(pid_to_print, 0) == 0){                  // Process exist, and we have permission
      //std::cout << std::fixed << setprecision(1);
      std::cout << "PID: " << pid_to_print << " | CPU Usage: " << getCpuUsage(pid_to_print) << "%" <<" | Memory Usage: " << getMemUsage(pid_to_print) <<" MB" << std::endl;

    }
    else{
      std::cerr << "smash error: watchproc: pid " << pid_to_print << " does not exist" << std::endl;
    }
  }
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ------------------------------------------Who Am I Command methods section---------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void WhoAmICommand::execute(){

  string result[2];

  uid_t shell_uid = SmallShell::getInstance().get_shell_uid();
  get_user_and_path_by_uid(shell_uid, result); 
  std::cout << result[0] << " " << result[1] << std::endl;
}

void WhoAmICommand::get_user_and_path_by_uid(uid_t shell_uid, string result[2]){

  std::string path_to_check = "/etc/passwd";
  int fd = open(path_to_check.c_str(), O_RDONLY);
  if (fd == -1) {
      perror("smash error: open failed");
      return;
  }

  const size_t bufferSize = 8192;                                       // 8 KB buffer
  char buffer[bufferSize];
  ssize_t bytesRead = read(fd, buffer, bufferSize);
  close(fd);

  bool username_read_enable = true;
  bool home_path_read_enable = false;
  bool uid_read_enable = false;
  int num_of_dots = 0;

  string username_string = "";
  string home_path_string = "";
  string uid_string = "";

  for(int i = 0 ; i<bytesRead ; i++){

    if(username_read_enable && (buffer[i] != ':')){         // Username is the first word until ':'
      username_string += buffer[i];
    }
    else{
      username_read_enable = false;                         // If we saw a ':' than first word over
    }

    if(buffer[i] == ':'){                                   // Count the word location
      num_of_dots++;
    }

    if (num_of_dots > 3){                                   // If we passed the third word (UID)
      uid_read_enable = false;
    }
    if(uid_read_enable){
      uid_string += buffer[i];
    }
    if(num_of_dots == 3){                                   // If we just started to read the third word
      uid_read_enable = true;
    }

    if (num_of_dots > 5){                                   // If we passed the fifth word (home path)
      home_path_read_enable = false;
    }
    if(home_path_read_enable){
      home_path_string += buffer[i];
    }
    if(num_of_dots == 5){                                   // If we just started to read the fifth word
      home_path_read_enable = true;
    }

    if(buffer[i] == '\n'){

      if(uid_t(atoi(uid_string.c_str())) == shell_uid){
        result[0] = username_string;
        result[1] = home_path_string;
        return;
      }
      username_string = "";                                 // Reset all to start a new line search
      username_read_enable = true;
      uid_string = "";
      home_path_string = "";
      num_of_dots = 0;
    }
  }
}

uid_t SmallShell::get_shell_uid(){

  char* args[5];

  std::string path_to_check = "/proc/" + to_string(SmallShell::getInstance().get_pid()) + "/status";
  int fd = open(path_to_check.c_str(), O_RDONLY);
  if (fd == -1) {
      perror("smash error: open failed");
      return false;
  }

  const size_t bufferSize = 8192;                                       // 8 KB buffer
  char buffer[bufferSize];
  ssize_t bytesRead = read(fd, buffer, bufferSize);
  close(fd);

  bool read_enable = false;
  string uid_string = "";

  for(int i = 0 ; i<bytesRead ; i++){
    if(buffer[i] == 'U' && buffer[i+1] == 'i' && buffer[i+2] == 'd' && buffer[i+3] == ':'){
      read_enable = true;
    }
    if(read_enable && buffer[i] == '\n'){
      read_enable = false;
    }
    if(read_enable){
      uid_string += buffer[i];
    }
  }
  
  _parseCommandLine(uid_string.c_str(), args);

  return atoi(args[1]);
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// -----------------------------------------Disk Usage Command methods section--------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void DiskUsageCommand::execute(){

  char* args[21]; 
  int argc = _parseCommandLine(m_cmdLine, args);

  if(argc == 1){
    std::cout << "Total disk usage: " << ((sum_directory_files(SmallShell::getInstance().get_shell_pwd().c_str())) / 1024.0) << " KB" << std::endl;
  }
  else if(argc == 2){

    int dir_FD = open(args[1], O_RDONLY | O_DIRECTORY);                                       //----------------------------------------------------
    if (dir_FD < 0) {                                                                         //
        std::cerr << "smash error: du: " << args[1] << " does not exist" << std::endl;        // Check exsitance, other function is recursive.
        return;                                                                               //
    }                                                                                         //
    close(dir_FD);                                                                            //-----------------------------------------------------

    std::cout << "Total disk usage: " << (sum_directory_files(args[1]) / 1024.0) << " KB" << std::endl;
  }
  else{
    std::cerr << "smash error: du: too many arguements" << std::endl;
  }
}


string SmallShell::get_shell_pwd(){
  char pwd[200];
  ssize_t len = syscall(SYS_readlink, "/proc/self/cwd", pwd, sizeof(pwd) - 1);
  if (len != -1) {
    pwd[len] = '\0';
  } else {
    std::cout << "Failed to read /proc/self/cwd\n";
  }
  return pwd;
}


int DiskUsageCommand::sum_directory_files(const char* dirPath) {

    int dir_FD = open(dirPath, O_RDONLY | O_DIRECTORY);
    if (dir_FD < 0) {
        return 0;
    }

    char buffer[8192];                                                                    // Buffer to hold directory entries
    int num_bytes_read;

    struct stat initial_file_stat;
    stat(dirPath, &initial_file_stat);

    int dir_sum = (initial_file_stat.st_blocks * 512);

    while ((num_bytes_read = syscall(SYS_getdents64, dir_FD, buffer, sizeof(buffer))) > 0) { // Read the directory contents into the buffer (SYS_getdents64 is 217 syscall)
        int current_position = 0;                                                         // Track position in the buffer
        
        while (current_position < num_bytes_read) {                                       // Process each directory entry
            linux_dirent64* dir_entry = (linux_dirent64*)(buffer + current_position);     // Cast to directory entry structure
            std::string current_entry_name = dir_entry->entry_name;                       // Get the name of the entry

            if (current_entry_name != "." && current_entry_name != "..") {                // Skip "." and ".." entries
                std::string current_entry_path = string(dirPath) + "/" + current_entry_name;
                // std::cout << current_entry_path << '\n';                                  // Print the full path of the entry (Debug)

                if (dir_entry->entry_type == DT_DIR) {                                    // If is a sub-directory
                    std::string sub_dir_path = std::string(dirPath) + "/" + current_entry_name;
                    dir_sum += sum_directory_files(sub_dir_path.c_str());                 // Recursive call for subdirectory
                }
                else{
                  struct stat file_stat;
                  stat(current_entry_path.c_str(), &file_stat);
                  dir_sum += (file_stat.st_blocks * 512);                                 // Calculation of disk usage
                }
            }
            current_position += dir_entry->record_length;                                 // Move to the next directory entry
        }
    }
    close(dir_FD);                                                                        // Close the directory
    return (dir_sum);
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ------------------------------------------Net Info Command methods section---------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void NetInfo::execute(){

  char* args[21]; 
  int argc = _parseCommandLine(m_cmdLine, args);

  if(argc == 1){
    std::cerr << "smash error: netinfo: interface not specified" << std::endl;
  }
  else if(argc == 2){
    if(interface_exist(args[1])){
      print_netinfo(args[1]);
    }
    else{
      std::cerr << "smash error: netinfo: interface " << args[1] << " does not exist" << std::endl;
    }
  }
  else{
    // TODO handle with it too
  }

}

void NetInfo::print_netinfo(string input_interface_name){
  std::cout << "IP address: " << get_IP_for_interface(input_interface_name) << std::endl;
  std::cout << "Subnet Mask: " << get_subnet_mask_for_interface(input_interface_name) << std::endl;
  std::cout << "Default Gateway: " << get_dg_for_interface(input_interface_name) << std::endl;
  print_DNS_servers();
}

string NetInfo::get_IP_for_interface(string input_interface_name){

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, input_interface_name.c_str(), IFNAMSIZ-1);

    if (syscall(SYS_ioctl, fd, SIOCGIFADDR, &ifr) < 0) {                    // Use syscall to invoke SYS_ioctl for SIOCGIFADDR
        close(fd);
        return "";
    }
 
    close(fd);

    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

}

string NetInfo::get_subnet_mask_for_interface(string input_interface_name){

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, input_interface_name.c_str(), IFNAMSIZ-1);

    if (syscall(SYS_ioctl, fd, SIOCGIFNETMASK, &ifr) < 0) {                    // Use syscall to invoke SYS_ioctl for SIOCGIFNETMASK
        close(fd);
        return "";
    }
 
    close(fd);

    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

}

string NetInfo::get_dg_for_interface(string input_interface_name){

  int fd = open("/proc/net/route", O_RDONLY);    // Open the file using open() system call
  if (fd == -1) {
      std::cerr << "Failed to open /etc/resolv.conf\n";
      return "";
  }

  const size_t bufferSize = 8192;                                       // 8 KB buffer
  char buffer[bufferSize];
  ssize_t bytesRead = read(fd, buffer, bufferSize);
  close(fd);

  char* args[bytesRead];
  int buffer_size = _parseCommandLine(buffer, args);

  for(int i = 0 ; i<buffer_size ; i++){
    if((string(args[i]) == input_interface_name) && (string(args[i+1]) == "00000000")){
      int result_0 = stoi(string({args[i+2][6], args[i+2][7]}), nullptr, 16);
      int result_1 = stoi(string({args[i+2][4], args[i+2][5]}), nullptr, 16); 
      int result_2 = stoi(string({args[i+2][2], args[i+2][3]}), nullptr, 16); 
      int result_3 = stoi(string({args[i+2][0], args[i+2][1]}), nullptr, 16); 
      return to_string(result_0) + "." + to_string(result_1) + "." + to_string(result_2) + "." + to_string(result_3); 
    }
  }
  return "";
}

void NetInfo::print_DNS_servers(){
    
  int fd = open("/etc/resolv.conf", O_RDONLY);    // Open the file using open() system call
  if (fd == -1) {
      std::cerr << "Failed to open /etc/resolv.conf\n";
      return;
  }

  const size_t bufferSize = 8192;                                       // 8 KB buffer
  char buffer[bufferSize];
  ssize_t bytesRead = read(fd, buffer, bufferSize);
  close(fd);

  int DNS_index = 0;

  char* args[bytesRead];
  int buffer_size = _parseCommandLine(buffer, args);

  for(int i = 0 ; i<buffer_size ; i++){
    if(string(args[i]) == "nameserver"){
      DNS_index = i+1;
    }
  }
  std::cout << "DNS Servers: " << args[DNS_index++];
  while(string(args[DNS_index]) != "options"){
    std::cout << ", " << args[DNS_index++];
  }
  std::cout << "\n";
  return;
}

bool NetInfo::interface_exist(string input_interface_name){

  std::string path_to_check = "/proc/net/dev";
  int fd = open(path_to_check.c_str(), O_RDONLY);
  if (fd == -1) {
      perror("smash error: open failed");
      return false;
  }

  const size_t bufferSize = 8192;                                       // 8 KB buffer
  char buffer[bufferSize];
  ssize_t bytesRead = read(fd, buffer, bufferSize);
  close(fd);

  string interface_name = "";
  int num_of_enter = 0;
  bool interface_read_enable = false;

  for(int i = 0 ; i<bytesRead ; i++){

    if(interface_read_enable && (buffer[i] != ':') && (num_of_enter > 1)){         // Interfaces names are the first word until ':', from the second line
      interface_name += buffer[i];
    }
    else{
      interface_read_enable = false;                         // If we saw a ':' than first word over
    }

    if(buffer[i] == '\n'){                                   // Count the word location
      num_of_enter++;
    }

    if(buffer[i] == '\n'){
      if(_trim(interface_name) == input_interface_name){
        return true;
      }
      interface_name = "";                                 // Reset all to start a new line search
      interface_read_enable = true;
    }
  }
  return false;
}
//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ------------------------------------------External Command methods section---------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void ExternalCommand::execute(){
  char* args[21];                                               // To hold the cmd + [MAX]20 arguements
  char* shorterCmd = new char[m_cmdLine.size() + 1];
  strcpy(shorterCmd, m_cmdLine.c_str());                                // Create a shorter (pottentially) modifiable version

  // int pipefd[2];
  // pipe(pipefd);

  bool background_command = _isBackgroundComamnd(m_cmdLine.c_str());    // Check if it is a background command (if "&" in the end)
  if(background_command){_removeBackgroundSign(shorterCmd);}    // Remove the "&" from shortherCmd, we don't need it anymore
  _parseCommandLine(shorterCmd, args);                          // Take the version without the "&" and divide it to an array

  pid_t pid = fork();                                           // Create a child process
  
  if(pid == 0){                                                 // New child process code
    setpgrp();                                     
    if(_isSpecialExternalComamnd(shorterCmd)) {
      char* bash_exec[] = {(char*)"bash", (char*)"-c", shorterCmd, nullptr};
      // char* bash_exec[] = {"bash", "-c", shorterCmd, nullptr};  // Create an array to run: "bash -c "<original input>""
      if (execvp("bash", bash_exec) == -1){
        perror("smash error: execvp failed");
      }
    }
    if (execvp(args[0], args) == -1) {                          // Search for the command in PATH env, with our arguments
      // std::cout << "im about to fail with command: " << shorterCmd << std::endl;
      perror("smash error: execvp failed");
    }
    SmallShell::getInstance().getJobsList()->getLastJob()->set_stopped();   // If execvp fail so out of vector
    exit(1);                                                    // Get here only if execvp failed

  }
  else if(pid > 0){                                              // Parent process code
    if(background_command){
      SmallShell::getInstance().getJobsList()->addJob(this, pid); // If parent process: add the child's process Entry to jobs vector
    }
    else {
      SmallShell::getInstance().set_fg_pid(pid);
      waitpid(pid, nullptr ,0);
      SmallShell::getInstance().set_fg_pid(SmallShell::getInstance().get_pid());
    }
  }
  else{                                                         // Failed fork, may not need to print, but know it is here TODO
    perror("smash error: fork failed");
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

  std::string path_to_check = "/bin";                                         // This is where we eill manual search

    int dir_FD = open(path_to_check.c_str(), O_RDONLY | O_DIRECTORY);
    if (dir_FD < 0) {
        return 0;
    }

    char buffer[8192];                                                                    // Buffer to hold directory entries
    int num_bytes_read;

    while ((num_bytes_read = syscall(SYS_getdents64, dir_FD, buffer, sizeof(buffer))) > 0) { // Read the directory contents into the buffer (SYS_getdents64 is 217 syscall)
        int current_position = 0;                                                         // Track position in the buffer
        
        while (current_position < num_bytes_read) {                                       // Process each directory entry
            linux_dirent64* dir_entry = (linux_dirent64*)(buffer + current_position);     // Cast to directory entry structure
            std::string current_entry_name = dir_entry->entry_name;                       // Get the name of the entry

            if (current_entry_name != "." && current_entry_name != "..") {                // Skip "." and ".." entries
                std::string current_entry_path = string(path_to_check) + "/" + current_entry_name;
                  if(_trim(current_entry_name.c_str()) == _trim(alias_name)){
                    return true;
                  }
            }
            current_position += dir_entry->record_length;                                 // Move to the next directory entry
        }
    }
    close(dir_FD);    

  return false;

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

  for (auto it = m_aliasList.begin(); it != m_aliasList.end(); ) {
    if(*it){
      if (alias_name == (*it)->get_name()) {
          delete *it;  // Free the memory of the JobEntry object
          it = m_aliasList.erase(it); // Remove the pointer from the vector
      } else {
          ++it;
      }
    }
  }
}

JobsList* SmallShell::getJobsList(){return this->m_jobsList;}

bool JobsList::job_exist(int job_ID_to_look){
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ++it) {
    if((*it)->getJobId() == job_ID_to_look) {
      return true;
    }
  }
  return false;
}

void JobsCommand::execute(){
    m_jobs->printJobsList();
}

//_____________________ Jobs List implemintation _____________________ //

JobsList::JobEntry::JobEntry(int newJobId, pid_t newJobPid, Command *cmd) : m_jobId(newJobId), m_pid(newJobPid), m_jobCmdLine(SmallShell::getInstance().getLastCmdLine()){
  m_stopped = false;
}

void JobsList::updateMaxJobID(){
  maxJobId = 0;
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ++it) {
    if((*it)->getJobId() > maxJobId) {
      maxJobId = ((*it)->getJobId());
    }
  }
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
  updateMaxJobID();
  JobEntry* new_entry = new JobEntry(this->maxJobId + 1 , pid , cmd);
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
}



void JobsList:: killAllJobs(){
  this->removeFinishedJobs();
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ++it) {
      if (*it == nullptr) continue; // skip null entries
      
      pid_t pid_to_kill = (*it)->getPid();
      kill(pid_to_kill, SIGKILL);
      (*it)->set_stopped();
      // waitpid(pid_to_kill, nullptr, WNOHANG);   // TODO consider to use because of zombies
  }
}

//___remove all the finished jobs from the jobs list___//
void JobsList::removeFinishedJobs() {
  for (auto it = jobsVector.begin(); it != jobsVector.end(); ) {
      int status;
      pid_t pid_to_delete = (*it)->getPid();
      pid_t result = waitpid(pid_to_delete, &status, WNOHANG);

      if ((*it)->is_stopped() || (pid_to_delete == result)) {
          delete *it;  // Free the memory of the JobEntry object
          it = jobsVector.erase(it); // Remove the pointer from the vector
      } else {
          ++it;
      }
  }
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


JobsList::JobEntry* JobsList::getLastJob() {
  if(jobsVector.empty()){
    return nullptr;
  }
  return jobsVector.back();
}

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
        std::cerr << "smash error: fg: jobs list is empty" << std::endl;
      }
      else{
        JobsList::JobEntry* requastedJob = m_jobs->getLastJob();
        requastedJob -> set_stopped();
        if( requastedJob != nullptr){
          SmallShell::getInstance().set_fg_pid(requastedJob->getPid());
          std::cout << requastedJob->getJobCmdLine() << " " << requastedJob->getPid() << std::endl;          // Print as declared in the PDF
          waitpid(requastedJob->getPid(), &status, 0);                                                       // waitpid method
          SmallShell::getInstance().set_fg_pid(SmallShell::getInstance().get_pid());
        }
      }
    }

    else if((argc == 2) && (atoi(args[1]) > 0)){
      int jobId = atoi(args[1]);
      if(SmallShell::getInstance().getJobsList()->job_exist(jobId)){
        JobsList::JobEntry* requastedJob = m_jobs->getJobById(jobId);
        requastedJob -> set_stopped();      
        SmallShell::getInstance().set_fg_pid(requastedJob->getPid());                                         // Status for waitpid usage
        std::cout << requastedJob->getJobCmdLine() << " " << requastedJob->getPid() << std::endl;             // Print as declared in the PDF   
        waitpid(requastedJob->getPid(), &status, 0);                                                          // waitpid method   
        SmallShell::getInstance().set_fg_pid(SmallShell::getInstance().get_pid());         
      }
      else{
        std::cerr << "smash error: fg: job-id " << jobId << " does not exist" << std::endl;
      }
    } 
    else{
      std::cerr << "smash error: fg: invalid arguments" << std::endl;
    }
  }
  SmallShell::getInstance().getJobsList()->removeFinishedJobs();
}

//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ------------------------------------------Redirection Command methods section------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line){

  std::string str_cmd_line(cmd_line);

  if(_isBackgroundComamnd(cmd_line)){
    char* temp_cmd = new char[std::strlen(cmd_line) + 1]; // + 1 for null terminator
    std::strcpy(temp_cmd, cmd_line);
    _removeBackgroundSign(temp_cmd);
    str_cmd_line = std::string(temp_cmd);
    delete[] temp_cmd;
  }

  size_t index;

  if((index = str_cmd_line.find(">>")) != std::string::npos){
    this->append_to_end = true;
  }  
  else{
    index = str_cmd_line.find(">");
    this->append_to_end = false;
  }
  std::string command = str_cmd_line.substr(0, index);
  std::string file = str_cmd_line.substr(index + (append_to_end ? 2 : 1));

  m_file_name = _trim(file);

  m_command = SmallShell::getInstance().CreateCommand(command.c_str());
}
  
RedirectionCommand::~RedirectionCommand(){
  delete m_command;
}

void RedirectionCommand::execute(){ 
  int old_stdout_fd = dup(STDOUT_FILENO);
  if(old_stdout_fd == -1){
    perror("smash error: dup failed");
    return;
  }

  int file_fd = open(m_file_name.c_str(), O_WRONLY | O_CREAT | (append_to_end ? O_APPEND : O_TRUNC),0644);
  if(file_fd == -1){
    perror("smash error: open failed");
    close(old_stdout_fd); 
    return;
  }

  if(dup2(file_fd, STDOUT_FILENO) == -1){
    perror("smash error: dup2 failed");
    close(file_fd);
    return;
  }
  close(file_fd);

  m_command->execute();

  if(dup2(old_stdout_fd, STDOUT_FILENO) == -1){
    perror("smash error: dup2 failed");
    close(old_stdout_fd);
    return;
  }
  close(old_stdout_fd);
}

//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//
//****************************************************************************************************************************//
// ------------------------------------------Pipe Command methods section-------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
  std::string str_cmd_line(cmd_line);
  //maybe need to add ignore background like redirection command
  
  size_t index;

  if((index = str_cmd_line.find("|&")) != std::string::npos){
    this->pipe_stderr = true;
  }  
  else{
    index = str_cmd_line.find("|");
    this->pipe_stderr = false;
  }

  std::string first_cmd = _trim(str_cmd_line.substr(0, index));
  std::string second_cmd = _trim(str_cmd_line.substr(index + (pipe_stderr ? 2 : 1)));
  
  m_first_command = SmallShell::getInstance().CreateCommand(first_cmd.c_str());
  m_second_command = SmallShell::getInstance().CreateCommand(second_cmd.c_str());
 }


 PipeCommand::~PipeCommand(){
  delete m_first_command;
  delete m_second_command;
 }

void PipeCommand::execute(){
  int pipe_read_write_fds[2];
  if(pipe(pipe_read_write_fds) == -1){
    perror("smash error: pipe failed");
    return;
  }
  int old_stdout_fd = dup(STDOUT_FILENO);
  int old_stdin_fd = dup(STDIN_FILENO);
  int old_stderr_fd = dup(STDERR_FILENO);

  if(old_stdout_fd == -1 || old_stdin_fd == -1 || old_stderr_fd == -1){
    perror("smash error: dup failed");
    if(old_stdout_fd != -1) close(old_stdout_fd);
    if(old_stdin_fd != -1) close(old_stdin_fd);
    if(old_stderr_fd != -1) close(old_stderr_fd);
    return;
    }

  pid_t pid_first = fork();

  if(pid_first == -1){
    perror("smash error: fork failed");
    return;
  }
    
  if(pid_first == 0){ 
  //first command process
    setpgrp();
    close(pipe_read_write_fds[0]); //closing first command read fd

    if(dup2(pipe_read_write_fds[1],STDOUT_FILENO) == -1){
      perror("smash error: dup2 failed");
      close(pipe_read_write_fds[1]);
      exit(1);
    }

    if(pipe_stderr){
      if(dup2(pipe_read_write_fds[1],STDERR_FILENO) == -1){
        perror("smash error: dup2 failed");
        close(pipe_read_write_fds[1]);
        exit(1);
      }
    }
    close(pipe_read_write_fds[1]);
    m_first_command->execute();
    exit(0);
  }

  else if(pid_first > 0){ 
    //parent process
    pid_t pid_second = fork();

    if(pid_second == -1){
      perror("smash error: fork failed");
      return;
    }

    if(pid_second == 0){ 
    //second command process 
      setpgid(0, pid_first);
      close(pipe_read_write_fds[1]); //closing second command write fd 

      if(dup2(pipe_read_write_fds[0],STDIN_FILENO) == -1){
        perror("smash error: dup2 failed");
        close(pipe_read_write_fds[0]);
        exit(1);
      }

      close(pipe_read_write_fds[0]);
      m_second_command->execute();
      exit(0);
    }

    //parent process
    close(pipe_read_write_fds[0]);
    close(pipe_read_write_fds[1]);


    SmallShell::getInstance().set_fg_pid(pid_second);
    waitpid(pid_first, nullptr, 0);
    waitpid(pid_second, nullptr, 0);
    SmallShell::getInstance().set_fg_pid(SmallShell::getInstance().get_pid());
    
    if(dup2(old_stdout_fd,STDOUT_FILENO) == -1 || dup2(old_stdin_fd,STDIN_FILENO) == -1){
      perror("smash error: dup2 failed");
      return;
    }

    if(pipe_stderr){
      if(dup2(old_stderr_fd,STDERR_FILENO) == -1){
        perror("smash error: dup2 failed");
        return;
      }
    }
    close(old_stdout_fd);
    close(old_stdin_fd);
    close(old_stderr_fd);
  }

}


//------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------End-of-section---------------------------------------------------------
//****************************************************************************************************************************//

