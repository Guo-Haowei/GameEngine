#include "engine/core/io/file_access_unix.h"

namespace my {

static std::string s_buffer;
class FileAccessFoo : public FileAccessUnix {
public:
    auto OpenInternal(std::string_view path, ModeFlags mode_flags) -> Result<void> override {
        auto info = std::format("[Open]f:{},m:{},a:{};", path, (int)mode_flags, (int)GetAccessType());
        s_buffer = info;
        return Result<void>();
    }
};

TEST(file_access, make_default) {
    FileAccess::MakeDefault<FileAccessFoo>(FileAccess::ACCESS_RESOURCE);
    FileAccess::MakeDefault<FileAccessFoo>(FileAccess::ACCESS_USERDATA);
    FileAccess::MakeDefault<FileAccessFoo>(FileAccess::ACCESS_FILESYSTEM);

    {
        auto file = FileAccess::Open("a.txt", (FileAccess::ModeFlags)10);
    }
    EXPECT_EQ(s_buffer, "[Open]f:a.txt,m:10,a:0;");
    {
        auto file = FileAccess::Open("@res://abc.txt", (FileAccess::ModeFlags)1);
    }
    EXPECT_EQ(s_buffer, "[Open]f:@res://abc.txt,m:1,a:1;");
    {
        auto file = FileAccess::Open("@user://cache", (FileAccess::ModeFlags)7);
    }
    EXPECT_EQ(s_buffer, "[Open]f:@user://cache,m:7,a:2;");

    s_buffer.clear();
}

}  // namespace my
