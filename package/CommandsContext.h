
#ifndef COMMANDS_CONTEXT_H
#define COMMANDS_CONTEXT_H


class Command;
class Package;


//============================================================================//
class CommandsContext
{
public:

    CommandsContext() { }

    virtual void run(Command *command, Package &package);

};


//============================================================================//
class CommandsContextSkipError : public CommandsContext
{
public:

    CommandsContextSkipError() { }

    virtual void run(Command *command, Package &package);

};


#endif
