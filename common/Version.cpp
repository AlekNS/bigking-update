
#include <common/stdafx.h>


#include <Poco/Format.h>
#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>


#include "Version.h"


using namespace std;


using namespace Poco;


//============================================================================//
const unsigned int Version::max = 0xFFFFFFFF;

const Version currentApplication(BIGKING_UPDATE_VERSION_STRING);


//============================================================================//
int Version::compare(unsigned int major, unsigned int minor, unsigned int release,
    unsigned int build) const
{
    if(_major < major)      return -1;
    if(_major > major)      return 1;

    if(_minor < minor)      return -1;
    if(_minor > minor)      return 1;

    if(_release < release)  return -1;
    if(_release > release)  return 1;

    if(_build < build)      return -1;
    if(_build > build)      return 1;

    return 0;
}


//============================================================================//
string Version::toString(unsigned int major,
    unsigned int minor, unsigned int release, unsigned int build) const
{
    if(build)
        return format("%d.%d.%d.%d",
            (int)(major), (int)(minor), (int)(release), (int)(build));
    return format("%d.%d.%d",
        (int)(major), (int)(minor), (int)(release));
}


//============================================================================//
bool Version::parse(const string &str)
{
    StringTokenizer st(str, ".", StringTokenizer::TOK_TRIM);
    StringTokenizer::Iterator it = st.begin();

    int         versionIndex = 0, versionPartParsed[4] = {max, max, max, 0};

    for(;it != st.end();++it, ++versionIndex) {
        if(!NumberParser::tryParse(*it,versionPartParsed[versionIndex])
            || versionIndex > 3 || versionPartParsed[versionIndex] < 0) {

            return false;
        }
    }

    _major   = versionPartParsed[0];
    _minor   = versionPartParsed[1];
    _release = versionPartParsed[2];
    _build   = versionPartParsed[3];

    return true;
}
