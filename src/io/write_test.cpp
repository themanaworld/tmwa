#include "write.hpp"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <unistd.h>

#include "../strings/fstring.hpp"
#include "../strings/mstring.hpp"
#include "../strings/xstring.hpp"

static
int pipew(int& rfd)
{
    int pfd[2];
    pipe2(pfd, O_NONBLOCK);
    rfd = pfd[0];
    return pfd[1];
}

class PipeWriter
{
private:
    int rfd;
public:
    io::WriteFile wf;
public:
    PipeWriter(bool lb)
    : wf(pipew(rfd), lb)
    {}
    ~PipeWriter()
    {
        close(rfd);
    }
    FString slurp()
    {
        MString tmp;
        char buf[4096];
        while (true)
        {
            ssize_t rv = read(rfd, buf, sizeof(buf));
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
        return FString(tmp);
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
    EXPECT_EQ("", std::string(pw.slurp().c_str()));
    wf.put_line("World!");
    wf.really_put("XXX", 3);
    EXPECT_EQ("Hello, World!\n", std::string(pw.slurp().c_str()));
    EXPECT_TRUE(wf.close());
    EXPECT_EQ("XXX", std::string(pw.slurp().c_str()));
}
