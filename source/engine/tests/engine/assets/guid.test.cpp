#include <engine/assets/guid.h>

namespace my {

TEST(guid, default_constructor) {
    Guid guid;

    std::array<uint8_t, 16> bytes;
    bytes.fill(0);
    static_assert(sizeof(bytes) == sizeof(guid));

    const int result = memcmp(&guid, bytes.data(), sizeof(guid));
    EXPECT_EQ(result, 0);
}

TEST(guid, constructor) {
    std::array<uint8_t, 16> bytes;
    uint8_t counter = 1;
    for (auto& i : bytes) {
        i = counter++;
    }

    Guid guid(bytes.data());
    const int result = memcmp(&guid, bytes.data(), sizeof(guid));
    EXPECT_EQ(result, 0);
}

TEST(guid, to_string) {
    // clang-format off
	const uint8_t bytes[] = {
		0xAA, 0xBB, 0xCC, 0xDD,
		0xEE, 0xFF,
		0x10, 0xAA,
		0xEA, 0xEB, 0xEC, 0xED, 0xFA, 0xFB, 0xFC, 0xFD,
    };
    // clang-format on
    static_assert(sizeof(bytes) == 16);

    Guid guid(bytes);
    std::string str = guid.ToString();
    EXPECT_EQ(str, "AABBCCDD-EEFF-10AA-EAEBECEDFAFBFCFD");
}

TEST(guid, padd_zero) {
    Guid guid;
    std::string str = guid.ToString();
    EXPECT_EQ(str, "00000000-0000-0000-0000000000000000");
}

TEST(guid, generation) {
    Guid guid1 = Guid::Create();
    Guid guid2 = Guid::Create();

    std::string str1 = guid1.ToString();
    std::string str2 = guid2.ToString();
    if (str1 != str2) {
        EXPECT_NE(guid1, guid2);
    } else {
        ASSERT_FALSE(0 && "Guid collision?");
    }
}

TEST(guid, parse_wrong_length) {
    const std::string source = "578F-E7F234-E948-9EBCEF5BA35E854C";
    auto res = Guid::Parse(source);
    EXPECT_FALSE(res.has_value());
}

TEST(guid, parse_wrong_dash) {
    const std::string source = "61578F-E7F234-E948-9EBCEF5BA35E854C";
    auto res = Guid::Parse(source);
    EXPECT_FALSE(res.has_value());
}

TEST(guid, parse_wrong_value) {
    const std::string source = "61578X-E7G234-E948-9EBCEF5BA35E854C";
    auto res = Guid::Parse(source);
    EXPECT_FALSE(res.has_value());
}

TEST(guid, parse_ok) {
    const std::string source = "61578FE7-F234-E948-9EBCEF5BA35E854C";
    auto res = Guid::Parse(source);
    ASSERT_TRUE(res);
    Guid guid = *res;
    std::string dump = guid.ToString();
    EXPECT_EQ(source, dump);
}

TEST(guid, compare) {
    Guid guid = Guid::Create();

    const std::string source = guid.ToString();

    auto res = Guid::Parse(source);
    ASSERT_TRUE(res);
    Guid guid2 = *res;
    EXPECT_EQ(guid, guid2);
}

TEST(guid, hasing) {
    std::set<Guid> guids;

    Guid guid = Guid::Create();
    const std::string source = guid.ToString();
    auto res = Guid::Parse(source);
    ASSERT_TRUE(res);
    Guid guid2 = *res;

    guids.insert(guid);

    {
        auto it = guids.find(guid2);
        EXPECT_TRUE(it != guids.end());
    }
    {
        auto it = guids.find(guid);
        EXPECT_TRUE(it != guids.end());
    }
    {
        Guid guid3;
        auto it = guids.find(guid3);
        EXPECT_TRUE(it == guids.end());
    }
}

}  // namespace my
