
#include <common/stdafx.h>


#include <Poco/StreamCopier.h>
#include <Poco/URI.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Util/Application.h>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/XML/XMLWriter.h>


#include "../common/Software.h"
#include "VersionInfo.h"


using namespace std;


using namespace Poco;
using namespace Poco::XML;
using namespace Poco::Util;
using namespace Poco::Net;


//============================================================================//
const Version VersionInfo::currentFormat(0, 0, 1);


//============================================================================//
bool __versionPairCompare__(const VersionInfo &a, const VersionInfo &b)
{
    return a.from().compare(b.from()) < 0;
}


void VersionInfo::request(Software &software, VersType &versions)
{
    versions.clear();

    try {
        if(!software.versionRead()) {
            throw Exception("invalid software version file");
        }

        bool findedCurrentVersion = false;

        //
        // Request versions from the server.
        //
        URI               uri(software.config().getString("uri") + "/versions.xml");
        HTTPClientSession serverSession(uri.getHost(), uri.getPort());

        std::string      requestPath(uri.getPathAndQuery());
        if(requestPath.empty())
            requestPath = "/";

        HTTPRequest     serverRequest(HTTPRequest::HTTP_GET, requestPath, HTTPMessage::HTTP_1_1);
        HTTPResponse    serverResponse;
        serverSession.sendRequest(serverRequest);

        // receive data
        std::istream& receivedStream = serverSession.receiveResponse(serverResponse);
        if(HTTPResponse::HTTP_OK != serverResponse.getStatus()) {
            throw Exception(format("http request failed, error = %d",
                (int)serverResponse.getStatus()));
        }

        //
        // Receive and parse
        //
        DOMParser         parser;
        InputSource       inSource(receivedStream);
        AutoPtr<Document> pDoc = parser.parse(&inSource);

        Element          *rootNode      = pDoc->documentElement(),
                         *versionsNode  = rootNode->getChildElement("versions"),
                         *nameNode      = rootNode->getChildElement("identifier")
        ;

        //
        // Check format
        //
        if(!rootNode || rootNode->nodeName().compare("updates") ||
            !versionsNode || !nameNode) {

            throw Exception("invalid format of information versions, check format");
        }

        if(!rootNode->hasAttribute("format") ||
            Version(rootNode->getAttribute("format")).compare(currentFormat) > 0
        ) {
            throw Exception("invalid information versions, supported format " +
                currentFormat.toString());
        }

        if(nameNode->innerText().compare(software.name())) {
            throw Exception("invalid information versions, wrong software identifier");
        }

        //
        // Iterate over platforms nodes
        //
        bool                    platformFound = false;
        AutoPtr<NodeList>       platformsNodeList = versionsNode->childNodes();
        for(int i = 0; i < platformsNodeList->length(); i++) {
            Node    *platformNode = platformsNodeList->item(i);
            if(platformNode->nodeType() != Node::ELEMENT_NODE)
                continue;

            AutoPtr<NamedNodeMap>  nodeAttributes = platformNode->attributes();

            // check format
            if(!nodeAttributes->getNamedItem("type")) {
                poco_warning(Logger::get("VersionInfo"), "info.xml has wrong platform tag without type attribute");
                continue;
            }

            if(nodeAttributes->getNamedItem("type")->innerText().compare(software.platform())) {
                continue;
            }

            if(!nodeAttributes->getNamedItem("tag") && software.tag().size() > 0)               continue;
            if(nodeAttributes->getNamedItem("tag")) {
                if(nodeAttributes->getNamedItem("tag")->innerText().size() && !software.tag().size())   continue;
                if(software.tag().compare(nodeAttributes->getNamedItem("tag")->innerText()))            continue;
            }

            platformFound = true;

            AutoPtr<NodeList>       versionsNodeList = platformNode->childNodes();
            for(int i = 0; i < versionsNodeList->length(); i++) {

                Node    *node = versionsNodeList->item(i);

                if(node->nodeType() != Node::ELEMENT_NODE)
                    continue;

                AutoPtr<NamedNodeMap>  nodeAttributes = node->attributes();

                // check format
                if(node->nodeName().compare("up") ||
                    !nodeAttributes->getNamedItem("for") ||
                    !nodeAttributes->getNamedItem("to") ||
                    !nodeAttributes->getNamedItem("md5")
                ) {
                    throw Exception("invelid tag in <versions>, check format");
                }

                if(nodeAttributes->getNamedItem("md5")->innerText().size() != 32) {
                    throw Exception("<up> md5 attribute is invalid");
                }

                Version versionFor(nodeAttributes->getNamedItem("for")->innerText()),
                    versionTo(nodeAttributes->getNamedItem("to")->innerText());

                if(versionFor.compare(versionTo) >= 0) {
                    throw Exception("invalid updating version chain (version \"to\" less or equal to version \"for\")");
                }

                // append versions
                if(software.version().compare(versionFor) > 0) {
                    continue;
                }

                if(software.version().compare(versionFor) == 0) {
                    findedCurrentVersion = true;
                }

                versions.push_back(VersionInfo(
                    toLower(nodeAttributes->getNamedItem("md5")->innerText()),
                    versionFor,
                    versionTo
                ));

            }

            break;
        }

        if(!platformFound) {
            throw Exception("software platform not found in updating information!");
        }

        if(!findedCurrentVersion && versions.size() > 0) {
            throw Exception("not found current software version in updating information?");
        }

        if(versions.size() == 0) {
            return;
        }

        // make update chain, sort them...
        sort(versions.begin(), versions.end(), __versionPairCompare__);

        bool                brokenChain = false;

        VersType::iterator  it = versions.begin();
        Version             currentVersion = software.version();
        for(;it != versions.end(); currentVersion = it->to(), ++it) {

            if(currentVersion.compare(it->from())) {
                poco_warning_f3(Logger::get("VersionInfo"),
                    "broken chain versions (from lower to upper) iterator from=[%s] to=[%s], from != current=[%s]", 
                        it->from().toString(), it->to().toString(), currentVersion.toString());
                brokenChain = true;
                break;
            }

        }

        if(brokenChain) {
            versions.erase(it, versions.end());
        }

    }
    catch(Exception &e) {
        versions.clear();
        poco_error_f1(Logger::get("VersionInfo"),
            "fail request version information [%s]",
            e.displayText());

        e.rethrow();
    }

}

