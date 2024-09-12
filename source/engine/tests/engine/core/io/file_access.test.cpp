#include "core/io/file_access_unix.h"

namespace my {

static std::string s_buffer;
class FileAccessFoo : public FileAccessUnix {
public:
    virtual std::string fixPath(std::string_view path) override {
        return std::string(path);
    }

    virtual ErrorCode openInternal(std::string_view path, int mode_flags) override {
        auto info = std::format("[open]f:{},m:{},a:{};", path, mode_flags, (int)getAccessType());
        s_buffer = info;
        return OK;
    }
};

TEST(file_access, make_default) {
    FileAccess::makeDefault<FileAccessFoo>(FileAccess::ACCESS_RESOURCE);
    FileAccess::makeDefault<FileAccessFoo>(FileAccess::ACCESS_USERDATA);
    FileAccess::makeDefault<FileAccessFoo>(FileAccess::ACCESS_FILESYSTEM);

    { auto file = FileAccess::open("a.txt", 10); }
    EXPECT_EQ(s_buffer, "[open]f:a.txt,m:10,a:0;");
    { auto file = FileAccess::open("@res://abc.txt", 1); }
    EXPECT_EQ(s_buffer, "[open]f:@res://abc.txt,m:1,a:1;");
    { auto file = FileAccess::open("@user://cache", 7); }
    EXPECT_EQ(s_buffer, "[open]f:@user://cache,m:7,a:2;");

    s_buffer.clear();
}

}  // namespace my
