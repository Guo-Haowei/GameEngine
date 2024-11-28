#include "engine/core/io/file_path.h"

namespace my {

TEST(file_path, constructor) {
    FilePath file_path{ "D:\\workspace\\abc.txt" };
    EXPECT_EQ(file_path.String(), "D:/workspace/abc.txt");
}

TEST(file_path, get_extension) {
    FilePath path1{ "D:\\workspace\\abc.txt" };
    EXPECT_EQ(path1.Extension(), ".txt");

    FilePath path2{ "abc.txt.pdf" };
    EXPECT_EQ(path2.Extension(), ".pdf");
}

TEST(file_path, get_extension_no_extension) {
    FilePath path{ "abc" };
    EXPECT_EQ(path.Extension(), "");
}

TEST(file_path, concat) {
    FilePath path = FilePath{ "path" } / (const char*)"to" / FilePath{ "my" } / std::string("random\\file.txt");
    EXPECT_EQ(path.String(), "path/to/my/random/file.txt");
}

}  // namespace my
