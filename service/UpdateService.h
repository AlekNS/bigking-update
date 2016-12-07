
#ifndef SMITLAB_SERVICE_H
#define	SMITLAB_SERVICE_H


#include <vector>


#include <Poco/Util/ServerApplication.h>


#include "Worker.h"


using std::vector;


using Poco::Logger;
using Poco::Util::Application;
using Poco::Util::OptionSet;
using Poco::Util::ServerApplication;



//============================================================================//
class UpdateService : public
#if defined(BUILD_UPDATE_SERVICE)
    ServerApplication
#else
    Application
#endif
{
public:

	UpdateService();

private:

    int main(const std::vector<std::string>& args);

    void initialize(Application& self);
    void uninitialize();

    void defineOptions(OptionSet& options);
    void handleOption(const std::string& name, const std::string& value);

    void displayHelp();

    bool                        _mainLoopping;

    vector<Worker*>             _workers;

#if defined(BUILD_UPDATE_SERVICE)
    Worker                      *_worker;
#endif

};


#endif

