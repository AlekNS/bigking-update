

#include <common/stdafx.h>


#include <Poco/File.h>
#include <Poco/URI.h>
#include <Poco/Delegate.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/StreamCopier.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Util/Application.h>
#include <Poco/Zip/Decompress.h>

#include <Poco/MD5Engine.h>

#include "Downloader.h"
#include "../common/Software.h"
#include "VersionInfo.h"


#include "../common/DecompressZipHandler.h"


using namespace std;
using namespace Poco;
using namespace Poco::Util;
using namespace Poco::Net;
using namespace Poco::Zip;


//============================================================================//
DownloaderHolder::DownloaderHolder(const string &name, const string &path) :
    _name(name),
    _path(path),
    _pathExtracted()
{
}

//============================================================================//
string DownloaderHolder::name() const
{
    return _name;
}
//============================================================================//
string DownloaderHolder::path() const
{
    return _path;
}

//============================================================================//
string DownloaderHolder::pathExtracted() const
{
    return _pathExtracted;
}

//============================================================================//
void DownloaderHolder::remove()
{
    try {
        File  file(_path);
        if(file.exists()) {
            file.remove(true);
        }
    }
    catch(...) {
        throw;
    }

    try {
        File  file(_pathExtracted);
        if(file.exists()) {
            file.remove(true);
        }
    }
    catch(...) {
        throw;
    }
}


string DownloaderHolder::extract()
{
    if(_pathExtracted.size() > 0) {
        poco_warning_f1(Logger::get("DownloaderHolder"), "already extracted [%s]", _path);
        return _pathExtracted;
    }

    try {

        std::ifstream inputStream(_path.c_str(), std::ios::binary);

        if(!inputStream.good()) {
            throw Exception("reading of package");
        }

        _pathExtracted = Path(Application::instance().config().getString("temporary")).makeDirectory().
            makeAbsolute().pushDirectory(UUIDGenerator().createRandom().toString()).toString();

        Decompress extracter(inputStream, _pathExtracted);

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
        remove();
        _pathExtracted.clear();
        poco_error_f1(Logger::get("DownloaderHolder"),
            "fail to extract package, %s", e.displayText());
        e.rethrow();
    }
    catch(...) {
        remove();
        _pathExtracted.clear();
    }


    return _pathExtracted;
}


//============================================================================//
DownloaderHolder Downloader::download(Software &software, const VersionInfo &version)
{
    string          filePath;
    string          packageName;

    packageName = format("%s_%s_%s_%s.up",
                           software.name(),
                           version.from().toString(),
                           version.to().toString(),
                           software.platform()
                        );

    Logger      &log = Logger::get("Downloader");

    try {

        ofstream        outputStream;

        filePath = Path(Application::instance().config().getString("temporary")).
            makeDirectory().makeAbsolute().setFileName(packageName).toString();

        outputStream.open(filePath.c_str(),
            ios::binary|ios::out);

        if(outputStream.fail()) {
            throw Exception("writing downloaded file denied, check permissions");
        }


        // download software update package
        string      uriPath = format("%s/%d.%d/%s",
                            software.config().getString("uri"),
                            (int)version.to().major(),
                            (int)version.to().minor(),
                            packageName
                        );

        if(software.tag().size() > 0) {
            uriPath = format("%s/%d.%d-%s/%s",
                software.config().getString("uri"),
                (int)version.to().major(),
                (int)version.to().minor(),
                software.tag(),
                packageName
            );
        }

        URI         uri(uriPath);

        std::string         path(uri.getPathAndQuery());

        HTTPClientSession   serverSession(uri.getHost(), uri.getPort());
        HTTPRequest         serverRequest(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        HTTPResponse        serverResponse;

        poco_information_f1(log, "request to the server... [%s]", path);

        serverSession.sendRequest(serverRequest);

        std::istream& receivedStream = serverSession.receiveResponse(serverResponse);
        if(HTTPResponse::HTTP_OK != serverResponse.getStatus()) {
            throw Exception(format("http request failed, error = %d",
                (int)serverResponse.getStatus()));
        }

        poco_information(log, "downloading package from the server...");


        // copy response data to file
        StreamCopier::copyStream(receivedStream, outputStream);
        outputStream.close();


        // Check md5
        MD5Engine    md5sum;

        const int   bufferSize = 1<<12;
        char        buffer[bufferSize];

        ifstream   inputStream(filePath.c_str(),
            ios::binary|ios::out);

        if(!inputStream.good()) {
            throw Exception("reading downloaded package is denied");
        }

        try {
            while(!inputStream.eof()) {
                inputStream.read(buffer, bufferSize);
                if(inputStream.gcount() > 0) {
                    md5sum.update(buffer, inputStream.gcount());
                }
            }
        }
        catch(Exception &e) {
            inputStream.close();
            e.rethrow();
        }
        inputStream.close();

        if(version.md5().compare(DigestEngine::digestToHex(md5sum.digest()))) {
            throw Exception("package is damaged, wrong md5 sum");
        }

    }
    catch(Exception &e) {
        poco_error_f1(log,
            "fail to download packages from the server, %s", e.displayText());
        e.rethrow();
    }
    catch(...) {
        throw;
    }

    return DownloaderHolder(version.to().toString(), filePath);
}
