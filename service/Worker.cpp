
#include <common/stdafx.h>


#include <set>
#include <sstream>


#include <Poco/LocalDateTime.h>
#include <Poco/Logger.h>
#include <Poco/Pipe.h>
#include <Poco/File.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/Process.h>
#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Thread.h>
#include <Poco/Util/Application.h>
#include <Poco/URI.h>
#include <Poco/StreamCopier.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>


#include "Worker.h"


#include "../common/Software.h"
#include "../software/VersionInfo.h"



using namespace std;
using namespace Poco;
using namespace Poco::Util;
using namespace Poco::Net;


//============================================================================//
Worker::Worker(const string &softwareName, const bool &_processTimeSchedules) :
    _terminating(false),
    _software(softwareName),
    _thread(softwareName),
    _workerStarting(false),
    _processTimeSchedules(_processTimeSchedules)
{
}

//============================================================================//
void Worker::start()
{
    _thread.start(*this);
}

//============================================================================//
static FastMutex    __workerMutex__;

void Worker::stop()
{
    ScopedLock<FastMutex>  lock(__workerMutex__);
    _terminating = true;
}

void Worker::waitUntilStop()
{
    if(_terminating) {
        wait();
    }
}

void Worker::wait()
{
    if(_thread.isRunning()) {
        _thread.join();
        _terminating = false;
    }
}

//============================================================================//
struct __schedule__
{
    int         day_of_week;
    int         day;
    int         hour;
    int         minute;
    bool        isMatch;

    __schedule__() : isMatch(false) { }

    bool operator<(const __schedule__& s) const
    {
        return day_of_week < s.day_of_week &&
            day < s.day &&
            hour < s.hour &&
            minute < s.minute;
    }

    bool check(LocalDateTime &stamp)
    {
        if(day_of_week > -1 && day_of_week != stamp.dayOfWeek()) {
            isMatch = false;
            return false;
        }

        if(day > -1 && day != stamp.day()) {
            isMatch = false;
            return false;
        }
        if(hour > -1 && hour != stamp.hour()) {
            isMatch = false;
            return false;
        }
        if(minute > -1 && minute != stamp.minute()) {
            isMatch = false;
            return false;
        }

        if(isMatch)
            return false;

        isMatch = true;

        return true;
    }

    static void parse(const string &text, int minValue, int maxValue, set<int> &items)
    {
        StringTokenizer     partParser(text, ";, ", StringTokenizer::TOK_IGNORE_EMPTY |
                                                    StringTokenizer::TOK_TRIM);
        StringTokenizer::Iterator partToken = partParser.begin();

        for(; partToken != partParser.end(); ++partToken) {

            StringTokenizer     numberParser(*partToken, "/", StringTokenizer::TOK_IGNORE_EMPTY |
                                                    StringTokenizer::TOK_TRIM);

            if(numberParser.count() == 2 && !numberParser[0].compare("*")) {

                int parsedNumber = -1;

                if(!NumberParser::tryParse(numberParser[1], parsedNumber) ||
                    parsedNumber < minValue || parsedNumber > maxValue || parsedNumber == 0)
                    continue;

                for(int i = minValue; i <= maxValue; i++) {
                    if((i % parsedNumber) != 0) continue;
                    items.insert(i);
                }

                continue;
            }

            if(numberParser.count() == 1) {

                int parsedNumber = -1;

                if(NumberParser::tryParse(numberParser[0], parsedNumber)) {
                    if(parsedNumber >= minValue && parsedNumber <= maxValue)
                        items.insert(parsedNumber);
                }

            }

        }

    }
};


void Worker::run()
{
    _workerStarting = true;

    bool             noConsole = Application::instance().config().has("args.disable_console");
    Logger          &log = Logger::get("Worker");

    poco_information(log, "worker started...");

    vector<__schedule__>            schedules;

    if(_processTimeSchedules) {
        AbstractConfiguration::Keys     keys;

        _software.config().keys("schedules", keys);
        AbstractConfiguration::Keys::iterator keysIt = keys.begin();

        poco_information_f1(log, "parse schedules %d...", (int)keys.size());
        for(;keysIt != keys.end();++keysIt) {

            if(!keysIt->compare("start")) {
                continue;
            }

            try {

                set<int>        days, hours, minutes;

                __schedule__::parse(_software.config().getString("schedules." + *keysIt + ".day", ""), 1, 365, days);
                __schedule__::parse(_software.config().getString("schedules." + *keysIt + ".hour", ""), 0, 23, hours);
                __schedule__::parse(_software.config().getString("schedules." + *keysIt + ".minute", ""), 0, 59, minutes);

                days.insert(-1);
                hours.insert(-1);
                minutes.insert(-1);

                for(set<int>::iterator day = days.begin(); day != days.end(); ++day) {
                    if(days.size() > 1 && *day == -1) continue;
                for(set<int>::iterator hour = hours.begin(); hour != hours.end(); ++hour) {
                    if(hours.size() > 1 && *hour == -1) continue;
                for(set<int>::iterator minute = minutes.begin(); minute != minutes.end(); ++minute) {
                    if(minutes.size() > 1 && *minute == -1) continue;

                    __schedule__    s;

                    s.day_of_week   = _software.config().getInt("schedules." + *keysIt + ".day_of_week", -1);

                    s.day           = *day;
                    s.hour          = *hour;
                    s.minute        = *minute;

                    if(s.day_of_week < 0 && s.day < 0 && s.hour < 0 && s.minute < 0) {
                        poco_warning(log, "wrong schedule format for " + *keysIt);
                        continue;
                    }

                    schedules.push_back(s);
                }}}

            }
            catch(...) {
            }

        }


        if(_software.config().getString("schedules.start", "off").compare("on") && !schedules.size()) {
            poco_warning(log, "empty schedules");
            return;
        }

        poco_information_f1(log, "schedules %d, wait events...", (int)schedules.size());
    }

    poco_information_f1(log, "goto loop...", (int)schedules.size());
    while(!_terminating) {

        try {
            Thread::sleep(1500 + rand()%500);

            bool        checkUpdating = false;

            if(_workerStarting) {
                _workerStarting = false;
                if(!_software.config().getString("schedules.start", "off").compare("on") || !_processTimeSchedules) {
                    checkUpdating = true;
                }
            }

            LocalDateTime stamp;

            vector<__schedule__>::iterator     currentSchedule = schedules.begin();

            while(currentSchedule != schedules.end()) {

                if(currentSchedule->check(stamp)) {
                    checkUpdating = true;
                    break;
                }
                ++currentSchedule;
            }

            if(checkUpdating) {

#ifdef BUILD_UPDATE_SERVICE
                poco_information(log,
                    "schedule event! self updating process started..."
                );
#else
                poco_information(log,
                    "schedule event! updating process started..."
                );
#endif
                checkUpdating = false;
                if(schedules.size() == 0) {
                    poco_information(log, "event is only on start update, after work it will be stopped");
                    _terminating = true;
                }

                //
                // Call update application
                //
                string      servicePath = Application::instance().config().getString("application.dir", "") + Path::separator();
                string      prefix =
#if defined(POCO_OS_FAMILY_WINDOWS)
                                ".exe"
#elif defined(POCO_OS_FAMILY_UNIX)
                                ""
#endif
                ;

                string      updateSoftAppPath       = servicePath + "BigKingUpdateSoftware"  + prefix;
                string      updatePackAppPath       = servicePath + "BigKingUpdatePackage" + prefix;

#if defined(BUILD_UPDATE_SERVICE)
                    updateSoftAppPath = servicePath + UUIDGenerator().createRandom().toString() + prefix;
                    updatePackAppPath = servicePath + UUIDGenerator().createRandom().toString() + prefix;

                    try {
                        File(servicePath + "BigKingUpdatePackage" + prefix).
                            copyTo(updatePackAppPath);
                    }
                    catch(...) { }

                    try {
                        File(servicePath + "BigKingUpdateSoftware" + prefix).
                            copyTo(updateSoftAppPath);
                    }
                    catch(...) { }
#endif

                int executeResult = 0;

                try {

                    Pipe        outPipe,
                                errPipe;

                    Process::Args       args;
                    Process::Env        env;

                    if(noConsole) {
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
                        _software.name()
                    );
//#endif


                    args.push_back(
#if defined(POCO_OS_FAMILY_WINDOWS)
                        format("/packageapp \"%s", updatePackAppPath)
#else
                        "--packageapp"
#endif
                    );

#if defined(POCO_OS_FAMILY_WINDOWS)
#else
                    args.push_back(
                        updatePackAppPath
                    );
#endif

                    poco_information(log, "execute update application...");

//                    cout << updateSoftAppPath << endl;

                    ProcessHandle       handle =
                        Process::launch(
                            updateSoftAppPath,
                            args,
                            Application::instance().config().getString("application.dir"),
                            NULL,
                            NULL, //&outPipe,
                            NULL, //&errPipe,
                            env
                        );

                    executeResult = handle.wait();

                    if(executeResult < 2) {
                        poco_information(log, "result of execution success!");
                    }
                    else {
                        poco_error_f1(log, "result of execution failed, exit code %d",
                            executeResult);
                    }

                }
                catch(Exception &e) {
                    poco_error_f1(log, "exception when calling update application, %s",
                        e.displayText());
                }

#if defined(BUILD_UPDATE_SERVICE)

                    // delete old update apps
                    try {
                        File(updatePackAppPath).remove();
                    }
                    catch(...) { }
                    try {
                        File(updateSoftAppPath).remove();
                    }
                    catch(...) { }

                    if(executeResult == 1) {

                        // restart daemon/service
                        poco_information(log, "self service update complete!");
                    }

#endif
            }
        }
        catch(...) {
            poco_error(log, "catched unknown exception, stop Worker... ");
            _terminating = true;
        }
    }

    poco_information(log, "worker stopped");
}
