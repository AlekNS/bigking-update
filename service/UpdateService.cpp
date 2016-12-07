
#include <common/stdafx.h>


#include <Poco/Pipe.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/Process.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/OptionCallback.h>


#include "UpdateService.h"
#include "../common/Configuration.h"


using namespace Poco;
using namespace Poco::Util;
using namespace std;


//============================================================================//
UpdateService::UpdateService() :
    _mainLoopping(true)
{
    // display header
    cout << BIGKING_UPDATE_HEADER_TEXT
#if defined(BUILD_UPDATE_SERVICE)
        "The service for auto-update softwares."
#else
        "The software for auto-update softwares."
#endif
        << endl << endl;
}

//============================================================================//
void UpdateService::initialize(Application& self)
{
    if(!_mainLoopping)
        return;

    Path configPath(config().getString("application.dir", ""), "BigKingUpdate.xml");

    try {

        loadConfiguration(configPath.toString());

        config().setString("args.software_identifier", "");

        Configuration::checkForApplication(config());
        Configuration::prepareApplicationLoggingFiles(config(),
#if defined(BUILD_UPDATE_SERVICE)
                "BigKingUpdateService"
#else
                "BigKingUpdateMaintainer"
#endif
        );

#if defined(BUILD_UPDATE_SERVICE)
        if(!isInteractive())
            Configuration::prepareApplicationLoggingDisableConsole(config());
#endif

    }
    catch(Exception &e) {
        e.rethrow();
    }
    catch(...) {
        throw;
    }

#if defined(BUILD_UPDATE_SERVICE)
    ServerApplication::initialize(self);
#else
    Application::initialize(self);
#endif


    AbstractConfiguration &conf = Application::instance().config();

    poco_information(logger(), "clean all files in the temporary folder...");

    vector<string>          tempFiles;
    File rootOfTempFiles(conf.getString("temporary"));

    rootOfTempFiles.createDirectories();
    rootOfTempFiles.list(tempFiles);
    for(int i = 0; i < tempFiles.size(); i++) {
        try {
            File(rootOfTempFiles.path() + Path::separator() +
                tempFiles[i]).remove(true);
        }
        catch(Exception &e) {
            poco_information_f1(logger(), "remove file/folder failed with [%s]",
                e.displayText());
        }
    }

#if defined(BUILD_UPDATE_SERVICE)
    poco_information(logger(), "launch self update service...");

    if(!conf.has("updateSoftwares.BigKingUpdateService")) {
        throw Exception("ERROR: Self updating service must be configured BigKingUpdateService node.");
    }

    _worker = new Worker("BigKingUpdateService", false);
    _workers.push_back(_worker);

    poco_information(logger(), "launch mainteiner updating worker...");

    Thread::sleep(10000); // late start

    _worker->start();

#else

    poco_information(logger(), "launch updating workers for specified softwares...");

    // Iterate over softwares
    AbstractConfiguration::Keys         softwaresKeys;

    conf.keys("updateSoftwares", softwaresKeys);


    for(int i = 0; i < softwaresKeys.size(); i++) {
        if(!softwaresKeys[i].compare("BigKingUpdateService"))
            continue;

        Worker     *worker = new Worker(softwaresKeys[i]);

        _workers.push_back(worker);
        poco_information_f1(logger(), "launch updating worker %s...", softwaresKeys[i]);
        worker->start();
    }

    if(!_workers.size()) {
        throw Exception("ERROR: No softwares for updating");
    }
#endif

    poco_information(logger(), "initialization complete");

}

//============================================================================//
void UpdateService::uninitialize()
{

    if(!_mainLoopping)
        return;

    poco_information(logger(), "shutting down");

    poco_information(logger(), "send to stop workers...");
    // stop workers
    for(int i = 0; i < _workers.size(); i++) {
        _workers[i]->stop();
    }

    poco_information(logger(), "wait stopping workers...");
    // wait stop workers
    for(int i = 0; i < _workers.size(); i++) {
        _workers[i]->waitUntilStop();
        delete _workers[i];
    }

    poco_information(logger(), "uninitialize complete");

#if defined(BUILD_UPDATE_SERVICE)
    ServerApplication::uninitialize();
#else
    Application::uninitialize();
#endif
}

//============================================================================//
void UpdateService::defineOptions(OptionSet& options)
{

#if defined(BUILD_UPDATE_SERVICE)
    ServerApplication::defineOptions(options);
#else
    Application::defineOptions(options);
#endif

    options.addOption(Option("help",        "h",
        "display help information for command line arguments"));

    options.addOption(Option("generate",    "g",
        "generate unique identifie"));

    options.addOption(Option("config",      "f",
        "load configuration data from a file", false, "path", true));

}

//============================================================================//
void UpdateService::handleOption(const std::string& name,
        const std::string& value)
{

#if defined(BUILD_UPDATE_SERVICE)
	ServerApplication::handleOption(name, value);
#else
    Application::handleOption(name, value);
#endif

    if(!name.compare("help")) {
        displayHelp();
    }
    else if(!name.compare("generate")) {
        cout << "UUID: " << UUIDGenerator().createRandom().toString() << endl;
        _mainLoopping = false;
        stopOptionsProcessing();
    }
    else if(!name.compare("config")) {
        loadConfiguration(value);
        _mainLoopping = true;
    }

}

//============================================================================//
void UpdateService::displayHelp()
{
    _mainLoopping = false;

    HelpFormatter helpFormatter(options());

    helpFormatter.setIndent(4);
    helpFormatter.setWidth(80);
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("{options} ...");
    helpFormatter.setHeader("\noptions:");
    helpFormatter.format(std::cout);

    stopOptionsProcessing();
}

//============================================================================//
int UpdateService::main(const std::vector<std::string>& args)
{
    if(!_mainLoopping)
        return Application::EXIT_OK;

    poco_information(logger(), "enter main loop...");

#if defined(BUILD_UPDATE_SERVICE)

    _worker->wait();

    Pipe                outPipe,
                        errPipe;

    Process::Args       processArgs;
    Process::Env        processEnv;


    poco_information(logger(), "launch mainteiner process...");

    string      servicePath = Application::instance().config().getString("application.dir", "") + Path::separator();
    string      prefix =
#if defined(POCO_OS_FAMILY_WINDOWS)
                                ".exe"
#elif defined(POCO_OS_FAMILY_UNIX)
                                ""
#endif
    ;

    ProcessHandle       handle =
        Process::launch(
            servicePath + "BigKingUpdateMaintainer" + prefix,
            processArgs,
            Application::instance().config().getString("application.dir"),
            NULL,
            NULL, //&outPipe,
            NULL, //&errPipe,
            processEnv
        );

    poco_information(logger(), "wait termination request...");

    waitForTerminationRequest();

    poco_information(logger(), "stop maintainer process...");
    try {
        Process::kill(handle);
    }
    catch(...) {
        poco_warning(logger(), "can't kill maintainer process! is it stop!?");
    }
/*
    int executeResult = handle.wait();

    if(executeResult < 2) {
        poco_information(logger(), "result of maintainer execution success!");
    }
    else {
        poco_error_f1(logger(), "result of maintainer execution failed, exit code %d",
            executeResult);
    }
*/
#else

//    string temporaryString;
//    while(temporaryString.compare("exit")) {
//    cin >> temporaryString;
//    }
    for(int i = 0; i < _workers.size(); i++) {
        _workers[i]->wait();
    }

#endif

    poco_information(logger(), "leave main loop.");

    return Application::EXIT_OK;
}

#if defined(BUILD_UPDATE_SERVICE)
POCO_SERVER_MAIN(UpdateService);
#else
POCO_APP_MAIN(UpdateService);
#endif
