// Reads .gat files by name-mapping .wlk files
#include "grfio.hpp"

#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <map>

#include "../common/cxxstdio.hpp"
#include "../common/extract.hpp"

#include "../poison.hpp"

static
std::map<std::string, std::string> load_resnametable()
{
    std::ifstream in("data/resnametable.txt");
    if (!in.is_open())
    {
        fprintf(stderr, "Missing data/resnametable.txt");
        abort();
    }
    std::map<std::string, std::string> out;

    std::string line;
    while (std::getline(in, line))
    {
        std::string key, value;
        if (!extract(line,
                    record<'#'>(&key, &value)))
            continue;
        out[key] = value;
    }
    return out;
}

/// Change *.gat to *.wlk
static
std::string grfio_resnametable(const_string rname)
{
    static
    std::map<std::string, std::string> resnametable = load_resnametable();

    return resnametable.at(std::string(rname.begin(), rname.end()));
}

std::vector<uint8_t> grfio_reads(const_string rname)
{
    std::string lfname = "data/" + grfio_resnametable(rname);

    int fd = open(lfname.c_str(), O_RDONLY);
    if (fd == -1)
    {
        FPRINTF(stderr, "Resource %s (file %s) not found\n",
                std::string(rname.begin(), rname.end()), lfname);
        return {};
    }
    off_t len = lseek(fd, 0, SEEK_END);
    assert (len != -1);
    std::vector<uint8_t> buffer(len);
    int err = pread(fd, buffer.data(), len, 0);
    assert (err == len);
    close(fd);
    return buffer;
}
