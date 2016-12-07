
#ifndef COMMAND_H
#define COMMAND_H


#include <string>
#include <vector>


#include <Poco/Path.h>


using std::string;
using std::vector;
using Poco::Path;


class Package;


//============================================================================//
class Command
{
public:

    enum OSTYPE
    {
        OS_WRONG        = 0,
        OS_UNKNOWN,
        OS_UNIX,
        OS_WINDOWS
    };

    Command();
    virtual ~Command();

    virtual bool execute(Package &package)  = 0;
    virtual bool undo(Package &package)     { return true; }
    virtual bool clean(Package &package)    { return true; }

    bool skipError() const;
    void skipError(const bool &enabled);

    int  osType() const;
    void osType(const string &type);

    virtual const char *name() const = 0;

protected:

    static void convertPathSeparator(string &str);

    bool        _skipError, _executed;
    int         _osType;

};


//============================================================================//
class CommandCopy : public Command
{
public:

    CommandCopy(const string &filePath, bool isOverwritable=true);

    virtual bool execute(Package &package);
    virtual bool undo(Package &package);
    virtual bool clean(Package &package);

    virtual const char *name() const;

private:

    string              _filePath;
    string              _filePathTemporary;
    bool                _overwritable;

};


//============================================================================//
class CommandRename : public Command
{
public:

    CommandRename(const string &filePathFrom, const string &filePathTo);

    virtual bool execute(Package &package);
    virtual bool undo(Package &package);

    virtual const char *name() const;

private:

    string              _filePathFrom;
    string              _filePathTo;

};


//============================================================================//
class CommandMove : public Command
{
public:

    CommandMove(const string &filePathFrom, const string &filePathTo);

    virtual bool execute(Package &package);
    virtual bool undo(Package &package);

    virtual const char *name() const;

private:

    string              _filePathFrom;
    string              _filePathTo;

};


//============================================================================//
class CommandDelete : public Command
{
public:

    CommandDelete(const string &filePath);

    virtual bool execute(Package &package);
    virtual bool undo(Package &package);
    virtual bool clean(Package &package);

    virtual const char *name() const;

private:

    string              _filePath;
    string              _filePathTemporary;

};


//============================================================================//
class CommandMakeFolder : public Command
{
public:

    CommandMakeFolder(const string &filePath);

    virtual bool execute(Package &package);
    virtual bool undo(Package &package);

    virtual const char *name() const;

private:

    string              _filePath;

};


//============================================================================//
class CommandShell : public Command
{
public:

    CommandShell(const string &shellCommands);

    virtual bool execute(Package &package);

    virtual const char *name() const;

private:

    string              _shellCommands;

};


//============================================================================//
class CommandScript : public Command
{
public:

    CommandScript(const string &filePath);

    virtual bool execute(Package &package);

    virtual const char *name() const;

private:

    string              _filePath;

};


//============================================================================//
class CommandPatch : public Command
{
public:

    CommandPatch(const string &filePath);

    virtual bool execute(Package &package);
    virtual bool undo(Package &package);
    virtual bool clean(Package &package);

    virtual const char *name() const;

private:

    string              _filePath;
    string              _filePathTemporary;

};


#endif
