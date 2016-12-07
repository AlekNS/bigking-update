
#ifndef PACKAGE_H
#define PACKAGE_H


#include <string>
#include <vector>


#include <Poco/Path.h>
#include <Poco/DOM/Document.h>


#include "common/Software.h"
#include "common/Version.h"


using std::string;
using std::vector;


using Poco::Path;
using Poco::XML::Node;
using Poco::XML::Element;


class Command;


//============================================================================//
class Package
{
public:

    static const Version            currentFormat;

    Package(Software &software, const string &pathToPackage, const string &temporaryPath);
    virtual ~Package();

    bool                parse();

    bool                good() const            { return _good; }
    Path                path() const            { return _path; }

    Software            &software()             { return _software; }

    Version&            version()               { return _version; }

    vector<Command*>&   getCommandsWork()    { return _commandsWork; }
    vector<Command*>&   getCommandsSuccess() { return _commandsSuccess; }
    vector<Command*>&   getCommandsError()   { return _commandsError; }
    vector<Command*>&   getCommandsDone()    { return _commandsDone; }

protected:

    void                extractToTemporary(const string &packagePath, const string &extractToPath);

    void                parseNodeOfCommands(Node *element,
                            vector<Command*> &commands, const int level = 0);

    string              tempFolderToRemove;

    bool                _good;
    Software            &_software;

    Path                _path, _temporaryPath;

    Version             _version;

    vector<Command*>    _commandsWork,
                        _commandsSuccess,
                        _commandsError,
                        _commandsDone;

};


#endif
