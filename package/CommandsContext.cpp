
#include <common/stdafx.h>


#include "CommandsContext.h"
#include "Command.h"
#include "Package.h"


using namespace std;


using namespace Poco;


//============================================================================//
void CommandsContext::run(Command *command, Package &package)
{
    try {
        if(!command->execute(package) && !command->skipError()) {
            throw Exception("Command execution failed then stop sequences of actions");
        }
    }
    catch(Exception &e) {
        e.rethrow();
    }
    catch(exception &e) {
        throw e;
    }
}

//============================================================================//
void CommandsContextSkipError::run(Command *command, Package &package)
{
    try {
        command->execute(package);
    }
    catch(exception &e) {
    }
}
