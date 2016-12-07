
#include <common/stdafx.h>


#include <Poco/File.h>
#include <Poco/Util/HelpFormatter.h>


#include "PackageSoftware.h"
#include "../common/Software.h"
#include "../common/Configuration.h"
#include "Package.h"
#include "Update.h"


using namespace std;


using namespace Poco;
using namespace Poco::Util;


//============================================================================//
PackageSoftware::PackageSoftware() :
    _mainLoopping(true)
{
    // display header
    cout << BIGKING_UPDATE_HEADER_TEXT
        "The application for update software from the extracted package (.up files)."
        << endl << endl;
}


//============================================================================//
void PackageSoftware::initialize(Application& self)
{
    if(!_mainLoopping)
        return;

    Path configPath(config().getString("application.dir", ""), "BigKingUpdate.xml");

    try {

        loadConfiguration(configPath.toString());

        if(!config().has("updateSoftwares." + config().getString("args.software_identifier")))
            throw Exception("invalid software argument identifier");

        Configuration::checkForApplication(config());
        Configuration::prepareApplicationLoggingFiles(config(), "Package");

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
void PackageSoftware::uninitialize()
{
    if(!_mainLoopping)
        return;

    Application::uninitialize();
}

//============================================================================//
void PackageSoftware::defineOptions(OptionSet& options)
{
    Application::defineOptions(options);

    options.addOption(Option("help",        "h",
        "display help information on command line arguments"));

    options.addOption(Option("noconsole",   "nc",
        "disable console output"));

    options.addOption(Option("path",        "p",
        "path to extracted files of package or file package",   true, "folder",     true));

    options.addOption(Option("software",    "s",
        "identifier of software to update",     true, "identifier", true));
}

//============================================================================//
void PackageSoftware::handleOption(const std::string& name, const std::string& value)
{
    Application::handleOption(name, value);

    if(!name.compare("help")) {
        displayHelp();
    }
    else if(!name.compare("path")) {
        config().setString("args.package_path", value);
    }
    else if(!name.compare("software")) {
        config().setString("args.software_identifier", value);
    }
    else if(!name.compare("noconsole")) {
        config().setBool("args.disable_console", true);
    }

}

//============================================================================//
void PackageSoftware::displayHelp()
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
int PackageSoftware::main(const std::vector<std::string>& args)
{
    if(!_mainLoopping) {
        return Application::EXIT_OK;
    }

    try {
        Software        software(config().getString("args.software_identifier"));


        poco_information_f1(logger(), "begin updating [%s]...", software.name());

        if(!software.versionRead()) {
            throw Exception("reading of software version file");
        }

        poco_information_f1(logger(), "current software version [%s]",
            software.version().toString());

        Package         package(software, config().getString("args.package_path"),
                config().getString("temporary"));
        if(!package.parse()) {
            throw Exception("reading of extracted package, may be corrupted?");
        }

        poco_information_f1(logger(), "updating software to version [%s]",
            package.version().toString());


        Update::softwareByExtractedPackage(package);

        poco_information_f1(logger(), "updating of [%s] is finished successful", software.name());

    }
    catch(Exception &e) {
        poco_error_f1(logger(), "ERROR: Exception raised [%s]",
            e.displayText());
        return 2;
    }
    catch(exception &e) {
        poco_error_f1(logger(), "ERROR: Execution raised [%s]",
            string(e.what()));
        return 2;
    }

    return Application::EXIT_OK;

}


POCO_APP_MAIN(PackageSoftware);
