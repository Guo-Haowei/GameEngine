#include "core/io/file_path.h"

namespace my {

TEST(file_path, constructor) {
    FilePath file_path{ "D:\\workspace\\abc.txt" };
    EXPECT_EQ(file_path.string(), "D:/workspace/abc.txt");
}

TEST(file_path, get_extension) {
    FilePath path1{ "D:\\workspace\\abc.txt" };
    EXPECT_EQ(path1.extension(), ".txt");

    FilePath path2{ "abc.txt.pdf" };
    EXPECT_EQ(path2.extension(), ".pdf");
}

TEST(file_path, get_extension_no_extension) {
    FilePath path{ "abc" };
    EXPECT_EQ(path.extension(), "");
}

TEST(file_path, concat) {
    FilePath path = FilePath{ "path" } / (const char*)"to" / FilePath{ "my" } / std::string("random\\file.txt");
    EXPECT_EQ(path.string(), "path/to/my/random/file.txt");
}

}  // namespace my
