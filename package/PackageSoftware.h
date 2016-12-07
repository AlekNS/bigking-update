
#include <string>
#include <vector>


#include <Poco/Util/Application.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>


using std::string;
using std::vector;


using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;


//============================================================================//
class PackageSoftware : public Application
{
public:

    PackageSoftware();

private:

    int main(const std::vector<std::string>& args);

    void initialize(Application& self);
    void uninitialize();

    void defineOptions(OptionSet& options);
    void handleOption(const std::string& name, const std::string& value);

    void displayHelp();

    bool                _mainLoopping;

};
