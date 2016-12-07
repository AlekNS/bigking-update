
#include <common/stdafx.h>


#include <Poco/File.h>
#include <Poco/Path.h>


#include "Configuration.h"


using namespace Poco;
using namespace Poco::Util;


//============================================================================//
void Configuration::checkForApplication(AbstractConfiguration &config)
{
    if(!config.has("identifier")) throw Exception(
        "<identifier> node doesn't exist");

    if(!config.has("temporary"))  throw Exception(
        "<temporary> node doesn't exist");

    if(!File(config.getString("temporary")).exists()) throw Exception(
        "<temporary> node refers to not exist path or access denied to the folder");

    if(!config.has("updateSoftwares")) throw Exception(
        "<updateSoftwares> node doesn't exist");

}

//============================================================================//
void Configuration::checkForSoftware(AbstractConfiguration &config)
{
    if(!config.has("path")) throw Exception(
        "<path> node doesn't exist");

    if(!File(config.getString("path")).exists()) throw Exception(
        "<path> node refers to not exist path or access denied to the folder");

    if(!config.has("uri"))  throw Exception(
        "<uri> node doesn't exist");

    if(!config.has("schedules")) throw Exception(
        "<schedules> doesn't exist");
}

//============================================================================//
void Configuration::prepareApplicationLoggingFiles(AbstractConfiguration &config, const string &prefix)
{

    // find all files loggers
    AbstractConfiguration::Keys             channelsKeys;
    AbstractConfiguration::Keys::iterator   keyIt;

    config.keys("logging.channels", channelsKeys);

    config.setString("application.logger", prefix);


    for(keyIt = channelsKeys.begin(); keyIt != channelsKeys.end(); ++keyIt) {
        if(!config.getString("logging.channels." + *keyIt + ".class", "").
            compare("FileChannel")) {

            config.setString("logging.channels." + *keyIt + ".path",
                Path(config.getString("logging.channels." + *keyIt + ".path", "") +
                    Path::separator()).makeAbsolute().
                    setFileName(
                        "UpSys" + prefix + 
                        config.getString("args.software_identifier")
                        + ".log").toString());
        }
    }

}

//============================================================================//
void Configuration::prepareApplicationLoggingDisableConsole(AbstractConfiguration &config)
{
    // find all files loggers
    AbstractConfiguration::Keys             channelsKeys;
    AbstractConfiguration::Keys::iterator   keyIt;

    config.keys("logging.channels", channelsKeys);

    for(keyIt = channelsKeys.begin(); keyIt != channelsKeys.end(); ++keyIt) {
        if(!config.getString("logging.channels." + *keyIt + ".class", "").
            compare("ConsoleChannel")) {

            config.setString("logging.channels." + *keyIt + ".class",
                "NullChannel");
        }
    }
}
