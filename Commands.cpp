#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <cstring>

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
    // TODO: add your implementation
    text_prompt = "smash> ";
    last_dir = nullptr;
}

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:
    /*
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
    }
    else if ...
    .....
    else {
      return new ExternalCommand(cmd_line);
    }
    */

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0) {
        return new ChangePromptCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, &getInstance().last_dir);
    }
    else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)

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

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl; // may need to change cout to the terminal
}

ShowPidCommand::ShowPidCommand(char const* cmd_line): BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char* path = getcwd(NULL, 0);
    cout << path << endl; // may need to change cout to the terminal
}

GetCurrDirCommand::GetCurrDirCommand(char const* cmd_line): BuiltInCommand(cmd_line) {}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd):
    BuiltInCommand(cmd_line), last_dir_pointer(plastPwd) {
    char** args = new char*[COMMAND_MAX_ARGS];
    int size = _parseCommandLine(cmd_line, args);

    if (size > 2) {
        cout << "smash error: cd: too many arguments" << endl;
    }
    else if (size == 2) {
        path = args[1];
        if (path[0] == '-' && path[1] == '\0') {
            if (*last_dir_pointer != nullptr) {
                path = *last_dir_pointer;
                //cout << path;
            }
            else {
                cout << "smash error: cd: OLDPWD not set" << endl;
            }
        }
    }
}

void ChangeDirCommand::execute() {
    if (valid_command) {
        SmallShell::getInstance().set_last_dir(getcwd(NULL, 0));
        chdir(path); // if this fails, needs to send an error
    }
}



Command::Command(const char* cmd_line) {
    // store cmd_line if needed later
}

// add to Commands.cpp
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {
}

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {
    // store cmd_line if needed later
}

void ExternalCommand::execute() {
    // your implementation or leave empty for now
}
