
#include <common/stdafx.h>


#include <sstream>
#include <iostream>


#include <Poco/File.h>
#include <Poco/LocalDateTime.h>
#include <Poco/Logger.h>
#include <Poco/Pipe.h>
#include <Poco/Process.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>
#include <Poco/URI.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>


#include "UpdateSoftware.h"
#include "../common/Configuration.h"
#include "ProcessUpdate.h"
#include "../common/Software.h"


using namespace std;


using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;


//============================================================================//
UpdateSoftware::UpdateSoftware() :
    _mainLoopping(true)
{
    // display header
    cout << BIGKING_UPDATE_HEADER_TEXT
        "The application for downloading packages and update software by them."
        << endl << endl;
}

//============================================================================//
void UpdateSoftware::initialize(Application& self)
{
    if(!_mainLoopping)
        return;

    Path configPath(config().getString("application.dir"), "BigKingUpdate.xml");

    try {

        loadConfiguration(configPath.toString());

        if(!config().has("args.package_application")) {
            config().setString("args.package_application",
                Path(config().getString("application.dir"),
#if defined(POCO_OS_FAMILY_WINDOWS)
                    "BigKingUpdatePackage.exe"
#else
                    "BigKingUpdatePackage"
#endif
                ).toString());
        }

        if(!config().has("updateSoftwares." + config().getString("args.software_identifier"))) {
            throw Exception("invalid software argument identifier");
        }

        Configuration::checkForApplication(config());
        Configuration::prepareApplicationLoggingFiles(config(), "Software");

        if(config().has("args.disable_console"))
            Configuration::prepareApplicationLoggingDisableConsole(config());

    }
    catch(Exception &e) {
        _mainLoopping = false;
        e.rethrow();
    }
    catch(...) {
        _mainLoopping = false;
        throw;
    }

    Application::initialize(self);

}

//============================================================================//
void UpdateSoftware::uninitialize()
{
    if(!_mainLoopping)
        return;

    Application::uninitialize();
}

//============================================================================//
void UpdateSoftware::defineOptions(OptionSet& options)
{
    Application::defineOptions(options);

    options.addOption(Option("help",        "h",
        "display help information on command line arguments"));

    options.addOption(Option("noconsole",   "nc",
        "disable console output"));

    options.addOption(Option("software",    "s",
        "identifier of software to update",     true, "identifier", true));

    options.addOption(Option("packageapp",  "pa",
        "path to package application name",     false, "path",      true));

}

//============================================================================//
void UpdateSoftware::handleOption(const string& name, const string& value)
{

    if(!name.compare("help")) {
        displayHelp();
    }
    else if(!name.compare("packageapp")) {
        config().setString("args.package_application",
            Path(value).makeFile().makeAbsolute().toString());
    }
    else if(!name.compare("software")) {
        config().setString("args.software_identifier", value);
    }
    else if(!name.compare("noconsole")) {
        config().setBool("args.disable_console", true);
    }

}

//============================================================================//
void UpdateSoftware::displayHelp()
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
int UpdateSoftware::main(const std::vector<std::string>& args)
{

    if(!_mainLoopping) {
        return Application::EXIT_OK;
    }

    try {
        Software software(config().getString("args.software_identifier"));
        return ProcessUpdate::run(software);
    }
    catch(Exception &e) {
    }
    catch(...) {
    }

    return 3;

}


POCO_APP_MAIN(UpdateSoftware);
