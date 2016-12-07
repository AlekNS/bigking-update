
#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <vector>
#include <string>


using std::pair;
using std::vector;
using std::string;


class Software;
class VersionInfo;


//============================================================================//
class DownloaderHolder
{
public:

    DownloaderHolder(const string &name, const string &path);

    string  name() const;
    string  path() const;
    string  pathExtracted() const;

    string  extract();
    void    remove();

private:

    string              _path, _name, _pathExtracted;
};

//============================================================================//
class Downloader
{
public:

    static DownloaderHolder download(Software &software, const VersionInfo &version);

};


#endif
