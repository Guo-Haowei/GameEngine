#include "engine/core/io/archive.h"

#include "engine/core/io/file_access_unix.h"

namespace my {

// @TODO: allow archive to implement string stream
TEST(archive, open_read) {
    FileAccess::MakeDefault<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);

    Archive archive;
    auto err = archive.OpenRead("path_that_does_not_exist").error();
    EXPECT_EQ(err->value, ErrorCode::ERR_FILE_NOT_FOUND);
}

TEST(archive, open_write) {
    const std::string test_file = "archive_test_open_write";

    Archive archive;
    auto result = archive.OpenWrite(test_file);
    EXPECT_TRUE(result);
    EXPECT_TRUE(archive.IsWriteMode());

    archive.Close();
    EXPECT_TRUE(std::filesystem::remove(test_file));
}

TEST(archive, write_and_read) {
    const char* test_file = "archive_test_write_and_read";
    const std::string test_string = "add3to2";
    const int test_int = 12345;
    const char test_byte = 0x11;
    const double test_double = 3.1415926;
    const char test_cstring[8] = "abcdefg";

    Archive writer;

    EXPECT_TRUE(writer.OpenWrite(test_file));
    EXPECT_TRUE(writer.IsWriteMode());

    writer << test_int;
    writer << test_byte;
    writer << test_string;
    writer << test_double;
    writer << test_cstring;

    writer.Close();

    Archive reader;
    EXPECT_TRUE(reader.OpenRead(test_file));
    EXPECT_FALSE(reader.IsWriteMode());

    int actual_int;
    reader >> actual_int;
    EXPECT_EQ(actual_int, test_int);

    char actual_byte;
    reader >> actual_byte;
    EXPECT_EQ(actual_byte, test_byte);

    std::string actual_string;
    reader >> actual_string;
    EXPECT_EQ(actual_string, test_string);

    double actual_double;
    reader >> actual_double;
    EXPECT_EQ(actual_double, test_double);

    char actual_c_string[sizeof(test_cstring)]{ 0 };
    reader >> actual_c_string;
    EXPECT_STREQ(actual_c_string, test_cstring);

    reader.Close();

    EXPECT_TRUE(std::filesystem::remove(test_file));
}

}  // namespace my
