
#ifndef SOFTWARE_H
#define SOFTWARE_H


#include <string>


#include <Poco/Util/AbstractConfiguration.h>


#include "Version.h"


using std::string;


using Poco::AutoPtr;
using Poco::Util::AbstractConfiguration;


//============================================================================//
class Software
{
public:

    Software(const string &name);

    Version&        version();
    bool            versionRead();
    bool            versionUpdate(const Version &v);

    AbstractConfiguration &config();

    bool            good()              const;

    string          path()              const;
    string          name()              const;
    string          platform()          const;
    string          tag()               const;

protected:

    AutoPtr<AbstractConfiguration>      _config;


    string                              _tag;
    string                              _platform;
    string                              _name;
    bool                                _good;

    Version                             _version;

};


#endif
