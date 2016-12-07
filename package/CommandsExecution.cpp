
#include <common/stdafx.h>


#include "CommandsExecution.h"
#include "CommandsContext.h"
#include "Command.h"


using namespace std;


using namespace Poco;

//============================================================================//
CommandsExecution::CommandsExecution(vector<Command*> &commands, Package &package) :
    _commands(commands),
    _package(package)
{
}

//============================================================================//
void CommandsExecution::execute(CommandsContext &context)
{

    _lastCommandExecutedIndex = 0;

    vector<Command*>::iterator          it = _commands.begin();

    for(; it != _commands.end(); ++it, ++_lastCommandExecutedIndex) {
        try {
            context.run(*it, _package);
        }
        catch(Exception &e) {
            poco_error_f1(Logger::get("CommandsExecution"), "stop command execution [%s]",
                string((*it)->name()));
            e.rethrow();
        }
        catch(exception &e) {
            throw e;
        }
    }

}

//============================================================================//
void CommandsExecution::clean()
{
    try {
        reverse_iterator<vector<Command*>::iterator> it = _commands.rbegin();
        for(;it != _commands.rend(); ++it) {
            if(!(*it)->clean(_package)) {
                throw;
            }
        }
    }
    catch(...) {
    }
}

//============================================================================//
void CommandsExecution::rollback()
{
    try {
        for(int index = _lastCommandExecutedIndex; index >= 0; index--) {
            if(!_commands[index]->undo(_package)) {
                throw;
            }
        }
    }
    catch(...) {
    }
}
