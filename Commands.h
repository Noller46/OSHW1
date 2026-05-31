// Ver: 04-11-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <cstring>
//#include <iostream>
#include <memory>
#include "Commands.h"

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using namespace std;

// struct Alias_Node {
//     string alias;
//     string command;
//     Alias_Node *next;
// };


class Command {
    // TODO: Add your data members
    const char * og_line;
public:
    Command(const char *cmd_line);

    virtual ~Command() = default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed

    const char * get_line() const;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ExternalCommand : public Command {
private:
    std::vector<char*> args;
    bool is_background;
    char * my_name;
public:
    ExternalCommand(const char *cmd_line);

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

class USBInfoCommand : public Command {
    // TODO: Add your data members **BONUS: 10 Points**
public:
    USBInfoCommand(const char *cmd_line);

    virtual ~USBInfoCommand() {
    }

    void execute() override;
};

//tested
class ChangePromptCommand : public BuiltInCommand {
    // TODO: Add your data members public:
private:
    string prompt;

public:
    ChangePromptCommand(const char* cmd_line);

    virtual ~ChangePromptCommand() {
    }

    void execute() override; // removed override hear, may be wrong to do so;
};

//tested
class ChangeDirCommand : public BuiltInCommand {
    // TODO: Add your data members public:
private:
    char* path;
    char** last_dir_pointer;
    bool valid_command;

public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

//tested
class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

//tested
class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

//only after background
class JobsList;

//only after background
class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {
    }

    void execute() override;
};

//only after background
class JobsList {
public:
    class JobEntry;
private:
    JobEntry* init;
public:
    class JobEntry {
        // TODO: Add your data members
        char * line;
        //pid_t pid;
    public:
        JobEntry* next;
        JobEntry* prev;
        JobEntry(): line(strdup("aaaaa")) {
            next = this;
            prev = this;
        }

        JobEntry(Command* com, JobEntry* next): line(strdup(com->get_line())), next(next) {
            prev = next->prev;
            this->next = next;

            prev->next = this;
            next->prev = this;
        }
        ~JobEntry() {
            next->prev = prev;
            prev->next = next;
            free(line);
        }

        const char* get_line();
    };

    // TODO: Add your data members
public:
    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    // TODO: Add extra methods or modify exisitng ones as needed
};

//only after background
class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {
    }

    void execute() override;
};

//only after background
class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {
    }

    void execute() override;
};

//only after background
class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

//tested
class AliasCommand : public BuiltInCommand {
private:
    const char* cmd_line;
public:
    AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() {
    }

    void execute() override;
};

//tested
class AliassedCommand : public BuiltInCommand {
    const char* cmd_line;
    string command;
public:
    AliassedCommand(const char *cmd_line, string command);

    virtual ~AliassedCommand() {
    }

    void execute() override;
};

//tested
class UnAliasCommand : public BuiltInCommand {
private:
    const char* cmd_line;
public:
    UnAliasCommand(const char *cmd_line);

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

class SysInfoCommand : public BuiltInCommand {
public:
    SysInfoCommand(const char *cmd_line);

    virtual ~SysInfoCommand() {
    }

    void execute() override;
};


class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
    string text_prompt;
    char* last_dir;
    vector<tuple<string, string, string>> alias_list;
    JobsList* J_list;
    int input_mode;
    string input;
    string output;


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

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed

    string get_text_prompt();

    void change_prompt(string str = "smash> ");

    char* get_last_dir();

    void set_last_dir(char* str);

    vector<tuple<string, string, string>> get_alias_list();

    void add_alias(string alias, string command, string cmd_line);

    bool remove_alias(string alias);

    void add_job(Command* com);

    int get_input_mode();

    void set_input_mode(int mode);

    string get_input();

    string get_output();
};


string replace_first_word(const char* cmd_line, string command);


#endif //SMASH_COMMAND_H_
