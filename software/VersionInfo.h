
#ifndef VERSION_INFO_H
#define VERSION_INFO_H


#include <vector>
#include <string>


#include "../common/Version.h"


using std::pair;
using std::vector;
using std::string;


class Software;


//============================================================================//
struct VersionInfo
{
public:

    typedef vector<VersionInfo>  VersType;

    string      md5() const     { return _md5; }
    Version     from() const    { return _from; }
    Version     to() const      { return _to; }

    static void request(Software &software, VersType &versions);

private:

    static const Version        currentFormat;

    VersionInfo() { }
    VersionInfo(const string &md5, const Version &from, const Version &to)
    { this->_md5 = md5; this->_from = from; this->_to = to; }

    string              _md5;
    Version             _from;
    Version             _to;

};


#endif
