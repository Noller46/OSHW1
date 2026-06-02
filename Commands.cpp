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
#include <complex>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
//#include <bits/valarray_before.h>
#include <ftw.h>

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

unsigned long long DiskUsageCommand::disk_usage = 0;

string _ltrim(const string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == string::npos) ? "" : s.substr(start);
}

string _rtrim(const string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    istringstream iss(_trim(string(cmd_line)).c_str());
    for (string s; iss >> s;) {
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
    // string str = "//CreateCommand//: " + string(cmd_line) + "\n";
    // write(2,str.c_str(),str.length());

    string stripped_input = _trim(string(cmd_line));
    input = stripped_input;

    char *filtered_line = strdup(cmd_line); //is this good? idk
    if (input_mode != 0) {
        filtered_line = strdup(input.c_str());
    }
    if (_isBackgroundComamnd(cmd_line)) _removeBackgroundSign(filtered_line);
    string cmd_s = _trim(string(filtered_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("alias") == 0) {
        return new AliasCommand(filtered_line);
    }

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
    if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, jobs);
    }
    // if (firstWord.compare("alias") == 0) {
    //     cout << "filtered_line: " << filtered_line << "cmd_line: " << cmd_line << endl;
    //     return new AliasCommand(filtered_line);
    // }
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
        // string str = "///CreateCommand external 1: " + string(input.c_str()) + "\n";
        // write(2,str.c_str(),str.length());
        return new ExternalCommand(input.c_str());  // just the command part
    } else {
        // string str = "///CreateCommand external 2: " + string(cmd_line) + "\n";
        // write(2,str.c_str(),str.length());
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

UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) :
BuiltInCommand(cmd_line), cmd_line(cmd_line), size(0), args(nullptr) {}

void UnSetEnvCommand::execute() {
    args = new char*[COMMAND_MAX_ARGS];
    size = _parseCommandLine(cmd_line, args);

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

    // separate key_values to vector
    vector<char> temp;
    char buffer[1024];
    int file_read = 1; // 1 is arbiturary
    while (file_read > 0) {
        file_read = read(file_open, buffer, 1024);
        if (file_read == -1) {
            perror("smash error: read failed");
            return;
        }
        for (int i = 0; i < file_read; i++)
        {
            if (buffer[i] != '\0') {
                temp.push_back(buffer[i]);
            } else {
                string key_value(temp.begin(), temp.end());
                key_value_vector.push_back(key_value);
                temp.clear();
            }
        }
    }
    close(file_open);

    //find_key_values_in_env(cmd_line, args, size, key_value_vector);

    // for every argument in args, check if its in the vector
    find_and_remove_env();

    // extern char **__environ;
    // for (char** env = __environ; *env != nullptr; ++env) {
    //     cout << *env << endl;
    // }

}

SysInfoCommand::SysInfoCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void SysInfoCommand::execute() { // works somehow
    string system; string hostname; string kernel; string architecture; string boot_time;
    string* path = new string[6];
    int pid = getpid();
    string buff = "";
    path[0] = "/proc/sys/kernel/ostype";
    path[1] = "/proc/sys/kernel/hostname";
    path[2] = "/proc/sys/kernel/osrelease";
    path[3] = "/proc/stat";
    path[4] = "/proc/" + to_string(pid) + "/environ";

    for (int i = 0; i < 3; i++)
    {
        int file_open = open(path[i].c_str(), O_RDONLY, 0666); // should probably be 0444 or 0222
        if (file_open == -1) {
            perror("smash error: open failed");
            return;
        }

        string str;
        char buffer[1024];

        int check;
        while (true) {
            check = read(file_open, buffer, 1024);
            if (check == 0) {
                break;
            }
            if (check == -1) {
                perror("smash error: read failed");
                return;
            }
            str.append(buffer, check);
        }

        if (i == 0) {
            system = str;
        } else if (i == 1) {
            hostname = str;
        } else {
            kernel = str;
        }

        close(file_open);
        if (file_open == -1) {
            perror("smash error: close failed");
            return;
        }
    }

    char seperator = '\n';
    for (int i = 3; i < 5; i++) {
        int file_open = open(path[i].c_str(), O_RDONLY, 0666); // should probably be 0444 or 0222
        if (file_open == -1) {
            perror("smash error: open failed");
            return;
        }
        if (i == 4) {
            seperator = '\0';
        }

        // separate key_values to vector
        vector<char> temp;
        char buffer[1024];
        int file_read = 1; // 1 is arbiturary
        while (file_read > 0) {
            file_read = read(file_open, buffer, 1024);
            if (file_read == -1) {
                perror("smash error: read failed");
                return;
            }
            for (int i = 0; i < file_read; i++) {
                if (buffer[i] != seperator) {
                    temp.push_back(buffer[i]);
                } else {
                    string key_value(temp.begin(), temp.end());
                    key_value_vector.push_back(key_value);
                    temp.clear();
                }
            }
        }
        close(file_open);
        if (file_open == -1) {
            perror("smash error: close failed");
            return;
        }

        if (i == 4) {
            for (auto it = key_value_vector.begin(); it != key_value_vector.end(); it++) {
                if (it->find( "HOSTTYPE=") == 0) {
                    architecture = *it + "\n";
                    break;
                }
            }
        } else {
            for (auto it = key_value_vector.begin(); it != key_value_vector.end(); it++) {
                if (it->find( "btime") == 0) {
                    boot_time = *it + "\n";
                    break;
                }
            }
        }

        key_value_vector.clear();
    }

    string architecture_value = "Architecture: ";
    size_t pos = architecture.find("=");
    if (pos != string::npos) {
        architecture_value += architecture.substr(pos + 1);
    }

    string boot_time_value = "Boot Time: ";
    time_t time;
    char formatted_time[20];

    pos = boot_time.find(" ");
    if (pos != string::npos) {
        time = stoi(boot_time.substr(pos + 1));
    }

    strftime(formatted_time, 20, "%Y-%m-%d %H:%M:%S", localtime(&time)); // check exactly what this does
    string actual_time(formatted_time);
    boot_time_value += actual_time + "\n";

    string system_value = "System: " + system;
    string hostname_value = "Hostname: " + hostname;
    string kernel_value = "Kernel: " + kernel;

    write(1,system_value.c_str(),system_value.length());
    write(1,hostname_value.c_str(),hostname_value.length());
    write(1,kernel_value.c_str(),kernel_value.length());
    write(1,architecture_value.c_str(),architecture_value.length());
    write(1,boot_time_value.c_str(),boot_time_value.length());
}



Command::Command(const char* cmd_line): og_line(cmd_line) {}

const char* Command::get_line() const {
    return og_line;
}

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute() {
    // string str = "///RedirectionCommand, input: " + SmallShell::getInstance().get_input() + "\n";
    // write(2,str.c_str(),str.length());
    int original_output_channel = dup(STDOUT_FILENO);
    int file_open;

    // file_open = close(1);
    // if (file_open == -1) {
    //     perror("smash error: close failed");
    //     exit(1);
    // }

    if (SmallShell::getInstance().get_input_mode() == 1) {
        file_open = open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0666);
    } else if (SmallShell::getInstance().get_input_mode() == 2) {
        file_open = open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND|O_RDWR, 0666);
    }
    if (file_open == -1) {
        perror("smash error: open failed");
        exit(1);
    }

    dup2(file_open, STDOUT_FILENO);
    close(file_open);

    string temp = SmallShell::getInstance().get_input();
    SmallShell::getInstance().set_input_mode(0);
    SmallShell::getInstance().set_input("");
    SmallShell::getInstance().set_output("");
    // string str = "//4//, temp.c_str(): " + temp + "\n";
    // write(2,str.c_str(),str.length());
    Command* cmd = SmallShell::getInstance().CreateCommand(temp.c_str());
    //write(2,"//51//\n",6);
    cmd->execute();
    //write(2,"//52//",5);
    delete cmd;
    //write(2,"//5//",5);
    dup2(original_output_channel, STDOUT_FILENO);
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

    unsigned long long total_usage = (sum_usage(path) + 1023) / 1024; // this is done to round up to KB
    string str = "Total disk usage: " + to_string(total_usage) + " KB\n";
    write(1, str.c_str(), str.length());
}

WhoAmICommand::WhoAmICommand(const char *cmd_line) : Command(cmd_line),cmd_line(cmd_line), size(0), args(nullptr) {}

void WhoAmICommand::execute() {
    //find_key_values_in_env(cmd_line, args, size, key_value_vector);
    int pid = getpid();
    string buff = "";
    string environment_file = "/proc/" + to_string(pid) + "/environ";

    int file_open = open(environment_file.c_str(), O_RDONLY, 0666); // should probably be 0444 or 0222
    if (file_open == -1) {
        perror("smash error: open failed");
        return;
    }

    // separate key_values to vector
    vector<char> temp;
    char buffer[1024];
    int file_read = 1; // 1 is arbiturary
    while (file_read > 0) {
        file_read = read(file_open, buffer, 1024);
        if (file_read == -1) {
            perror("smash error: read failed");
            return;
        }
        for (int i = 0; i < file_read; i++)
        {
            if (buffer[i] != '\0') {
                temp.push_back(buffer[i]);
            } else {
                string key_value(temp.begin(), temp.end());
                key_value_vector.push_back(key_value);
                temp.clear();
            }
        }
    }
    close(file_open);

    string username;
    for (auto it = key_value_vector.begin(); it != key_value_vector.end(); it++) {
        if (it->find( "USER=") == 0) {
            username = *it + "\n";
            break;
        }
    }
    string home_directory;
    for (auto it = key_value_vector.begin(); it != key_value_vector.end(); it++) {
        if (it->find( "HOME=") == 0) {
            home_directory = *it + "\n";
            break;
        }
    }

    string username_value;
    string home_directory_value;

    size_t pos = username.find("=");
    if (pos != string::npos) {
        username_value = username.substr(pos + 1);
    }
    pos = home_directory.find("=");
    if (pos != string::npos) {
        home_directory_value = home_directory.substr(pos + 1);
    }

    string user_id = to_string(getuid()) + "\n";
    string group_id = to_string(getgid()) + "\n";
    write(1, username_value.c_str(), username_value.length());
    write(1, user_id.c_str(), user_id.length());
    write(1, group_id.c_str(), group_id.length());
    write(1, home_directory_value.c_str(), home_directory_value.length());

}

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {
    // string str = "///EXTERNAL COMMAND create//// cmd_line: " + string(cmd_line) + "\n";
    // write(2,str.c_str(),str.length());
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
    // string str = "///EXTERNAL COMMAND execute//// cmd_line:\n";
    // write(2,str.c_str(),str.length());
    pid_t pid = fork();
    if (pid == 0) {
        //str = "///EXTERNAL COMMAND//// fork child: args[0]: " + string(args[0]) + ", args[1]:" + string(args[1]) + ", args[2]:" + string(args[2]) + "\n";
        //write(2,str.c_str(),str.length());
        args.push_back(nullptr);
        //str = "///EXTERNAL COMMAND//// fork child: args[0]: " + string(args[0]) + ", args[1]:" + string(args[1]) + ", args[2]:" + string(args[2]) + "\n";
        //write(2,str.c_str(),str.length());
        setpgrp();
        execvp(args[0], args.data());
        perror("smash error: execvp failed");
        exit(1);
    } else if (pid > 0) {
        //string str = "///EXTERNAL COMMAND//// fork parent:\n";
        //write(2,str.c_str(),str.length());
        if (!is_background) {
            waitpid(pid, nullptr, 0);
        } else {
            SmallShell::getInstance().add_job(this, pid);
        }
    }
    // str = "///EXTERNAL COMMAND//// end:\n";
    // write(2,str.c_str(),str.length());
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
        string str = "[" + to_string(trav->idx) + "] " + string(trav->get_line()) + "\n";;
        write(1,str.c_str(),str.length());
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
    bad = false;

    if (aug[2] != NULL) {
        string str = "smash error: fg: invalid arguments\n";
        write(2,str.c_str(),str.length());
        bad = true;
    } else if (aug[1] == NULL) {
        if (jobs->get_max() == 0) {
            string str = "smash error: fg: jobs list is empty\n";
            write(2,str.c_str(),str.length());
            bad = true;
        }
        idx = jobs->get_max();
    } else {
        char* end;
        strtol(aug[1], &end, 10);
        if (!(*aug[1] != '\0' && *end == '\0')) {
            string str = "smash error: fg: invalid arguments\n";
            write(2,str.c_str(),str.length());
            bad = true;
        } else if (atoi(aug[1]) > jobs->get_max() || atoi(aug[1]) <= 0) {
            string str = "smash error: fg: job-id <job-id> does not exist\n";
            write(2,str.c_str(),str.length());
            bad = true;
        }
        idx = atoi(aug[1]);
    }
    jobl = jobs;
}

void ForegroundCommand::execute() {
    if (bad) delete this;
    jobl->removeJobById(idx);
}

void JobsList::removeJobById(int jobId) {
    JobEntry* trav = init;
    while (jobId > 0) {
        jobId --;
        trav = trav->next;
    }
    waitpid(trav->get_pid(),  nullptr, 0);
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

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line) {
    char *filtered_line = strdup(cmd_line);
    if (_isBackgroundComamnd(filtered_line)) _removeBackgroundSign(filtered_line);
    char* aug[COMMAND_MAX_ARGS + 1] = {nullptr};
    _parseCommandLine(filtered_line, aug);
    if (aug[1] == NULL || aug[2] == NULL || aug[3] != NULL) {
        string str = "smash error: kill: invalid arguments\n";
        write(2,str.c_str(),str.length());
    }
    if (atoi(aug[1]) > jobs->get_max() || atoi(aug[1]) <= 0) {
        string str = "smash error: kill: job-id <job-id> does not exist\n";
        write(2,str.c_str(),str.length());
    }
    pid = jobs->get_pid_by_id(atoi(aug[2]));
    sig = -1 * atoi(aug[1]);
}

pid_t JobsList::get_pid_by_id(int jobId) {
    JobEntry* trav = init;
    while (jobId > 0) {
        jobId --;
        trav = trav->next;
    }
    return trav->get_pid();
}

void KillCommand::execute() {
    kill(pid, sig);
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

// unsigned long long calc_size(const char* path) {
//     struct stat st;
//
//     if (lstat(path, &st) == -1) {
//         return 0;
//     }
//     if (S_ISLNK(st.st_mode)) {
//         return 0;
//     }
//
//     unsigned long long total = st.st_blocks * 512;
//     if (S_ISREG(st.st_mode)) { // file
//         return total;
//     }
//     DIR* dir = opendir(path); // directory
//     if (!dir) {
//         return total;
//     }
//
//     struct dirent* entry;
//     while ((entry = readdir(dir)) != nullptr) {
//         string name = entry->d_name;
//         if (name == "." || name == "..") {
//             continue;
//         }
//         string child = string(path) + "/" + name;
//         total += calc_size(child.c_str());
//     }
//     closedir(dir);
//     return total;
// }

void UnSetEnvCommand::find_and_remove_env() {
    bool found = false;
    for (int i = 1; i < size; i++) {
        string key = string(args[i]);
        found = false;

        for (auto it = key_value_vector.begin(); it != key_value_vector.end(); it++) {
            if (it->find(key + "=") == 0) {
                shift_left((*it).c_str()); // removes the variable from __environment
                found = true;
                break;
            }
            found = false;
        }

        if (!found) {
            string str = "smash error: unsetenv: " + key + " does not exist\n";
            write(2,str.c_str(),str.length());
            return;
        }
    }
}

void shift_left(const char* variable) {
    extern char **__environ;

    for (int i = 0; __environ[i] != nullptr; i++) {
        if (variable == string(__environ[i])) {
            for (int j = i; __environ[j] != nullptr; j++) {
                __environ[j] = __environ[j + 1];
            }
            break;
        }
    }
}

static int display_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    (void)fpath;
    (void)tflag;
    (void)ftwbuf;

    int amount = (unsigned long long)sb->st_blocks * 512; // this is to ensure a standard size
    DiskUsageCommand::increase_disk_usage(amount);
    return 0;
}

unsigned long long DiskUsageCommand::sum_usage(const char* path) {
    // struct stat sb;
    //
    // if (lstat(path, &sb) == -1) {
    //     perror("lstat");
    //     exit(EXIT_FAILURE);
    // }
    //
    // int size = sb.st_size * 512;
    // cout << size << endl;


    disk_usage = 0;

    if (nftw(path, display_info, 10, FTW_PHYS) == -1) {
        perror("nftw");
        return 0;
    }

    return disk_usage;
}

void DiskUsageCommand::increase_disk_usage(int amount) {
    disk_usage += amount;
}


// void find_key_values_in_env(const char* cmd_line, char** args, int size, vector<string> key_value_vector) {
//     args = new char*[COMMAND_MAX_ARGS];
//     size = _parseCommandLine(cmd_line, args);
//
//     if (size == 1) {
//         string str = "smash error: unsetenv: not enough arguments\n";
//         write(2,str.c_str(),str.length());
//         return;
//     }
//
//     int pid = getpid();
//     string buff = "";
//     string environment_file = "/proc/" + to_string(pid) + "/environ";
//
//     int file_open = open(environment_file.c_str(), O_RDONLY, 0666); // should probably be 0444 or 0222
//     if (file_open == -1) {
//         perror("smash error: open failed");
//         return;
//     }
//
//     // separate key_values to vector
//     vector<char> temp;
//     char buffer[1024];
//     int file_read = 1; // 1 is arbiturary
//     while (file_read > 0) {
//         file_read = read(file_open, buffer, 1024);
//         if (file_read == -1) {
//             perror("smash error: read failed");
//             return;
//         }
//         for (int i = 0; i < file_read; i++)
//         {
//             if (buffer[i] != '\0') {
//                 temp.push_back(buffer[i]);
//             } else {
//                 string key_value(temp.begin(), temp.end());
//                 key_value_vector.push_back(key_value);
//                 temp.clear();
//             }
//         }
//     }
//     close(file_open);
// }
