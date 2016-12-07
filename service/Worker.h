
#ifndef WORKER_H
#define WORKER_H


#include <list>
#include <string>


#include <Poco/Runnable.h>
#include <Poco/Thread.h>


#include "../common/Software.h"


using std::list;
using std::string;


using Poco::Runnable;
using Poco::Thread;


//============================================================================//
class Worker : public Runnable
{
public:

    Worker(const string &softwareName, const bool &processTimeSchedules = true);
    void run();

    void start();
    void stop();
    void waitUntilStop();
    void wait();

private:

    bool                        _terminating,
                                _workerStarting,
                                _processTimeSchedules;

    Software                    _software;
    Thread                      _thread;

};


#endif
