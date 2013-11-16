// Reads .gat files by name-mapping .wlk files
#include "grfio.hpp"

#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <map>

#include "../strings/mstring.hpp"
#include "../strings/fstring.hpp"

#include "../io/read.hpp"

#include "../common/cxxstdio.hpp"
#include "../common/extract.hpp"

#include "../poison.hpp"

static
std::map<MapName, FString> load_resnametable()
{
    io::ReadFile in("data/resnametable.txt");
    if (!in.is_open())
    {
        fprintf(stderr, "Missing data/resnametable.txt");
        abort();
    }
    std::map<MapName, FString> out;

    FString line;
    while (in.getline(line))
    {
        MapName key;
        FString value;
        if (!extract(line,
                    record<'#'>(&key, &value)))
            continue;
        out[key] = value;
    }
    return out;
}

/// Change *.gat to *.wlk
static
FString grfio_resnametable(MapName rname)
{
    static
    std::map<MapName, FString> resnametable = load_resnametable();

    return resnametable.at(rname);
}

std::vector<uint8_t> grfio_reads(MapName rname)
{
    MString lfname_;
    lfname_ += "data/";
    lfname_ += grfio_resnametable(rname);
    FString lfname = FString(lfname_);

    int fd = open(lfname.c_str(), O_RDONLY);
    if (fd == -1)
    {
        FPRINTF(stderr, "Resource %s (file %s) not found\n",
                rname, lfname);
        return {};
    }
    off_t len = lseek(fd, 0, SEEK_END);
    assert (len != -1);
    std::vector<uint8_t> buffer(len);
    ssize_t err = pread(fd, buffer.data(), len, 0);
    assert (err == len);
    close(fd);
    return buffer;
}
