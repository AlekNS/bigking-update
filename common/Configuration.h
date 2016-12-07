
#ifndef CONFIGURATION_H
#define CONFIGURATION_H


#include <string>


#include <Poco/Util/AbstractConfiguration.h>


using std::string;


using Poco::Util::AbstractConfiguration;


//============================================================================//
class Configuration
{
public:

    static void checkForApplication(AbstractConfiguration &config);
    static void checkForSoftware(AbstractConfiguration &config);
    static void prepareApplicationLoggingFiles(AbstractConfiguration &config, const string &prefix = "");
    static void prepareApplicationLoggingDisableConsole(AbstractConfiguration &config);

};


#endif
