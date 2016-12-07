
#include <common/stdafx.h>


#include <iostream>
#include <fstream>
#include <algorithm>


#include <Poco/Util/Application.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/NumberParser.h>
#include <Poco/Format.h>
#include <Poco/StringTokenizer.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/SAX/InputSource.h>


#include "Package.h"
#include "Command.h"

#include <Poco/UUIDGenerator.h>
#include "../common/DecompressZipHandler.h"

using namespace std;
using namespace Poco;
using namespace Poco::XML;
using namespace Poco::Util;


//============================================================================//
const Version Package::currentFormat = Version(0, 1, 1);

static const string     __specPackageInfoFileName__ = "info.xml",
                        __specPackageFilesFolder__  = "files";


//============================================================================//
Package::Package(Software &software, const string &pathToPackage, const string &temporaryPath) :
    _good(false),
    _path(Path(pathToPackage).makeDirectory().makeAbsolute()),
    _software(software),
    _temporaryPath(temporaryPath)
{
}

//============================================================================//
void __deleteCommandObject__(Command *command)
{
    delete command;
}

Package::~Package()
{
    for_each(_commandsWork.begin(), _commandsWork.end(), __deleteCommandObject__);
    for_each(_commandsSuccess.begin(), _commandsSuccess.end(), __deleteCommandObject__);
    for_each(_commandsError.begin(), _commandsError.end(), __deleteCommandObject__);
    for_each(_commandsDone.begin(), _commandsDone.end(), __deleteCommandObject__);

    if(tempFolderToRemove.size()) {
        try {
            File temp(tempFolderToRemove);
            if(temp.exists()) temp.remove(true);
        }
        catch(...) {
        }
    }
}


void Package::extractToTemporary(const string &packagePath, const string &extractToPath) {

    try {
        tempFolderToRemove = extractToPath;

        std::ifstream inputStream(packagePath.c_str(), std::ios::binary);

        if(!inputStream.good()) {
            throw Exception("reading of package");
        }

        Decompress extracter(inputStream, extractToPath);

        DecompressZipHandler handler;

        extracter.EError += DecompressZipHandlerType(&handler, &DecompressZipHandler::onError);
        extracter.decompressAllFiles();
        extracter.EError -= DecompressZipHandlerType(&handler, &DecompressZipHandler::onError);

        inputStream.close();

        if(handler.fail()) {
            throw Exception("one or more files was corrupted");
        }
    }

    catch(Exception &e) {
        poco_error_f1(Logger::get("Package"),
            "fail to extract package, %s", e.displayText());
        e.rethrow();
    }
    catch(...) {
        throw;
    }
}


//============================================================================//
bool Package::parse()
{
    File curPath(_path);

    if(!curPath.exists()) {
        return _good = false;
    }

    // check if specify package
    if(curPath.isFile()) {
        try {
            _temporaryPath.makeDirectory().makeAbsolute().
                pushDirectory(UUIDGenerator().createRandom().toString());
            extractToTemporary(_path.makeFile().toString(), _temporaryPath.toString());
            _path = _temporaryPath;
        }
        catch(...) {
            return _good = false;
        }
    }

    _path.pushDirectory(__specPackageFilesFolder__);

    // check package folder
    Path pathToInfo(_path.parent().setFileName(__specPackageInfoFileName__));

    try {
        if(!File(_path).exists()) {
            _good = false;
            return _good;
        }

        if(!File(pathToInfo).exists()) {
            _good = false;
            return _good;
        }
    }
    catch(...) { }

    ifstream inputStream(pathToInfo.toString().c_str(), ifstream::in);

    if(inputStream.fail()) {
        throw Exception("Can't parse becouse can't read data. Exists!? Locked!?");
        return false;
    }


    try {

        DOMParser           parser;
        InputSource         src(inputStream);
        AutoPtr<Document>   pDoc = parser.parse(&src);

        inputStream.close();

        Element             *rootNode      = pDoc->documentElement();

        if(!rootNode || rootNode->nodeName().compare("info")) {
            throw Exception("<info> isn't root");
        }

        Version         versionUpdateApplication;

        if(!rootNode->hasAttribute("update_version") ||
            !versionUpdateApplication.parse(rootNode->getAttribute("update_version"))) {

            throw Exception("unknown package format version");
        }

        if(versionUpdateApplication.compare(currentFormat)) {
            throw Exception("invalid package format version, available " +
                currentFormat.toString());
        }

        Element             *versionNode   = rootNode->getChildElement("version"),
                            *nameNode      = rootNode->getChildElement("identifier"),
                            *commandsNode  = rootNode->getChildElement("commands"),
                            *forSoftwareVersionNode = rootNode->getChildElement("for");

        if(!nameNode || nameNode->innerText().compare(_software.name())) {
            throw Exception("invalid software <identifier>");
        }

        if(!versionNode || !versionNode->innerText().size() ||
            !_version.parse(versionNode->innerText())) {
                throw Exception("invalid software <version>");
        }

        if(!commandsNode || !commandsNode->hasChildNodes()) {
            throw Exception("empty <commands>");
        }

        Node        *commandsWork    = commandsNode->getNodeByPath("/work"),
                    *commandsDone    = commandsNode->getNodeByPath("/done"),
                    *commandsSuccess = commandsNode->getNodeByPath("/success"),
                    *commandsError   = commandsNode->getNodeByPath("/error")
        ;

        if(!commandsWork) {
            throw Exception("<work> isn't exists");
        }

        if(forSoftwareVersionNode) {
            if(Version(forSoftwareVersionNode->innerText()).compare(_software.version())) {
                throw Exception("package and software version not equal " +
                    forSoftwareVersionNode->innerText() + ", is software was already updated!?");
            }
        }
        else {
            throw Exception("<for> isn't exists");
        }

        // commands parsing...
        parseNodeOfCommands(commandsWork, _commandsWork);

        if(!_commandsWork.size()) {
            throw Exception("no commands to execute, dummy updating, other platform?");
        }

        if(commandsDone) {
            parseNodeOfCommands(commandsDone, _commandsDone);
        }

        if(commandsSuccess) {
            parseNodeOfCommands(commandsSuccess, _commandsSuccess);
        }

        if(commandsError) {
            parseNodeOfCommands(commandsError, _commandsError);
        }

        _good = true;

    }
    catch(Exception &e) {
        _good = false;
        poco_error_f1(Logger::get("Package"),
            "invalid package " + __specPackageInfoFileName__ + " file, %s",
            e.displayText());
    }

    inputStream.close();

    return _good;
}

//============================================================================//
void Package::parseNodeOfCommands(Node *rootNode,
    vector<Command*> &commands, const int level)
{
    if(!rootNode) {
        poco_information(Logger::get("Package"),
            "parsing of commands get null root");
    }


    try {
        AutoPtr<NodeList>           nodeList = rootNode->childNodes();

        if(!nodeList->length()) {
            poco_information(Logger::get("Package"),
                "parsing of commands get 0 nodes");

            return;
        }

        // iterate over commands node
        for(int i = 0; i < nodeList->length(); i++) {
            Node    *node = nodeList->item(i);

            if(node->nodeType() != Node::ELEMENT_NODE)
                continue;

            Command                         *command        = NULL;
            AutoPtr<NamedNodeMap>            nodeAttributes = node->attributes();

            if(!node->nodeName().compare("delete")) {
                command = new CommandDelete(node->innerText());
            }
            else if(!node->nodeName().compare("copy")) {
                command = new CommandCopy(node->innerText(),
                    !(nodeAttributes->getNamedItem("overwrite") != NULL &&
                        !nodeAttributes->getNamedItem("overwrite")->nodeValue().compare("false")));
            }
            else if(!node->nodeName().compare("rename")) {
                Node *nodeFrom = node->getNodeByPath("/from"),
                     *nodeTo   = node->getNodeByPath("/to");
                if(!nodeFrom || !nodeTo)
                    throw Exception("invalid rename command format, must be <from> and <to> nodes exists");
                command = new CommandRename(nodeFrom->innerText(),
                    nodeTo->innerText());
            }
            else if(!node->nodeName().compare("move")) {
                Node *nodeFrom = node->getNodeByPath("/from"),
                     *nodeTo   = node->getNodeByPath("/to");
                if(!nodeFrom || !nodeTo)
                    throw Exception("invalid move command format, must be <from> and <to> nodes exists");
                command = new CommandMove(nodeFrom->innerText(),
                    nodeTo->innerText());
            }
            else if(!node->nodeName().compare("make_folder")) {
                command = new CommandMakeFolder(node->innerText());
            }
            else if(!node->nodeName().compare("script")) {
                command = new CommandScript(node->innerText());
            }
            else if(!node->nodeName().compare("shell")) {
                command = new CommandShell(node->innerText());
            }
            else if(!node->nodeName().compare("patch")) {
                command = new CommandPatch(node->innerText());
            }
            else {
                throw Exception("invalid command " + node->nodeName());
            }

            if(command) {
                command->skipError(nodeAttributes->getNamedItem("skip_error") != NULL &&
                    !nodeAttributes->getNamedItem("skip_error")->nodeValue().compare("true"));

                if(nodeAttributes->getNamedItem("os_type")) {
                    try {

                        // @TODO: Check what is the best node->getNodeByPath("/[@os_type]")?
                        command->osType(nodeAttributes->getNamedItem("os_type")->nodeValue());
                        if(command->osType() !=
#if defined(POCO_OS_FAMILY_WINDOWS)
                            Command::OS_WINDOWS
#elif defined(POCO_OS_FAMILY_UNIX)
                            Command::OS_UNIX
#else
#error OS platform not implemented yet
#endif
                        ) {
                            delete command;
                            continue;
                        }

                    }
                    catch(Exception &e) {
                        poco_error(Logger::get("Package"), "invalid command os_type");
                        delete command;
                        continue;
                    }
                    catch(...) {
                        delete command;
                        continue;
                    }
                }

                commands.push_back(command);
            }

        }

    }
    catch(Exception &e) {
        e.rethrow();
    }

}
