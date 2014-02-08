#include "write.hpp"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <unistd.h>

#include "../strings/astring.hpp"
#include "../strings/mstring.hpp"
#include "../strings/xstring.hpp"

static
io::FD pipew(io::FD& rfd)
{
    io::FD wfd;
    if (-1 == io::FD::pipe2(rfd, wfd, O_NONBLOCK))
    {
        rfd = io::FD();
        return io::FD();
    }
    return wfd;
}

class PipeWriter
{
private:
    io::FD rfd;
public:
    io::WriteFile wf;
public:
    PipeWriter(bool lb)
    : wf(pipew(rfd), lb)
    {}
    ~PipeWriter()
    {
        rfd.close();
    }
    AString slurp()
    {
        MString tmp;
        char buf[4096];
        while (true)
        {
            ssize_t rv = rfd.read(buf, sizeof(buf));
            if (rv == -1)
            {
                if (errno != EAGAIN)
                    return {"Error, read failed :("};
                rv = 0;
            }
            if (rv == 0)
                break;
            tmp += XString(buf + 0, buf + rv, nullptr);
        }
        return AString(tmp);
    }
};

TEST(io, write1)
{
    PipeWriter pw(false);
    io::WriteFile& wf = pw.wf;
    wf.really_put("Hello, ", 7);
    EXPECT_EQ("", pw.slurp());
    wf.put_line("World!\n");
    EXPECT_EQ("", pw.slurp());
    EXPECT_TRUE(wf.close());
    EXPECT_EQ("Hello, World!\n", pw.slurp());
}

TEST(io, write2)
{
    PipeWriter pw(true);
    io::WriteFile& wf = pw.wf;
    wf.really_put("Hello, ", 7);
    EXPECT_EQ("", pw.slurp());
    wf.put_line("World!");
    wf.really_put("XXX", 3);
    EXPECT_EQ("Hello, World!\n", pw.slurp());
    EXPECT_TRUE(wf.close());
    EXPECT_EQ("XXX", pw.slurp());
}
