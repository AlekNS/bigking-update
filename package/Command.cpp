
#include <common/stdafx.h>

#include <Poco/Logger.h>
#include <Poco/Path.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/File.h>
#include <Poco/String.h>
#include <Poco/Process.h>
#include <Poco/Util/Application.h>

#include <python2.7/Python.h>


#include "Command.h"
#include "Package.h"


extern "C" {

    #include "xdelta/xdelta3.h"

}


using namespace std;
using namespace Poco;
using namespace Poco::Util;


static void inheritFileAttributes(const string &dst, const string &src) {
    try {

        File destinationFile(dst);
        if(!destinationFile.exists()) return;

        File sourceFile(src);
        if(!sourceFile.exists()) return;

        destinationFile.
                setExecutable(sourceFile.canExecute());

    }
    catch(...) {
    }
}


//============================================================================//
Command::Command() :
    _osType(OS_UNKNOWN),
    _skipError(false),
    _executed(false)
{
}

//============================================================================//
Command::~Command()
{
}

//============================================================================//
bool Command::skipError() const
{
    return _skipError;
}

//============================================================================//
void Command::skipError(const bool &enabled)
{
    _skipError = enabled;
}

//============================================================================//
int Command::osType() const
{
    return _osType;
}

//============================================================================//
void Command::osType(const string &type)
{
    if(!type.compare("windows"))
        _osType = OS_WINDOWS;
    else if(!type.compare("unix"))
        _osType = OS_UNIX;
    else
        throw Exception("unknown os_type tag: " + type);
}

//============================================================================//
void Command::convertPathSeparator(string &str)
{
#if defined(POCO_OS_FAMILY_WINDOWS)
    replaceInPlace(str, string("/"), string("\\"));
#elif defined(POCO_OS_FAMILY_UNIX)
    replaceInPlace(str, string("\\"), string("/"));
#else
#error OS platform not implemented yet
#endif
}




//============================================================================//
CommandCopy::CommandCopy(const string &filePath, bool isOverwritable) :
    _overwritable(isOverwritable),
    _filePath(filePath)
{
    convertPathSeparator(_filePath);
}


//============================================================================//
const char *CommandCopy::name() const
{
    return "copy";
}

//============================================================================//
bool CommandCopy::execute(Package &package)
{

    try {
        File  file(package.path().setFileName(_filePath));

        if(!file.exists() || _overwritable) {

            File   destFile(package.software().path() + _filePath);

            if(destFile.exists() && _overwritable) {
                _filePathTemporary = package.software().path() + _filePath + "." +
                    UUIDGenerator().createRandom().toString();

                destFile.moveTo(_filePathTemporary);
            }

            file.copyTo(package.software().path() + _filePath);

            _executed = true;
        }
        else
            throw Exception("file exists: " + _filePath);
    }
    catch(Exception &e) {

        if(_skipError) {
            poco_warning_f1(Logger::get("CommandCopy"),
                "[%s]", e.displayText());
        }
        else {
            poco_error_f1(Logger::get("CommandCopy"),
                "Execution has failed [%s]", e.displayText());
        }

        return false;
    }

    return true;
}

//============================================================================//
bool CommandCopy::undo(Package &package)
{

    try {
        File(package.software().path() + _filePath).remove(true);

        if(_filePathTemporary.size()) {
            File(_filePathTemporary).moveTo(package.software().path() + _filePath);
        }
    }
    catch(...) {
    }

    return true;
}

//============================================================================//
bool CommandCopy::clean(Package &package)
{
    if(!_executed) return true;
    try {
        File(_filePathTemporary).remove(true);
    }
    catch(...) {
    }

    return true;
}



//============================================================================//
CommandRename::CommandRename(const string &filePathFrom, const string &filePathTo) :
    _filePathFrom(filePathFrom),
    _filePathTo(filePathTo)
{
    convertPathSeparator(_filePathFrom);
    convertPathSeparator(_filePathTo);
}

//============================================================================//
const char *CommandRename::name() const
{
    return "rename";
}

//============================================================================//
bool CommandRename::execute(Package &package)
{
    if(!_filePathFrom.compare(_filePathTo)) {
        return true;
    }

    try {
        File  fileFrom(package.software().path() + _filePathFrom);
        File  fileTo(package.software().path() + _filePathTo);

        if(!fileFrom.exists())
            throw Exception("file doesn't exist: " + fileFrom.path());
        if(fileTo.exists())
            throw Exception("file exists: " + fileTo.path());

        fileFrom.renameTo(fileTo.path());

        _executed = true;
    }
    catch(Exception &e) {

        if(_skipError) {
            poco_warning_f1(Logger::get("CommandRename"),
                "[%s]", e.displayText());
        }
        else {
            poco_error_f1(Logger::get("CommandRename"),
                "Execution has failed [%s]", e.displayText());
        }

        return false;
    }

    return true;
}

//============================================================================//
bool CommandRename::undo(Package &package)
{
    try {
        File     file(package.software().path() + _filePathTo);

        file.renameTo(package.software().path() + _filePathFrom);
    }
    catch(...) {
    }

    return true;
}





//============================================================================//
CommandMove::CommandMove(const string &filePathFrom, const string &filePathTo) :
    _filePathFrom(filePathFrom),
    _filePathTo(filePathTo)
{
    convertPathSeparator(_filePathFrom);
    convertPathSeparator(_filePathTo);
}


//============================================================================//
const char *CommandMove::name() const
{
    return "move";
}

//============================================================================//
bool CommandMove::execute(Package &package)
{
    if(!_filePathFrom.compare(_filePathTo)) {
        return true;
    }

    try {
        File  fileFrom(package.software().path() + _filePathFrom);
        File  fileTo(package.software().path() + _filePathTo);

        if(!fileFrom.exists())
            throw Exception("file doesn't exist: " + fileFrom.path());

        if(fileTo.exists())
            throw Exception("file exists: " + fileTo.path());

        fileFrom.moveTo(fileTo.path());
        _executed = true;
    }
    catch(Exception &e) {

        if(_skipError) {
            poco_warning_f1(Logger::get("CommandMove"),
                "[%s]", e.displayText());
        }
        else {
            poco_error_f1(Logger::get("CommandMove"),
                "Execution has failed [%s]", e.displayText());
        }

        return false;
    }

    return true;
}

//============================================================================//
bool CommandMove::undo(Package &package)
{
    try {
        File   file(package.software().path() + _filePathTo);

        file.moveTo(package.software().path() + _filePathFrom);
    }
    catch(...) {
    }

    return true;
}







//============================================================================//
CommandDelete::CommandDelete(const string &filePath) :
    _filePath(filePath)
{
    convertPathSeparator(_filePath);
}


//============================================================================//
const char *CommandDelete::name() const
{
    return "delete";
}

//============================================================================//
bool CommandDelete::execute(Package &package)
{
    try {
        File file(package.software().path() + _filePath);

        if(!file.exists()) {
            throw Exception("file doesn't exist: " + file.path());
        }

        _filePathTemporary = package.software().path() + _filePath + "." +
            UUIDGenerator().createRandom().toString();

        file.moveTo(_filePathTemporary);

        _executed = true;
    }
    catch(Exception &e) {

        if(_skipError) {
            poco_warning_f1(Logger::get("CommandDelete"),
                "[%s]", e.displayText());
        }
        else {
            poco_error_f1(Logger::get("CommandDelete"),
                "Execution has failed [%s]", e.displayText());
        }

        return false;
    }

    return true;
}

//============================================================================//
bool CommandDelete::undo(Package &package)
{
    try {
        File   file(_filePathTemporary);
        file.moveTo(package.software().path() + _filePath);
    }
    catch(...) {
    }

    return true;
}

//============================================================================//
bool CommandDelete::clean(Package &package)
{
    if(!_executed && _filePathTemporary.size() > 0) return true;
    try {
        File file(_filePathTemporary);
        file.remove(true);
    }
    catch(...) {
    }
    return true;
}






//============================================================================//
CommandMakeFolder::CommandMakeFolder(const string &filePath) :
    _filePath(filePath)
{
    convertPathSeparator(_filePath);
}


//============================================================================//
const char *CommandMakeFolder::name() const
{
    return "make_folder";
}

//============================================================================//
bool CommandMakeFolder::execute(Package &package)
{
    try {
        File  file(package.software().path() + _filePath);
        if(file.exists()) {
            throw Exception("folder exists: " + file.path());
        }

        file.createDirectories();
        _executed = true;
    }
    catch(Exception &e) {

        if(_skipError) {
            poco_warning_f1(Logger::get("CommandMakeFolder"),
                "[%s]", e.displayText());
        }
        else {
            poco_error_f1(Logger::get("CommandMakeFolder"),
                "Execution has failed [%s]", e.displayText());
        }

        return false;
    }

    return true;
}

//============================================================================//
bool CommandMakeFolder::undo(Package &package)
{
    try {
        if(_executed) {
            File(package.software().path() + _filePath).remove(true);
        }
    }
    catch(...) {
    }

    return true;
}




//============================================================================//
CommandShell::CommandShell(const string &shellCommands) :
    _shellCommands(shellCommands)
{
}


//============================================================================//
const char *CommandShell::name() const
{
    return "shell";
}

//============================================================================//
bool CommandShell::execute(Package &package)
{
    _executed = true;
#if defined(POCO_OS_FAMILY_WINDOWS)
    return SetCurrentDirectoryA((package.software().path().c_str())) &&
        system(_shellCommands.c_str()) == 0;
#elif defined(POCO_OS_FAMILY_UNIX)
    return chdir(package.software().path().c_str()) == 0 &&
        system(_shellCommands.c_str()) == 0;
#else
#error OS platform not implemented yet
#endif
    return false;
}





//============================================================================//
CommandScript::CommandScript(const string &filePath) :
    _filePath(filePath)
{
    convertPathSeparator(_filePath);
}


//============================================================================//
const char *CommandScript::name() const
{
    return "script";
}

//============================================================================//
bool CommandScript::execute(Package &package)
{
    ifstream input;

    try {
        stringstream program;
        string       sourceScriptFilePath = package.path().setFileName(_filePath).toString();

        input.open(sourceScriptFilePath.c_str(), ifstream::in);

        if(input.fail()) {
            throw Exception("file doesn't exist or access denied to file: " + _filePath);
        }

        copy(istreambuf_iterator<char>(input),
             istreambuf_iterator<char>(),
             ostreambuf_iterator<char>(program));

        input.close();

        string arg2 = Application::instance().config().getString("application.dir").c_str(),
               arg3 = package.software().path(),
               arg4 = package.path().toString()
        ;

        char *args[] = {
            const_cast<char*>(package.software().name().c_str()),
            const_cast<char*>(arg2.c_str()),
            const_cast<char*>(arg3.c_str()),
            const_cast<char*>(arg4.c_str()),
        };

        string base = Application::instance().config().getString("application.dir") + string("python");
        Py_SetProgramName(const_cast<char*>(Application::instance().config().getString("application.path").c_str()));

#if defined(POCO_OS_FAMILY_WINDOWS)
        Py_SetPythonHome(const_cast<char*>(base.c_str()));
#elif defined(POCO_OS_FAMILY_UNIX)
#else
#error OS platform not implemented yet
#endif

        Py_Initialize();


        PySys_SetArgv(4, args);

        _executed = true;

#if defined(POCO_OS_FAMILY_WINDOWS)
        PySys_SetPath(const_cast<char*>(base.c_str()));

        if(!SetCurrentDirectoryA(package.software().path().c_str())) {
#elif defined(POCO_OS_FAMILY_UNIX)
        if(chdir(package.software().path().c_str()) != 0) {
#else
#error OS platform not implemented yet
#endif
            Py_Finalize();
            throw Exception("change working directory into unpacked package is failed");
        }

        ///
        /// PyRun_File
        ///
        static const char _separator[2] = {Path::separator(), 0};
        string separator = _separator;
        string programText =
            string("import sys; sys.path.append('").append(package.path().toString() + "')\n").
            append("sys.path.append('").append(base+separator+"imports.zip')\n").
            append("sys.path.append('").append(base+separator+"')\n");


        replaceInPlace(programText, "\\", "\\\\");
        programText.append(program.str());


        PyObject        *globals = PyModule_GetDict(
            PyImport_AddModule(const_cast<char*>("__main__")));
        Py_INCREF(globals);


        PyObject        *resultExecution =
            PyRun_String(programText.c_str(), Py_file_input, globals, globals);

        if(!resultExecution) {

            if(PyErr_ExceptionMatches(PyExc_SystemExit)) {
                PyObject  *type, *value, *trace;
                PyErr_Fetch(&type, &value, &trace);

                int exitValue = PyInt_AsLong(value);
                if(exitValue != 0) {
                    Py_Finalize();
                    throw Exception(format("python script done with error [%d]", exitValue));
                }
            }
            else if(PyErr_Occurred()) {
                PyErr_PrintEx(0);
                Py_Finalize();
                throw Exception("uncatched python script exception");
            }
        }
        Py_DECREF(globals);

        Py_Finalize();

        return true;
    }
    catch(Exception &e) {

        if(_skipError) {
            poco_warning_f1(Logger::get("CommandSciprt"),
                "[%s]", e.displayText());
        }
        else {
            poco_error_f1(Logger::get("CommandScript"),
                "Execution has failed [%s]", e.displayText());
        }

        return false;
    }

    input.close();

    return false;
}






//============================================================================//
CommandPatch::CommandPatch(const string &filePath) :
    _filePath(filePath)
{
    convertPathSeparator(_filePath);
}


//============================================================================//
const char *CommandPatch::name() const
{
    return "patch";
}

//============================================================================//
bool CommandPatch::execute(Package &package)
{
    //
    // XDelta
    //
    xd3_stream 	stream;
    xd3_config 	config;
    xd3_source 	source;

    char* 	inputBuffer;
    int 	bufferSize = bufferSize < XD3_ALLOCSIZE ? XD3_ALLOCSIZE : 0x1000, 
                inputBufferSize, returnResult, chunkType;

    memset(&stream, 0, sizeof (stream));
    memset(&source, 0, sizeof (source));

    xd3_init_config(&config, XD3_ADLER32);
    config.winsize = bufferSize;
    xd3_config_stream(&stream, &config);

    //
    // Common
    //
    string      srcFileName = package.software().path() + _filePath;

    ifstream    srcStream, inStream;
    ofstream    outStream;
    bool        resultStatus = false;

    try {
        //
        // Prepare files
        //
        _filePathTemporary = package.software().path() + _filePath + "." +
            UUIDGenerator().createRandom().toString();
        File  inDeltaFile(package.path().setFileName(_filePath).toString() + ".delta");
        File  destFile(srcFileName);

        if(!inDeltaFile.exists()) {
            throw Exception("source delta file doesn't exists " + inDeltaFile.path());
        }

        if(!destFile.exists()) {
            throw Exception("destination file doesn't exists");
        }

        destFile.renameTo(_filePathTemporary);

        srcStream.open(_filePathTemporary.c_str());
        inStream.open(inDeltaFile.path().c_str());
        outStream.open(srcFileName.c_str());

        //
        // Setup XDelta
        //
        source.blksize  = bufferSize;
        source.curblk   = (uint8_t*)new char[source.blksize];

        srcStream.read((char*)source.curblk, source.blksize);

        source.onblk    = srcStream.gcount();
        source.curblkno = 0;

        inStream.seekg(0, ios::beg);

        xd3_set_source(&stream, &source);

        inputBuffer = new char[bufferSize];

        do {
            inStream.read(inputBuffer, bufferSize);
            inputBufferSize = inStream.gcount();

            if (inputBufferSize < bufferSize) {
                xd3_set_flags(&stream, XD3_FLUSH | stream.flags);
            }

            xd3_avail_input(&stream, (uint8_t*)inputBuffer, inputBufferSize);

process:
            chunkType = xd3_decode_input(&stream);

            switch (chunkType)
            {
                case XD3_INPUT:
                    continue;

                case XD3_OUTPUT:
                    outStream.write((char*)stream.next_out, stream.avail_out);
                    xd3_consume_output(&stream);
                    goto process;

                case XD3_GETSRCBLK:
                    srcStream.seekg(source.blksize * source.getblkno, ios::beg);
                    srcStream.read((char*)source.curblk, source.blksize);
                    source.onblk = srcStream.gcount();
                    source.curblkno = source.getblkno;
                    goto process;

                case XD3_GOTHEADER:
                    goto process;

                case XD3_WINSTART:
                    goto process;

                case XD3_WINFINISH:
                    goto process;

                default:
                    throw Exception(format("!!! INVALID %s %d !!! Damaged!?", stream.msg, chunkType));
            }
        } while (inputBufferSize == bufferSize);

        resultStatus = true;
    }
    catch(Exception &e) {
        if(_skipError) {
            poco_warning_f1(Logger::get("CommandPatch"),
                "[%s]", e.displayText());
        }
        else {
            poco_error_f1(Logger::get("CommandPatch"),
                "Patching was failed [%s]", e.displayText());
        }
    }

    //
    // Free resources
    //
    if(inputBuffer)     delete inputBuffer;
    if(source.curblk)   delete source.curblk;

    xd3_close_stream(&stream);
    xd3_free_stream(&stream);

    inStream.close();
    outStream.close();
    srcStream.close();

    inheritFileAttributes(srcFileName, _filePathTemporary);

    _executed = resultStatus;

    return resultStatus;
}

//============================================================================//
bool CommandPatch::undo(Package &package)
{
    try {
        File   file(_filePathTemporary);
        file.renameTo(package.software().path() + _filePath);
    }
    catch(...) {
    }

    return true;
}

//============================================================================//
bool CommandPatch::clean(Package &package)
{
    if(!_executed) return true;
    try {
        File(_filePathTemporary).remove();
    }
    catch(...) {
    }
    return true;
}
