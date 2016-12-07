
#include <common/stdafx.h>


#include <sstream>
#include <iostream>


#include <Poco/File.h>
#include <Poco/LocalDateTime.h>
#include <Poco/Logger.h>
#include <Poco/Pipe.h>
#include <Poco/Process.h>
#include <Poco/StreamCopier.h>
#include <Poco/Thread.h>
#include <Poco/URI.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>


#include "ProcessUpdate.h"
#include "../common/Software.h"
#include "VersionInfo.h"
#include "Downloader.h"


using namespace std;


using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;


//============================================================================//
void ProcessUpdate::notify(Software &software, const string &text, const bool &failed)
{

    if(!software.config().has("notify"))
        return;

    string status = failed ? "failed" : "success";

    AbstractConfiguration      &config = Application::instance().config();

    try {
        URI    uri(software.config().getString("notify") +

                    string("?id="           + config.getString("identifier", "unknown")) +
                    string("&name="         + software.name()) +
                    string("&message=")     + text +
                    string("&status=")      + status

               );

        // send result to the server
        std::string         path(uri.getPathAndQuery());

        HTTPClientSession   serverSession(uri.getHost(), uri.getPort());
        HTTPRequest         serverRequest(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        HTTPResponse        serverResponse;

        poco_information(Logger::get("ProcessUpdate"), "notify the server...");

        serverSession.sendRequest(serverRequest);

        std::istream        &receivedStream = serverSession.receiveResponse(serverResponse);
        if(HTTPResponse::HTTP_OK != serverResponse.getStatus()) {
            throw Exception(format("http request failed, error = %d",
                (int)serverResponse.getStatus()));
        }

        // fill out
        stringstream  outputDummyStream;
        StreamCopier::copyStream(receivedStream, outputDummyStream);

        poco_information_f1(Logger::get("ProcessUpdate"), "notified with [%s]", outputDummyStream.str());

    }
    catch(Exception &e) {
        poco_error_f1(Logger::get("ProcessUpdate"), "can't notify about update process [%s]", e.displayText());
    }
    catch(exception &e) {
        poco_error_f1(Logger::get("ProcessUpdate"), "can't notify about update process [%s]", string(e.what()));
    }
}

//============================================================================//
int ProcessUpdate::run(Software &software)
{

    bool isSoftwareUpdated = false;

    AbstractConfiguration &config = Application::instance().config();

    Logger          &log = Logger::get("ProcessUpdate");

    try {
        poco_information(log, "request server for new versions...");

        VersionInfo::VersType  versions;
        VersionInfo::request(software, versions);

        if(versions.size() == 0) {
            poco_information(log, "nothing to update");
            return Application::EXIT_OK;
        }

        VersionInfo::VersType::iterator    versionIterator;
        versionIterator = versions.begin();
        for(;versionIterator != versions.end(); ++versionIterator) {
            poco_information_f1(log, "new version founded [%s]",
                versionIterator->to().toString());
        }

        vector<DownloaderHolder>            holders;

        poco_information_f1(log, "download new updating packages [%d]...",
            (int)versions.size());

        versionIterator = versions.begin();
        for(;versionIterator != versions.end(); ++versionIterator) {

            try {
                poco_information_f1(log, "download new package [%s]...",
                    versionIterator->to().toString());
                holders.push_back(Downloader::download(software, *versionIterator));
            }
            catch(...) {
                break;
            }
        }

        poco_information(log, "extract downloaded updating packages...");

        vector<DownloaderHolder>::iterator holderIterator;
        holderIterator = holders.begin();
        for(;holderIterator != holders.end(); ++holderIterator) {
            try {
                poco_information_f1(log, "extract [%s]",
                    holderIterator->path());
                holderIterator->extract();
            }
            catch(...) {
                poco_warning_f2(log, "extraction failed for [%s]/[%s]",
                    holderIterator->path(), holderIterator->pathExtracted());
                break;
            }
        }

        //
        // Call update application
        //
        poco_information(log, "run updating application...");

        int executeResult = 3;
        holderIterator = holders.begin();
        for(;holderIterator != holders.end(); ++holderIterator) {

            if(holderIterator->pathExtracted().size() == 0) {
                poco_error_f1(log, "can't run update, not extracted package for [%s]",
                    holderIterator->path());
                break;
            }

            try {

                Pipe
                            outPipe,
                            errPipe;

                try {
                    Process::Args       args;
                    Process::Env        env;

                    if(config.has("args.disable_console")) {
                        args.push_back(
#if defined(POCO_OS_FAMILY_WINDOWS)
                            "/"
#else
                            "--"
#endif
                            "noconsole");
                    }


                    args.push_back(
#if defined(POCO_OS_FAMILY_WINDOWS)
                        "/software"
#else
                        "--software"
#endif
                    );

//#if defined(POCO_OS_FAMILY_WINDOWS)
//#else
                    args.push_back(
                        software.name()
                    );
//#endif


                    args.push_back(
#if defined(POCO_OS_FAMILY_WINDOWS)
                        format("/path \"%s", holderIterator->pathExtracted())
#else
                        "--path"
#endif
                    );

#if defined(POCO_OS_FAMILY_WINDOWS)
#else
                    args.push_back(
                        holderIterator->pathExtracted()
                    );
#endif

                    poco_information_f1(log, "updating software by [%s]",
                        holderIterator->path());

                    ProcessHandle       handle =
                        Process::launch(
                            config.getString("args.package_application").c_str(),
                            args,
                            Application::instance().config().getString("application.dir"),
                            NULL,
                            &outPipe,
                            &errPipe,
                            env
                        );

                    executeResult = handle.wait();

                    //
                    // Write to log all output information
                    //
                    {
                        const int bufferSize = 512, stringSize = 53;
                        char charBuffer[bufferSize] = {0};
                        string resultString;
                        resultString.reserve(4096);
                        int readSize;

                        resultString = "update application :\n***********STDOUT**************\n";
                        readSize = 0;
                        while((readSize = outPipe.readBytes(charBuffer, bufferSize-1)) > 0) {
                            charBuffer[readSize] = 0;
                            resultString.append(charBuffer);
                        }
                        if(resultString.size() > stringSize) {
                            resultString.append("*******************************");
                            poco_information(log, resultString);
                        }

                        resultString = "update application :\n***********STDERR**************\n";
                        readSize = 0;
                        while((readSize = errPipe.readBytes(charBuffer, bufferSize-1)) > 0) {
                            charBuffer[readSize] = 0;
                            resultString.append(charBuffer);
                        }
                        if(resultString.size() > stringSize) {
                            resultString.append("*******************************");
                            poco_information(log, resultString);
                        }
                    }

                    //
                    // Check reuslt
                    //
                    if(executeResult) {
                        poco_information_f2(log, "update application failed for [%s]/[%d]",
                            holderIterator->path(), executeResult);
                        // notify
                        notify(software, holderIterator->name(), true);
                        break;

                    }
                    else {
                        poco_information_f1(log, "update application complete successful for [%s]",
                            holderIterator->path());
                        // notify
                        isSoftwareUpdated = true;
                        notify(software, holderIterator->name(), false);
                    }
                }
                catch(...) {
                }
            }
            catch(...) {
            }

        }

        poco_information(log, "remove temporary download and extracted files...");

        holderIterator = holders.begin();
        while(holderIterator != holders.end()) {
            try {
                holderIterator->remove();
            }
            catch(...) {
                poco_warning_f2(log, "can't remove downloaded or extracted files [%s]/[%s]",
                    holderIterator->path(), holderIterator->pathExtracted());
            }
            ++holderIterator;
        }

        poco_information(log, "updating complete");
        return isSoftwareUpdated ? 1 : 0;
    }
    catch(Exception &e) {
        poco_error_f1(log, "updating process failed, %s",
            e.displayText());
        return 3;
    }
    catch(exception &e) {
        poco_error_f1(log, "updating process failed, %s",
            string(e.what()));
        return 3;
    }

    return 3;
}

