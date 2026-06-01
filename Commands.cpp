#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <cstring>
//#include <bits/regex.h>

#include <regex>
#include <tuple>
#include <sys/utsname.h> // might be illegal
#include <fcntl.h> // might be illegal
#include <sys/types.h> // shown in power point, may not be necessary
#include <signal.h> // shown in power point, may not be necessary
//#include <sys/statvfs.h> // might be illegal
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>

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

SmallShell::SmallShell() : text_prompt("smash> "), last_dir(nullptr), input_mode(0) /*pipe_in(false), pipe_out(false)*/ {
    // TODO: add your implementation
    jobs = new JobsList();
}

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string stripped_input = _trim(string(cmd_line));
    input = stripped_input;

    // checks for I/O redirection and pipes
    size_t should_not_redirect  = stripped_input.find(">>>"); // may not be needed. if needed, maybe add one for pipes too
    size_t redirect_append = stripped_input.find(">>");
    size_t redirect  = stripped_input.find(">");
    size_t pipe  = stripped_input.find("|");
    size_t error_pipe  = stripped_input.find("|&");

    if (should_not_redirect != string::npos) {
        redirect_append = string::npos;
        redirect  = string::npos;
    }
    if (redirect_append != string::npos) { // this has to be checked redirect
        input_mode = 2;
        input  = _trim(stripped_input.substr(0, redirect_append));
        output = _trim(stripped_input.substr(redirect_append + 2));
        return new RedirectionCommand(stripped_input.c_str());
    } else if (redirect != string::npos) {
        input_mode = 1;
        input  = _trim(stripped_input.substr(0, redirect));
        output = _trim(stripped_input.substr(redirect + 1));
        return new RedirectionCommand(stripped_input.c_str());
    } else if (error_pipe != string::npos) { // this has to be checked before pipe
        input_mode = 4;
        input  = _trim(stripped_input.substr(0, error_pipe));
        output = _trim(stripped_input.substr(error_pipe + 2));
        return new PipeCommand(stripped_input.c_str()); // probably needs to be filtered_line
    } else if (pipe != string::npos) {
        input_mode = 3;
        input  = _trim(stripped_input.substr(0, pipe));
        output = _trim(stripped_input.substr(pipe + 1));
        return new PipeCommand(stripped_input.c_str()); // probably needs to be filtered_line
    }

    char *filtered_line = strdup(cmd_line); //is this good? idk
    if (input_mode != 0) {
        filtered_line = strdup(input.c_str());
    }
    if (_isBackgroundComamnd(cmd_line)) _removeBackgroundSign(filtered_line);
    string cmd_s = _trim(string(filtered_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0) {
        return new ChangePromptCommand(filtered_line);
    }
    if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(filtered_line);
    }
    if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(filtered_line);
    }
    if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(filtered_line, &getInstance().last_dir);
    }
    if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, jobs);
    }
    if (firstWord.compare("fg") == 0) {
         return new ForegroundCommand(cmd_line, jobs);
    }
    if (firstWord.compare("quit") == 0) {
         return new QuitCommand(cmd_line, jobs);
    }
    // if (firstWord.compare("kill") == 0) {
    //     return new KillCommand(cmd_line, jobs);
    // }
    if (firstWord.compare("alias") == 0) {
        return new AliasCommand(filtered_line);
    }
    if (firstWord.compare("unalias") == 0) {
        return new UnAliasCommand(filtered_line);
    }
    if (firstWord.compare("unsetenv") == 0) {
        return new UnSetEnvCommand(filtered_line);
    }
    if (firstWord.compare("sysinfo") == 0) {
        return new SysInfoCommand(filtered_line);
    }
    if (firstWord.compare("du") == 0) {
        return new DiskUsageCommand(filtered_line);
    }
    if (firstWord.compare("whoami") == 0) {
        return new WhoAmICommand(filtered_line);
    }
    for (auto i : getInstance().get_alias_list()) {
        if (get<0>(i) == string(firstWord)) {
            return new AliassedCommand(filtered_line, get<1>(i));
        }
    }

    if (input_mode != 0) {
        return new ExternalCommand(input.c_str());  // just the command part
    } else {
        return new ExternalCommand(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    input_mode = 0;
    input = "";
    output = "";
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
}

void SmallShell::change_prompt(string str) {
    getInstance().text_prompt = str;
}

string SmallShell::get_text_prompt() {
    return getInstance().text_prompt;
}

char* SmallShell::get_last_dir() {
    return getInstance().last_dir;
}

void SmallShell::set_last_dir(char* str) {
    getInstance().last_dir = str;
}

void SmallShell::add_alias(string alias, string command, string cmd_line) {
    tuple<string, string, string> temp(alias, command, cmd_line);
    alias_list.push_back(tuple<string, string, string>(alias, command, cmd_line));
}

bool SmallShell::remove_alias(string alias) {
    for (auto i = alias_list.begin(); i != alias_list.end(); i++) {
        if (get<0>(*i) == alias) {
            alias_list.erase(i);
            return true;
        }
    }
    return false;
}

vector<tuple<string, string, string>> SmallShell::get_alias_list() {
    return alias_list;
}

int SmallShell::get_input_mode() {
    return input_mode;
}

string SmallShell::get_input() {
    return input;
}

string SmallShell::get_output() {
    return output;
}

void SmallShell::set_input_mode(int mode) {
    input_mode = mode;
}

void SmallShell::set_input(string str) {
    input = str;
}

void SmallShell::set_output(string str) {
    output = str;
}

// bool SmallShell::get_pipe_in() {
//     return pipe_in;
// }
//
// bool SmallShell::get_pipe_out() {
//     return pipe_out;
// }
//
// void SmallShell::set_pipe_in(bool input) {
//     pipe_in = input;
// }
//
// void SmallShell::set_pipe_out(bool input) {
//     pipe_out = input;
// }




ChangePromptCommand::ChangePromptCommand(const char* cmd_line): BuiltInCommand(cmd_line) {
    char** args = new char*[COMMAND_MAX_ARGS];
    int size = _parseCommandLine(cmd_line, args);

    string temp = "smash";
    if (size > 1) {
        temp = string(args[1]);
    }
    prompt = temp + "> ";
}

void ChangePromptCommand::execute() {
    SmallShell::getInstance().change_prompt(this->prompt);
}

ShowPidCommand::ShowPidCommand(char const* cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    string str = "smash pid is " + to_string(getpid()) + "\n";
    write(1,str.c_str(),str.length());
}

GetCurrDirCommand::GetCurrDirCommand(char const* cmd_line): BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char* path = getcwd(NULL, 0);
    string str = string(path) + "\n";
    write(1,str.c_str(),str.length());
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd):
    BuiltInCommand(cmd_line), last_dir_pointer(plastPwd) {
    char** args = new char*[COMMAND_MAX_ARGS];
    int size = _parseCommandLine(cmd_line, args);
    valid_command = false;

    if (size > 2) {
        string str = "smash error: cd: too many arguments\n";
        write(2,str.c_str(),str.length());
    }
    else if (size == 2) {
        valid_command = true;
        path = args[1];
        if (path[0] == '-' && path[1] == '\0') {
            if (*last_dir_pointer != nullptr) {
                path = *last_dir_pointer;
            }
            else {
                valid_command = false;
                string str = "smash error: cd: OLDPWD not set\n";
                write(2,str.c_str(),str.length());
            }
        }
    }
}

void ChangeDirCommand::execute() {
    if (valid_command) {
        SmallShell::getInstance().set_last_dir(getcwd(NULL, 0));
        if (chdir(path) == -1) {
            perror("smash error: chdir failed");
        }
    }
}

AliasCommand::AliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line), cmd_line(cmd_line) {}

void AliasCommand::execute() {
    string stripped_input = _trim(string(SmallShell::getInstance().get_input()));

    if (stripped_input == "alias") {
        write(1,"\n",1);
        for (auto i : SmallShell::getInstance().get_alias_list()) {
            string str = string(get<2>(i)) + "\n\n";
            write(1,str.c_str(),str.length());
        }
    } else {
        regex pattern("^alias ([a-zA-Z0-9_]+)='([^']*)'$");
        if (regex_match(string(cmd_line), pattern)) {
            string temp = string(stripped_input.substr(6));
            size_t index = temp.find('=');
            size_t before_equal_sign = temp.find('\'', index);
            size_t after_equal_sign = temp.find('\'', before_equal_sign + 1);

            string alias = temp.substr(0, index);
            string command = temp.substr(before_equal_sign + 1, after_equal_sign - before_equal_sign - 1);

            if (alias == "chprompt" || alias == "showpid" || alias == "pwd" || alias == "cd" || alias == "jobs" ||
                alias == "fg" || alias == "quit" || alias == "kill" || alias == "alias" || alias == "unalias" ||
                alias == "unsetenv " || alias == "sysinfo" || alias == "du" || alias == "whoami" ||
                alias == "usbinfo ") { // may need to add more
                string str = "smash error: alias: " + string(alias) + " already exists or is a reserved command\n";
                write(2,str.c_str(),str.length());
                return;
            }

            for (auto i : SmallShell::getInstance().get_alias_list()) {
                if (get<0>(i) == alias) {
                    SmallShell::getInstance().remove_alias(alias);
                }
            }
            SmallShell::getInstance().add_alias(alias,command,stripped_input.substr(6));

        } else {
            string str = "smash error: alias: invalid alias format\n";
            write(2,str.c_str(),str.length());
        }
    }
}

AliassedCommand::AliassedCommand(const char *cmd_line, string command) : BuiltInCommand(cmd_line),
cmd_line(cmd_line), command(command) {}

void AliassedCommand::execute() {
    SmallShell::getInstance().set_input_mode(0);
    SmallShell::getInstance().set_input("");
    SmallShell::getInstance().set_output("");

    string temp = replace_first_word(cmd_line, command);
    Command* cmd = SmallShell::getInstance().CreateCommand(temp.c_str());
    cmd->execute();
}

UnAliasCommand::UnAliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line), cmd_line(cmd_line) {}

void UnAliasCommand::execute() {
    char** args = new char*[COMMAND_MAX_ARGS];
    int size = _parseCommandLine(cmd_line, args);

    if (size == 1) {
        string str = "smash error: unalias: not enough arguments\n";
        write(2,str.c_str(),str.length());

    }
    else {
        for (int i = 1; i < size; ++i) {
            char* name = args[i];
            if (!SmallShell::getInstance().remove_alias(name)) {
                string str = "smash error: unalias: " + string(name) + " alias does not exist\n";
                write(2,str.c_str(),str.length());
                return;
            }
        }
    }
}

UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line), cmd_line(cmd_line) {}

void UnSetEnvCommand::execute() {
    char** args = new char*[COMMAND_MAX_ARGS];
    int size = _parseCommandLine(cmd_line, args);

    if (size == 1) {
        string str = "smash error: unsetenv: not enough arguments\n";
        write(2,str.c_str(),str.length());
        return;
    }

    int pid = getpid();
    string buff = "";
    string environment_file = "/proc/" + to_string(pid) + "/environ";


    int file_open = open(environment_file.c_str(), O_RDONLY, 0666); // should probably be 0444 or 0222
    if (file_open == -1) {
        perror("smash error: open failed");
        return;
    }

    vector<char> temp;
    char buffer[1000];
    int file_read = 1; // 1 is arbiturary
    while (file_read > 0) {
        //cout << "while ,";
        file_read = read(file_open, buffer, 1000);
        //cout << "read" << endl;
        if (file_read == -1) {
            perror("smash error: read failed");
            return;
        }
        for (int i = 0; i < file_read; i++)
        {
            //cout << "for, ";
            if (buffer[i] != '\0') {
                temp.push_back(buffer[i]);
            } else {
                //cout << "(\\.0)" << endl;
                string key_value(temp.begin(), temp.end());
                key_value_vector.push_back(key_value); // maybe key_value_vector.push_back(temp.data()); possible and better
                temp.clear();
            }
        }
    }
    close(file_open);
    for (int j = 0; j < key_value_vector.size(); j++)
    {
        cout << key_value_vector[j] << endl;
    }

    // reads the file from open/proc/<pid>/environ and adds key-value pairs to the vector

    // for every argument in args, check if its in the vector

    // remove the key-values in open/proc/<pid>/environ using char **__environ array
}

SysInfoCommand::SysInfoCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void SysInfoCommand::execute() { // works somehow
    struct utsname info;
    uname(&info);

    string str = "System: " + string(info.sysname) + "\n";
    str += "Hostname: " + string(info.nodename) + "\n";
    str += "Kernel: " + string(info.release) + "\n";
    str += "Machine: " + string(info.machine) + "\n";
    write(1,str.c_str(),str.length());
}



Command::Command(const char* cmd_line): og_line(cmd_line) {}

const char* Command::get_line() const {
    return og_line;
}

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute() {
    int original_output_channel = dup(1);
    close(1);

    if (SmallShell::getInstance().get_input_mode() == 1) {
        open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0666);
    } else if (SmallShell::getInstance().get_input_mode() == 2) {
        open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND|O_RDWR, 0666);
    }

    string temp = SmallShell::getInstance().get_input();
    SmallShell::getInstance().set_input_mode(0);
    SmallShell::getInstance().set_input("");
    SmallShell::getInstance().set_output("");

    Command* cmd = SmallShell::getInstance().CreateCommand(temp.c_str());
    cmd->execute();
    delete cmd;

    dup2(original_output_channel, 1);
    close(original_output_channel);
}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {}

void PipeCommand::execute() {
    string input = SmallShell::getInstance().get_input();
    string output = SmallShell::getInstance().get_output();

    const char* pipe_in = input.c_str();
    const char* pipe_out = output.c_str();

    int fd[2];
    pipe(fd);

    pid_t pid_in = fork();
    //perror("smash error: fork failed");
    if (pid_in == 0) { // first son - writer
        if (SmallShell::getInstance().get_input_mode() == 3) {
            dup2(fd[1], 1);
        } else {
            dup2(fd[1], 2);
        }
        close(fd[0]);
        close(fd[1]);
        setpgrp();

        Command* cmd = SmallShell::getInstance().CreateCommand(pipe_in);
        cmd->execute();
        exit(0);
    }
    pid_t pid_out = fork();
    if (pid_out == 0) { // second son - reader
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
        setpgrp();

        Command* cmd = SmallShell::getInstance().CreateCommand(pipe_out);
        cmd->execute();
        exit(0);
    }
    close(fd[0]);
    close(fd[1]);
    waitpid(pid_in,  nullptr, 0);
    waitpid(pid_out, nullptr, 0);
}

DiskUsageCommand::DiskUsageCommand(const char *cmd_line) : Command(cmd_line), cmd_line(cmd_line) {}

void DiskUsageCommand::execute() {
    char** args = new char*[COMMAND_MAX_ARGS];
    int size = _parseCommandLine(cmd_line, args);
    char* path;

    if (size == 1) {
        path = getcwd(NULL, 0);
    } else if (size == 2) {
        path = args[1];
    } else {
        string str = "smash error: du: too many arguments\n";
        write(2,str.c_str(),str.length());
        return;
    }

    unsigned long long usage = (calc_size(path) + 1023) / 1024; // this is done to round up to KB
    string str = "Total disk usage: " + to_string(usage) + " KB\n";
    write(1, str.c_str(), str.length());
}

WhoAmICommand::WhoAmICommand(const char *cmd_line) : Command(cmd_line) {}

void WhoAmICommand::execute() {
    uid_t user_id = getuid();
    gid_t group_id = getgid();
    struct passwd *user_information = getpwuid(user_id);

    if (user_information != nullptr) {
        string str = string(user_information->pw_name) + "\n";
        write(1, str.c_str(), str.length());
        str = to_string(user_id) + "\n";
        write(1, str.c_str(), str.length());
        str = to_string(group_id) + "\n";
        write(1, str.c_str(), str.length());
        str = string(user_information->pw_dir) + "\n";
        write(1, str.c_str(), str.length());
    }
}

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {
    is_background = _isBackgroundComamnd(cmd_line);
    my_name = strdup(cmd_line);

    char *filtered_line = strdup(cmd_line); //is this good? idk
    if (is_background) _removeBackgroundSign(filtered_line);

    char* aug[COMMAND_MAX_ARGS + 1] = {nullptr};
    _parseCommandLine(filtered_line, aug);
    int i = 0;
    if (strchr(cmd_line, '*') || strchr(cmd_line, '?')) {
        args.push_back(strdup("bash"));
        args.push_back(strdup("-c"));
        args.push_back(strdup(cmd_line));
    } else {
        while (aug[i] != NULL) {
            args.push_back(strdup(aug[i]));
            i ++;
        }
    }
    // store cmd_line if needed later
}

void ExternalCommand::execute() {
    pid_t pid = fork();
    if (pid == 0) {
        setpgrp();
        execvp(args[0], args.data());
        perror("smash error: execvp failed");
        exit(1);
    } else if (pid > 0) {
        if (!is_background) {
            waitpid(pid, nullptr, 0);
        } else {
            SmallShell::getInstance().add_job(this, pid);
        }
    }
    // your implementation or leave empty for now
}



JobsList::JobsList() {
    init = new JobEntry();
}

const char *JobsList::JobEntry::get_line() {
    return line;
}

pid_t JobsList::JobEntry::get_pid() {
    return pid;
}

void SmallShell::add_job(Command* com, pid_t pid) {
    jobs->addJob(com, pid);
}

void JobsList::addJob(Command* com, pid_t pid, bool baba) {
    max++;
    new JobEntry(com, init, pid, max);
}

void JobsList::printJobsList() {
    JobEntry* trav = init->next;
    while (string(trav->get_line()) != "aaaaa") {
        cout << "["<< trav->idx << "] " << trav->get_line() << endl;
        trav = trav->next;
    }
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    jobs->printJobsList();
}

void JobsList::removeFinishedJobs() {
    JobEntry* curr = init->next;
    int ind = 1;

    while(curr != init) {
        JobEntry* next = curr->next;

        pid_t result =
            waitpid(curr->get_pid(), nullptr, WNOHANG);

        if(result > 0)
        {
            delete curr;
        } else {
            curr->idx = ind;
            ind ++;
        }

        curr = next;
    }
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line) {
    char *filtered_line = strdup(cmd_line);
    if (_isBackgroundComamnd(filtered_line)) _removeBackgroundSign(filtered_line);
    char* aug[COMMAND_MAX_ARGS + 1] = {nullptr};
    _parseCommandLine(filtered_line, aug);
    if (aug[1] == NULL) {
        idx = jobs->get_max();
    } else {
        idx = atoi(aug[1]);
    }
    jobl = jobs;
}

void JobsList::removeJobById(int jobId) {
    JobEntry* trav = init;
    while (jobId > 0) {
        jobId --;
        trav = trav->next;
    }
    waitpid(trav->get_pid(),  nullptr, 0);

}

void ForegroundCommand::execute() {
    jobl->removeJobById(idx);
}

int JobsList::get_max() {
    return max;
}

QuitCommand::QuitCommand(const char* cmd_line,JobsList* jobs): BuiltInCommand(cmd_line) {
    char *filtered_line = strdup(cmd_line);
    if (_isBackgroundComamnd(filtered_line)) _removeBackgroundSign(filtered_line);
    char* aug[COMMAND_MAX_ARGS + 1] = {nullptr};
    _parseCommandLine(filtered_line, aug);
    type2 = false;
    if (aug[1] == NULL) {
        type2 = true;
    }
    jobl = jobs;
}

void QuitCommand::execute() {
    if (type2) {
        jobl->killAllJobs();
    }
    exit(0);
}

void JobsList::killAllJobs() {
    JobEntry* trav = init->next;
    while (string(trav->get_line()) != "aaaaa") {
        kill(trav->get_pid(), SIGKILL);
        trav = trav->next;
    }
}



string replace_first_word(const char* cmd_line, string command) {
    string str(cmd_line);
    size_t index = str.find(' ');

    if (index == string::npos) {
        return command;
    }

    str.replace(0, index, command);
    return str;
}

unsigned long long calc_size(const char* path) {
    struct stat st;

    if (lstat(path, &st) == -1) {
        return 0;
    }
    if (S_ISLNK(st.st_mode)) {
        return 0;
    }

    unsigned long long total = st.st_blocks * 512;
    if (S_ISREG(st.st_mode)) { // file
        return total;
    }
    DIR* dir = opendir(path); // directory
    if (!dir) {
        return total;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        string child = string(path) + "/" + name;
        total += calc_size(child.c_str());
    }
    closedir(dir);
    return total;
}
