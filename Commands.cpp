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

SmallShell::SmallShell() : text_prompt("smash> "), last_dir(nullptr), input_mode(0) {
    // TODO: add your implementation
    input_mode = 0;
    J_list = new JobsList();
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
    } else if (redirect != string::npos) {
        input_mode = 1;
        input  = _trim(stripped_input.substr(0, redirect));
        output = _trim(stripped_input.substr(redirect + 1));
    } else if (error_pipe != string::npos) { // this has to be checked before pipe
        input_mode = 4;
        input  = _trim(stripped_input.substr(0, error_pipe));
        output = _trim(stripped_input.substr(error_pipe + 1));
    } else if (pipe != string::npos) {
        input_mode = 3;
        input  = _trim(stripped_input.substr(0, pipe));
        output = _trim(stripped_input.substr(pipe + 1));
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
    //if (firstWord.compare("jobs") == 0) {
    //    return new JobsCommand(cmd_line, jobs);
    //}
    // if (firstWord.compare("fg") == 0) {
    //     return new ForegroundCommand(cmd_line, jobs);
    // }
    // if (firstWord.compare("quit") == 0) {
    //     return new QuitCommand(cmd_line, jobs);
    // }
    // if (firstWord.compare("kill") == 0) {
    //     return new KillCommand(cmd_line, jobs);
    // }
    if (firstWord.compare("alias") == 0) {
        return new AliasCommand(filtered_line);
    }
    if (firstWord.compare("unalias") == 0) {
        return new UnAliasCommand(filtered_line);
    }
    // if (firstWord.compare("unsetenv") == 0) {
    //     return new UnSetEnvCommand(cmd_line);
    // }
    // if (firstWord.compare("sysinfo") == 0) {
    //     return new SysInfoCommand(filtered_line);
    // }
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

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // Please note that you must fork smash process for some commands (e.g., external commands....)
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

void SmallShell::set_input_mode(int mode) {
    input_mode = mode;
}

string SmallShell::get_input() {
    return input;
}

string SmallShell::get_output() {
    return output;
}




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
    int original_output_channel = -1;
    if (SmallShell::getInstance().get_input_mode() == 1) {
        original_output_channel = dup(1); // may be illegal
        close(1);
        open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0777);
        // int fd = open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0777);
        // dup2(fd, 1);
        // close(fd);
    }
    else if (SmallShell::getInstance().get_input_mode() == 2) {
        original_output_channel = dup(1); // may be illegal
        close(1);
        open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND |O_RDWR, 0777);
        // int fd = open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND |O_RDWR, 0777);
        // dup2(fd, 1);
        // close(fd);
    }

    string str = "smash pid is " + to_string(getpid()) + "\n";
    write(1,str.c_str(),str.length());

    if (original_output_channel != -1) {
        dup2(original_output_channel, 1);
        close(original_output_channel);
    }
}

GetCurrDirCommand::GetCurrDirCommand(char const* cmd_line): BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    int original_output_channel = -1;
    if (SmallShell::getInstance().get_input_mode() == 1) {
        original_output_channel = dup(1); // may be illegal
        close(1);
        open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0777);
        // int fd = open(SmallShell::getInstance().get_output().c_str(),O_CREAT | O_TRUNC | O_RDWR, 0666);
        // cerr << "fd=" << fd << endl;
        // if (fd == -1) {
        //     perror("open failed");
        //     return;
        // }
        // dup2(fd, STDOUT_FILENO);
        // close(fd);
    }
    else if (SmallShell::getInstance().get_input_mode() == 2) {
        original_output_channel = dup(1); // may be illegal
        close(1);
        open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND |O_RDWR, 0777);
        // int fd = open(SmallShell::getInstance().get_output().c_str(), O_CREAT | O_APPEND | O_RDWR, 0666);
        // dup2(fd, 1);
        // close(fd);
    }

    char* path = getcwd(NULL, 0);
    string str = string(path) + "\n";
    write(1,str.c_str(),str.length());

    if (original_output_channel != -1) {
        dup2(original_output_channel, 1);
        close(original_output_channel);
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd):
    BuiltInCommand(cmd_line), last_dir_pointer(plastPwd) {
    char** args = new char*[COMMAND_MAX_ARGS];
    int size = _parseCommandLine(cmd_line, args);
    valid_command = false;

    if (size > 2) {
        string str = "smash error: cd: too many arguments\n";
        write(1,str.c_str(),str.length());
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
                write(1,str.c_str(),str.length());
            }
        }
    }
}

void ChangeDirCommand::execute() {
    if (valid_command) {
        SmallShell::getInstance().set_last_dir(getcwd(NULL, 0));
        if (chdir(path) == -1) {
            string str = "some error message \n"; // needs to be redone
            write(2,str.c_str(),str.length());
        }
    }
}

AliasCommand::AliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line), cmd_line(cmd_line) {}

void AliasCommand::execute() {
    int original_output_channel = -1;
    if (SmallShell::getInstance().get_input_mode() == 1) {
        original_output_channel = dup(1); // may be illegal
        close(1);
        open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0777);
        // int fd = open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0777);
        // dup2(fd, 1);
        // close(fd);
    } else if (SmallShell::getInstance().get_input_mode() == 2) {
        original_output_channel = dup(1); // may be illegal
        close(1);
        open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND |O_RDWR, 0777);
        // int fd = open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND |O_RDWR, 0777);
        // dup2(fd, 1);
        // close(fd);
    }
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
                write(1,str.c_str(),str.length());
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
            write(1,str.c_str(),str.length());
        }
    }
    if (original_output_channel != -1) {
        dup2(original_output_channel, 1);
        close(original_output_channel);
    }
}

AliassedCommand::AliassedCommand(const char *cmd_line, string command) : BuiltInCommand(cmd_line),
cmd_line(cmd_line), command(command) {}

void AliassedCommand::execute() {
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
        write(1,str.c_str(),str.length());

    }
    else {
        for (int i = 1; i < size; ++i) {
            char* name = args[i];
            if (!SmallShell::getInstance().remove_alias(name)) {
                string str = "smash error: unalias: " + string(name) + " alias does not exist\n";
                write(1,str.c_str(),str.length());
                return;
            }
        }
    }
}

UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void UnSetEnvCommand::execute() {

}

SysInfoCommand::SysInfoCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void SysInfoCommand::execute() { // works somehow
    // struct utsname info;
    // uname(&info);
    //
    // cout << "System: " << info.sysname << endl;
    // cout << "Hostname: " << info.nodename << endl;
    // cout << "Kernel: " << info.release << endl;
    // cout << "Architecture: " << info.machine << endl;
    // string str = "smash error: unalias: " + string(name) + " alias does not exist\n";
    // write(1,str.c_str(),str.length());
}


Command::Command(const char* cmd_line): og_line(cmd_line) {
}

// add to Commands.cpp
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

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
            args.push_back(aug[i]);
            i ++;
        }
    }
    // store cmd_line if needed later
}

JobsList::JobsList() {
    init = new JobEntry();
}

void ExternalCommand::execute() {
    pid_t pid = fork();
    if (pid == 0) {
        if (SmallShell::getInstance().get_input_mode() == 1) {
            close(1);
            open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0777);
            // int fd = open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_TRUNC|O_RDWR, 0777);
            // dup2(fd, 1);
            // close(fd);
        }
        else if (SmallShell::getInstance().get_input_mode() == 2) {
            close(1);
            open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND |O_RDWR, 0777);
            // int fd = open(SmallShell::getInstance().get_output().c_str(), O_CREAT|O_APPEND |O_RDWR, 0777);
            // dup2(fd, 1);
            // close(fd);
        }


        setpgrp();
        execvp(args[0], args.data());
    } else if (pid > 0) {
        if (!is_background) {
            waitpid(pid, nullptr, 0);
        } else {
            SmallShell::getInstance().add_job(this);
        }
    }
    // your implementation or leave empty for now
}

const char* Command::get_line() const {
    return og_line;
}

const char *JobsList::JobEntry::get_line() {
    return line;
}

void SmallShell::add_job(Command* com) {
    J_list->addJob(com);
}

void JobsList::addJob(Command* com, bool baba) {
    new JobEntry(com, init);
}

void JobsList::printJobsList() {
    JobEntry* trav = init->next;
    while (string(trav->get_line()) != "aaaaa") {
        cout << trav->get_line() << ":" << endl;
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



