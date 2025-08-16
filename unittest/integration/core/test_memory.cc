#include <doctest/doctest.h>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

// Memory utility tests
// These test standard memory operations that will replace SDL equivalents

template<typename T>
void zero(T& obj) {
    std::memset(&obj, 0, sizeof(T));
}

TEST_SUITE("Core::Memory") {
    TEST_CASE("Memory operations") {
        SUBCASE("memcpy") {
            const char src[] = "Hello, World!";
            char dst[32] = {0};
            
            std::memcpy(dst, src, sizeof(src));
            CHECK(std::string(dst) == std::string(src));
            
            // Test non-overlapping copy
            char buffer1[] = "ABCDEFGHIJ";
            char buffer2[11] = {0};
            std::memcpy(buffer2, buffer1, 10);
            CHECK(std::string(buffer2, 10) == "ABCDEFGHIJ");
        }
        
        SUBCASE("memset") {
            uint8_t buffer[16];
            
            // Fill with pattern
            std::memset(buffer, 0xAB, sizeof(buffer));
            for (size_t i = 0; i < sizeof(buffer); i++) {
                CHECK(buffer[i] == 0xAB);
            }
            
            // Zero out
            std::memset(buffer, 0, sizeof(buffer));
            for (size_t i = 0; i < sizeof(buffer); i++) {
                CHECK(buffer[i] == 0);
            }
        }
        
        SUBCASE("memmove") {
            // Test with overlapping regions
            char buffer[] = "1234567890";
            
            // Move forward (overlapping)
            std::memmove(buffer + 2, buffer, 5);
            CHECK(std::string(buffer, 10) == "1212345890");
            
            // Reset
            std::strcpy(buffer, "1234567890");
            
            // Move backward (overlapping)
            std::memmove(buffer, buffer + 2, 5);
            CHECK(std::string(buffer, 10) == "3456767890");
        }
        
        SUBCASE("memcmp") {
            const char str1[] = "Hello";
            const char str2[] = "Hello";
            const char str3[] = "World";
            
            CHECK(std::memcmp(str1, str2, 5) == 0);
            CHECK(std::memcmp(str1, str3, 5) < 0); // 'H' < 'W'
            CHECK(std::memcmp(str3, str1, 5) > 0); // 'W' > 'H'
            
            // Test with binary data
            uint8_t data1[] = {0x12, 0x34, 0x56, 0x78};
            uint8_t data2[] = {0x12, 0x34, 0x56, 0x78};
            uint8_t data3[] = {0x12, 0x34, 0x57, 0x78};
            
            CHECK(std::memcmp(data1, data2, 4) == 0);
            CHECK(std::memcmp(data1, data3, 4) < 0);
        }
    }
    
    TEST_CASE("Zero template function") {
        SUBCASE("POD types") {
            int value = 42;
            zero(value);
            CHECK(value == 0);
            
            float fvalue = 3.14f;
            zero(fvalue);
            CHECK(fvalue == 0.0f);
        }
        
        SUBCASE("Structures") {
            struct TestStruct {
                int a;
                float b;
                char c[8];
            };
            
            TestStruct s = {42, 3.14f, "Hello"};
            zero(s);
            
            CHECK(s.a == 0);
            CHECK(s.b == 0.0f);
            CHECK(s.c[0] == 0);
            CHECK(s.c[7] == 0);
        }
        
        SUBCASE("Arrays") {
            int arr[10];
            std::fill(std::begin(arr), std::end(arr), 99);
            
            zero(arr);
            
            for (int i = 0; i < 10; i++) {
                CHECK(arr[i] == 0);
            }
        }
    }
    
    TEST_CASE("Memory allocation patterns") {
        SUBCASE("Dynamic allocation") {
            // Test malloc/free pattern
            void* ptr = std::malloc(1024);
            CHECK(ptr != nullptr);
            
            // Use the memory
            std::memset(ptr, 0xFF, 1024);
            
            std::free(ptr);
        }
        
        SUBCASE("Aligned allocation") {
            // C++17 aligned allocation
            size_t alignment = 32;
            size_t size = 1024;
            
            void* ptr = std::aligned_alloc(alignment, size);
            CHECK(ptr != nullptr);
            CHECK(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
            
            std::free(ptr);
        }
        
        SUBCASE("Vector as dynamic buffer") {
            // Common pattern for dynamic audio buffers
            std::vector<float> buffer(1024);
            
            CHECK(buffer.size() == 1024);
            CHECK(buffer.data() != nullptr);
            
            // Resize dynamically
            buffer.resize(2048);
            CHECK(buffer.size() == 2048);
            
            // Access raw pointer for C APIs
            float* raw_ptr = buffer.data();
            CHECK(raw_ptr != nullptr);
        }
    }
    
    TEST_CASE("Stack allocation helpers") {
        SUBCASE("Stack arrays") {
            constexpr size_t STACK_SIZE = 256;
            
            // Simulate stack allocation
            uint8_t stack_buffer[STACK_SIZE];
            
            // Initialize
            std::memset(stack_buffer, 0, STACK_SIZE);
            
            // Use as temporary buffer
            const char* test_data = "Temporary data";
            std::memcpy(stack_buffer, test_data, strlen(test_data) + 1);
            
            CHECK(std::string(reinterpret_cast<char*>(stack_buffer)) == test_data);
        }
        
        SUBCASE("alloca simulation") {
            // Note: alloca is non-standard, using VLA pattern
            auto allocate_on_stack = [](size_t size) {
                std::vector<uint8_t> buffer(size);
                
                // Do something with buffer
                std::fill(buffer.begin(), buffer.end(), 0xAA);
                
                // Verify
                for (size_t i = 0; i < size; i++) {
                    CHECK(buffer[i] == 0xAA);
                }
            };
            
            allocate_on_stack(128);
            allocate_on_stack(256);
            allocate_on_stack(512);
        }
    }
}