#include <doctest/doctest.h>
#include <cstdint>
#include <limits>

// This will test the future core types after SDL migration
// For now, we use standard types as placeholders

TEST_SUITE("Core::Types") {
    TEST_CASE("Basic integer types have correct sizes") {
        // Test 8-bit types
        CHECK(sizeof(int8_t) == 1);
        CHECK(sizeof(uint8_t) == 1);
        
        // Test 16-bit types
        CHECK(sizeof(int16_t) == 2);
        CHECK(sizeof(uint16_t) == 2);
        
        // Test 32-bit types
        CHECK(sizeof(int32_t) == 4);
        CHECK(sizeof(uint32_t) == 4);
        
        // Test 64-bit types
        CHECK(sizeof(int64_t) == 8);
        CHECK(sizeof(uint64_t) == 8);
    }
    
    TEST_CASE("Integer types have correct ranges") {
        // Signed types
        CHECK(std::numeric_limits<int8_t>::min() == -128);
        CHECK(std::numeric_limits<int8_t>::max() == 127);
        
        CHECK(std::numeric_limits<int16_t>::min() == -32768);
        CHECK(std::numeric_limits<int16_t>::max() == 32767);
        
        CHECK(std::numeric_limits<int32_t>::min() == -2147483648);
        CHECK(std::numeric_limits<int32_t>::max() == 2147483647);
        
        // Unsigned types
        CHECK(std::numeric_limits<uint8_t>::min() == 0);
        CHECK(std::numeric_limits<uint8_t>::max() == 255);
        
        CHECK(std::numeric_limits<uint16_t>::min() == 0);
        CHECK(std::numeric_limits<uint16_t>::max() == 65535);
        
        CHECK(std::numeric_limits<uint32_t>::min() == 0);
        CHECK(std::numeric_limits<uint32_t>::max() == 4294967295U);
    }
    
    TEST_CASE("Floating point types") {
        CHECK(sizeof(float) == 4);
        CHECK(sizeof(double) == 8);
        
        // Check float properties
        CHECK(std::numeric_limits<float>::is_iec559); // IEEE 754
        CHECK(std::numeric_limits<double>::is_iec559);
    }
}