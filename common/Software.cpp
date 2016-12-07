
#include <common/stdafx.h>


#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/ConfigurationView.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/XML/XMLWriter.h>


#include "Software.h"


using namespace std;


using namespace Poco;
using namespace Poco::Util;
using namespace Poco::XML;


//============================================================================//
static const string _specFileNameSuffix = ".version";


//============================================================================//
Software::Software(const string &name) :
    _config(new ConfigurationView("updateSoftwares." + name,
            &Application::instance().config())),
    _platform(
#if defined(POCO_OS_FAMILY_WINDOWS)
        "win"
    #if defined(_WIN64)
            "64"
    #else
            "32"
    #endif
#elif defined(POCO_OS_FAMILY_UNIX)
        "lin64"
#else
#error OS platform not implemented yet
#endif
    ),
    _name(name),
    _good(false),
    _version()
{
}

//============================================================================//
AbstractConfiguration& Software::config()
{
    return *_config;
}

//============================================================================//
string Software::path() const
{
    return Path(_config->getString("path")).makeDirectory().makeAbsolute().toString();
}

//============================================================================//
string Software::name() const
{
    return _name;
}

//============================================================================//
string Software::platform() const
{
    return _platform;
}

//============================================================================//
string Software::tag() const
{
    return _tag;
}

//============================================================================//
bool Software::good() const
{
    return _good;
}

//============================================================================//
Version& Software::version()
{
    return _version;
}

//============================================================================//
bool Software::versionRead()
{
    Path path(_config->getString("path"), string(".") + _name + _specFileNameSuffix);

    ifstream inputStream(path.toString().c_str(), ifstream::in);
    if(inputStream.fail()) {
        throw Exception("Service failed to read software version file! Read/Write lock!? Exists?");
    }

    InputSource src(inputStream);

    try {
        DOMParser           parser;
        AutoPtr<Document>   pDoc = parser.parse(&src);
        Element             *rootNode = pDoc->documentElement();

        if(!rootNode || rootNode->tagName() != "info") {
            throw Exception("<info> as root node doesn't exist");
        }

        Element          *nameNode    = rootNode->getChildElement("identifier"),
                         *versionNode = rootNode->getChildElement("version"),
                         *platformNode= rootNode->getChildElement("platform"),
                         *tagNode     = rootNode->getChildElement("tag");

        if(!nameNode || nameNode->innerText().compare(_name)) {
            throw Exception("<identifier> node doesn't match with software identifier: " + _name);
        }

        if(!versionNode || !versionNode->innerText().size()) {
            throw Exception("<version> node doesn't exist");
        }
        if(!_version.parse(versionNode->innerText())) {
            throw Exception("<version> node contain invalid data, format of data must be [major].[minor].[release].[build]");
        }

        if(platformNode && platformNode->innerText().size()) {
            _platform = platformNode->innerText();
        }
        else {
            poco_warning_f1(Logger::get("Software"), "platform of software isn't defined [%s]",
                _name);
        }

        if(tagNode && tagNode->innerText().size() > 0) {
            _tag = tagNode->innerText();
        }

        _good = true;

    }
    catch(Exception &e) {
        _good = false;
        poco_error_f2(Logger::get("Software"), "Invalid softaware [%s] version file or validation was failed, %s.",
            _name,
            string(e.displayText()));
    }
    inputStream.close();

    return _good;
}

//============================================================================//
bool Software::versionUpdate(const Version &newVersion)
{
    if(!_good || !newVersion.good())
        return false;

    Path path(_config->getString("path"), string(".") + _name + _specFileNameSuffix);

    ifstream inputStream;
    ofstream outputStream;

    try {

        inputStream.open(path.toString().c_str(), ifstream::in);
        if(inputStream.fail()) {
            throw Exception("access denied or file doesn't exist: " + path.toString());
        }

        DOMParser           parser;
        InputSource         src(inputStream);
        AutoPtr<Document>   pDoc = parser.parse(&src);

        inputStream.close();

#if defined(POCO_OS_FAMILY_WINDOWS)
        if(File(path.toString()).isHidden()) {
            DWORD attr = GetFileAttributes(path.toString().c_str());
            if(attr == -1) {
                poco_error_f2(Logger::get("Software"), "access denied to read attribute of software version "
                    "file for [%s] with error status [%d]",
                    _name, attr);

            }
            else {
                attr &= ~FILE_ATTRIBUTE_HIDDEN;

                if(SetFileAttributes(path.toString().c_str(), attr) == 0) {
                    throw Exception("remove hidden attribute for software version file: " + path.toString());
                }
            }
        }
#endif

        outputStream.open(path.toString().c_str(), ofstream::out);

        if(outputStream.fail()) {
            throw Exception("opening software version file for writing was failed");
        }

        Element                 *rootNode    = pDoc->documentElement(),
                                *versionNode = rootNode->getChildElement("version");

        // simple check right structure of xml
        if(!versionNode) {
            throw Exception("<version> node doesn't exists");
        }

        AutoPtr<NodeList>       versionChilds = versionNode->childNodes();

        if(!versionChilds->length()) {
            throw Exception("<version> node date is empty, format of data must be [major].[minor].[release].[build]");
        }

        Node                    *versionTextNode = versionChilds->item(0);

        versionNode->replaceChild(pDoc->createTextNode(newVersion.toString()),
            versionTextNode);

        // write formated software version file
        DOMWriter writer;
        writer.setEncoding("utf-8", TextEncoding::byName("utf-8"));
        writer.setOptions(
            XMLWriter::WRITE_XML_DECLARATION|
            XMLWriter::PRETTY_PRINT
        );
        writer.writeNode(outputStream, pDoc);

        // cache new software version
        _version = newVersion;
    }
    catch(Exception &e) {
        _good = false;

        inputStream.close();

        poco_error_f2(Logger::get("Software"), "Writing to software version file was failed [%s] version file, [%s]",
            _name,
            string(e.displayText()));
    }

    outputStream.close();

    return _good;
}

