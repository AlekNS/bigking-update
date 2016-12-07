
#ifndef PROCESS_UPDATE_H
#define	PROCESS_UPDATE_H


#include <string>


using std::string;


class Software;


//============================================================================//
class ProcessUpdate
{
public:

    static int run(Software &software);

private:
    static void notify(Software &software, const string &text, const bool &failed);

};


#endif
