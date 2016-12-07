
#ifndef VERSION_H
#define VERSION_H


#ifndef BIGKING_UPDATE_VERSION_STRING
#define BIGKING_UPDATE_VERSION_STRING "0.1.4"

#define BIGKING_UPDATE_HEADER_TEXT \
        "Updating System by Alexander Sedelnikov. Version " BIGKING_UPDATE_VERSION_STRING "\n" \
        "BigKing-EIT. All rights reserved (c). "  __DATE__ ".\n"

#endif


#include <string>


using std::string;


//============================================================================//
class Version
{
public:

    const static unsigned int max;
    const static Version        currentApplication;

    Version() :
        _major(max), _minor(max), _release(max), _build(0) { }

    Version(const string &str) :
        _major(max), _minor(max), _release(max), _build(0)
            { parse(str); }

    Version(const unsigned int &major,
            const unsigned int &minor,
            const unsigned int &release,
            const unsigned int &build = 0) :
                _major(major), _minor(minor), _release(release), _build(build) { }

    unsigned int    major() const { return _major; }
    unsigned int    minor() const { return _minor; }
    unsigned int    release() const { return _release; }
    unsigned int    build() const { return _build; }

    const bool      good() const { return _minor != max && _major != max && _release != max; }

    string          toString() const
                        { return toString(_major, _minor, _release, _build); }

    int             compare(unsigned int major, unsigned int minor,
                            unsigned int release,
                            unsigned int build = 0) const;

    int             compare(const Version &v) const
                        { return compare(v.major(), v.minor(), v.release(), v.build()); }

    bool            parse(const string &str);

protected:

    string          toString(unsigned int major, unsigned int minor,
                                    unsigned int release,
                                    unsigned int build) const;

    unsigned int    _major, _minor, _release, _build;

};


#endif
