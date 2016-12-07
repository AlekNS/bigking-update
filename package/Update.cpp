
#include <common/stdafx.h>


#include <Poco/Exception.h>


#include "Update.h"
#include "CommandsExecution.h"
#include "CommandsContext.h"


using namespace std;


using namespace Poco;


//============================================================================//
void Update::softwareByExtractedPackage(Package &package)
{
    CommandsExecution       workExecutor(package.getCommandsWork(), package),
                            successExecutor(package.getCommandsSuccess(), package),
                            errorExecutor(package.getCommandsError(), package),
                            doneExecutor(package.getCommandsDone(), package);

    CommandsContext             contextNormal;
    CommandsContextSkipError    contextSkipError;

    bool failedUpdating = false;

    try {
        workExecutor.execute(contextNormal);
        workExecutor.clean();

        try {
            package.software().versionUpdate(package.version());
        }
        catch(...) {
        }

        successExecutor.execute(contextSkipError);
        successExecutor.clean();
    }
    catch(Exception &e) {
        failedUpdating = true;

        errorExecutor.execute(contextSkipError);
        errorExecutor.clean();

        workExecutor.rollback();
        workExecutor.clean();

        doneExecutor.execute(contextSkipError);
        doneExecutor.clean();

        e.rethrow();
    }

    doneExecutor.execute(contextSkipError);
    doneExecutor.clean();
}
