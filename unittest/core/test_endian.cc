#include <doctest/doctest.h>
#include <cstdint>
#include <cstring>

// Placeholder functions for future endian utilities
// These will be replaced with actual implementations during SDL migration

namespace {
    inline uint16_t swap16(uint16_t value) {
        return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
    }
    
    inline uint32_t swap32(uint32_t value) {
        return ((value & 0x000000FF) << 24) |
               ((value & 0x0000FF00) << 8) |
               ((value & 0x00FF0000) >> 8) |
               ((value & 0xFF000000) >> 24);
    }
    
    inline bool isLittleEndian() {
        uint16_t test = 0x1234;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&test);
        return bytes[0] == 0x34;
    }
}

TEST_SUITE("Core::Endian") {
    TEST_CASE("16-bit byte swapping") {
        CHECK(swap16(0x1234) == 0x3412);
        CHECK(swap16(0x0000) == 0x0000);
        CHECK(swap16(0xFFFF) == 0xFFFF);
        CHECK(swap16(0x00FF) == 0xFF00);
        CHECK(swap16(0xFF00) == 0x00FF);
        
        // Test round-trip
        uint16_t original = 0xABCD;
        CHECK(swap16(swap16(original)) == original);
    }
    
    TEST_CASE("32-bit byte swapping") {
        CHECK(swap32(0x12345678) == 0x78563412);
        CHECK(swap32(0x00000000) == 0x00000000);
        CHECK(swap32(0xFFFFFFFF) == 0xFFFFFFFF);
        CHECK(swap32(0x000000FF) == 0xFF000000);
        CHECK(swap32(0xFF000000) == 0x000000FF);
        
        // Test round-trip
        uint32_t original = 0xDEADBEEF;
        CHECK(swap32(swap32(original)) == original);
    }
    
    TEST_CASE("Platform endianness detection") {
        // Create a known value
        uint32_t test = 0x01020304;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&test);
        
        if (isLittleEndian()) {
            // On little-endian, least significant byte comes first
            CHECK(bytes[0] == 0x04);
            CHECK(bytes[1] == 0x03);
            CHECK(bytes[2] == 0x02);
            CHECK(bytes[3] == 0x01);
        } else {
            // On big-endian, most significant byte comes first
            CHECK(bytes[0] == 0x01);
            CHECK(bytes[1] == 0x02);
            CHECK(bytes[2] == 0x03);
            CHECK(bytes[3] == 0x04);
        }
    }
    
    TEST_CASE("Endian-aware reading") {
        // Simulate reading from a big-endian file
        uint8_t be_data[] = {0x12, 0x34, 0x56, 0x78};
        
        // Read as big-endian 16-bit
        uint16_t be16;
        std::memcpy(&be16, be_data, 2);
        if (isLittleEndian()) {
            be16 = swap16(be16);
        }
        CHECK(be16 == 0x1234);
        
        // Read as big-endian 32-bit
        uint32_t be32;
        std::memcpy(&be32, be_data, 4);
        if (isLittleEndian()) {
            be32 = swap32(be32);
        }
        CHECK(be32 == 0x12345678);
    }
    
    TEST_CASE("Float byte swapping") {
        // Test float swapping by treating as uint32_t
        float original = 3.14159f;
        uint32_t as_int;
        std::memcpy(&as_int, &original, sizeof(float));
        
        uint32_t swapped = swap32(as_int);
        float result;
        std::memcpy(&result, &swapped, sizeof(float));
        
        // Swap back and verify
        std::memcpy(&as_int, &result, sizeof(float));
        as_int = swap32(as_int);
        float restored;
        std::memcpy(&restored, &as_int, sizeof(float));
        
        CHECK(restored == doctest::Approx(original));
    }
}