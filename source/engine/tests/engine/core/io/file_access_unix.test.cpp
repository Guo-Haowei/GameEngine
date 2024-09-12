#include "core/io/file_access_unix.h"

namespace my {

TEST(file_access_unix, open_read_fail) {
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);

    auto err = FileAccess::open("file_access_unix_open_read_fail", FileAccess::READ).error();
    EXPECT_EQ(err.getValue(), ERR_FILE_NOT_FOUND);
    EXPECT_EQ(err.getMessage(), "error code: 7");
}

TEST(file_access_unix, open_read_success) {
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
    const std::string FILE_NAME = "file_access_unix_open_read_success";

    FILE* f = fopen(FILE_NAME.c_str(), "wb");
    fclose(f);

    auto file = FileAccess::open(FILE_NAME, FileAccess::READ).value();

    try {
        std::filesystem::remove(FILE_NAME);
        FAIL();
    } catch (...) {
    }

    file->close();
    ASSERT_TRUE(std::filesystem::remove(FILE_NAME));
}

TEST(file_access_unix, open_write_fail) {
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
    const std::string FILE_NAME = "file_access_unix_open_write_fail";

    std::filesystem::create_directory(FILE_NAME);
    auto err = FileAccess::open(FILE_NAME, FileAccess::WRITE).error();
    EXPECT_EQ(err.getValue(), ERR_FILE_CANT_OPEN);

    ASSERT_TRUE(std::filesystem::remove_all(FILE_NAME));
}

TEST(file_access_unix, open_write_success) {
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
    const std::string FILE_NAME = "file_access_unix_open_write_success";

    {
        auto f = FileAccess::open(FILE_NAME, FileAccess::WRITE).value();
        ASSERT_TRUE(f->isOpen());
        // should call f->close() in destructor
    }

    ASSERT_TRUE(std::filesystem::remove(FILE_NAME));
}

TEST(file_access_unix, write_read_buffer) {
    FileAccess::makeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
    const std::string FILE_NAME = "file_access_unix_write_read_buffer";
    const std::string STRING = "abcdefg";

    {
        auto f = FileAccess::open(FILE_NAME, FileAccess::WRITE).value();
        ASSERT_TRUE(f->isOpen());
        ASSERT_TRUE(f->writeBuffer(STRING.data(), STRING.length()));
    }
    {
        auto f = FileAccess::open(FILE_NAME, FileAccess::READ).value();
        ASSERT_TRUE(f->isOpen());

        char buffer[128]{ 0 };
        ASSERT_TRUE(f->readBuffer(buffer, STRING.length()));

        EXPECT_EQ(std::string(buffer), STRING);
    }
}

}  // namespace my
