#ifndef DECOMPRESS_ZIP_HANDLER_H
#define DECOMPRESS_ZIP_HANDLER_H


#include <Poco/Zip/Decompress.h>
#include <Poco/Logger.h>
#include <Poco/Delegate.h>


using namespace Poco::Zip;


class DecompressZipHandler
{
public:
    DecompressZipHandler() : _isFail(false) { }

    void onError(const void*, std::pair<const ZipLocalFileHeader, const std::string>& info)
    {
        poco_error_f1(Poco::Logger::get("DecompressZipHandler"), "extracting file [%s] corrupted!?", info.second);
        _isFail = true;
    }

    bool fail() const { return _isFail; }

private:

    bool _isFail;

};

typedef Poco::Delegate<DecompressZipHandler, std::pair<const ZipLocalFileHeader, const std::string> >
    DecompressZipHandlerType;

#endif
