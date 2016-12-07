
#ifndef COMMANDS_EXECUTION_H
#define COMMANDS_EXECUTION_H


#include <vector>


class Command;
class Package;
class CommandsContext;


using std::vector;


//============================================================================//
class CommandsExecution
{
public:

    CommandsExecution(vector<Command*> &commands, Package &package);

    void                execute(CommandsContext &context);
    void                clean();
    void                rollback();

private:

    int                 _lastCommandExecutedIndex;
    
    vector<Command*>    &_commands;
    Package             &_package;

};


#endif
