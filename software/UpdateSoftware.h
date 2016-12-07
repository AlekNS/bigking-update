
#ifndef UPDATE_SOFTWARE_H
#define	UPDATE_SOFTWARE_H


#include <Poco/Util/Application.h>


#include "../common/Software.h"


using std::string;

using Poco::Logger;
using Poco::Util::Application;
using Poco::Util::OptionSet;


//============================================================================//
class UpdateSoftware : public Application
{
public:

    UpdateSoftware();

private:

    int main(const std::vector<std::string>& args);

    void initialize(Application& self);
    void uninitialize();

    void defineOptions(OptionSet& options);
    void handleOption(const std::string& name, const std::string& value);

    void displayHelp();

    bool                    _mainLoopping;

};


#endif
