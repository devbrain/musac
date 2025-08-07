#include <doctest/doctest.h>
#include <cstdint>
#include <vector>
#include <cstring>
#include <fstream>
#include <filesystem>

// Placeholder IO stream interface for testing
// This will be replaced with actual implementation during SDL migration

class IOStream {
public:
    virtual ~IOStream() = default;
    virtual size_t read(void* ptr, size_t size) = 0;
    virtual size_t write(const void* ptr, size_t size) = 0;
    virtual int64_t seek(int64_t offset, int whence) = 0;
    virtual int64_t tell() = 0;
    virtual int64_t size() = 0;
};

class MemoryStream : public IOStream {
private:
    std::vector<uint8_t> m_data;
    size_t m_position = 0;
    
public:
    MemoryStream() = default;
    MemoryStream(const uint8_t* data, size_t size) 
        : m_data(data, data + size), m_position(0) {}
    
    size_t read(void* ptr, size_t size) override {
        if (m_position >= m_data.size()) {
            return 0;  // Position is past end of data
        }
        size_t available = m_data.size() - m_position;
        size_t to_read = std::min(size, available);
        std::memcpy(ptr, m_data.data() + m_position, to_read);
        m_position += to_read;
        return to_read;
    }
    
    size_t write(const void* ptr, size_t size) override {
        const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
        m_data.insert(m_data.begin() + static_cast<std::ptrdiff_t>(m_position), bytes, bytes + size);
        m_position += size;
        return size;
    }
    
    int64_t seek(int64_t offset, int whence) override {
        switch (whence) {
            case SEEK_SET:
                if (offset >= 0) {
                    m_position = static_cast<size_t>(offset);
                }
                break;
            case SEEK_CUR:
                if (offset >= 0 || static_cast<size_t>(-offset) <= m_position) {
                    m_position = static_cast<size_t>(static_cast<int64_t>(m_position) + offset);
                }
                break;
            case SEEK_END:
                if (offset <= 0 || static_cast<size_t>(offset) <= m_data.size()) {
                    m_position = static_cast<size_t>(static_cast<int64_t>(m_data.size()) + offset);
                }
                break;
        }
        return static_cast<int64_t>(m_position);
    }
    
    int64_t tell() override {
        return static_cast<int64_t>(m_position);
    }
    
    int64_t size() override {
        return static_cast<int64_t>(m_data.size());
    }
};

TEST_SUITE("Core::IOStream") {
    TEST_CASE("MemoryStream basic operations") {
        MemoryStream stream;
        
        SUBCASE("Write and read back") {
            const char* test_data = "Hello, World!";
            size_t len = strlen(test_data);
            
            CHECK(stream.write(test_data, len) == len);
            CHECK(stream.size() == len);
            
            // Seek to beginning
            CHECK(stream.seek(0, SEEK_SET) == 0);
            
            // Read back
            char buffer[32] = {0};
            CHECK(stream.read(buffer, len) == len);
            CHECK(std::string(buffer) == test_data);
        }
        
        SUBCASE("Binary data") {
            uint32_t values[] = {0x12345678, 0xDEADBEEF, 0xCAFEBABE};
            
            CHECK(stream.write(values, sizeof(values)) == sizeof(values));
            CHECK(stream.size() == sizeof(values));
            
            stream.seek(0, SEEK_SET);
            
            uint32_t read_values[3] = {0};
            CHECK(stream.read(read_values, sizeof(read_values)) == sizeof(read_values));
            CHECK(read_values[0] == values[0]);
            CHECK(read_values[1] == values[1]);
            CHECK(read_values[2] == values[2]);
        }
    }
    
    TEST_CASE("MemoryStream seeking") {
        const uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        MemoryStream stream(data, sizeof(data));
        
        SUBCASE("SEEK_SET") {
            CHECK(stream.seek(5, SEEK_SET) == 5);
            CHECK(stream.tell() == 5);
            
            uint8_t value;
            stream.read(&value, 1);
            CHECK(value == 5);
        }
        
        SUBCASE("SEEK_CUR") {
            stream.seek(3, SEEK_SET);
            CHECK(stream.seek(2, SEEK_CUR) == 5);
            CHECK(stream.tell() == 5);
            
            CHECK(stream.seek(-2, SEEK_CUR) == 3);
            CHECK(stream.tell() == 3);
        }
        
        SUBCASE("SEEK_END") {
            CHECK(stream.seek(-1, SEEK_END) == 9);
            uint8_t value;
            stream.read(&value, 1);
            CHECK(value == 9);
            
            CHECK(stream.seek(-5, SEEK_END) == 5);
            stream.read(&value, 1);
            CHECK(value == 5);
        }
    }
    
    TEST_CASE("MemoryStream bounds checking") {
        const uint8_t data[] = {1, 2, 3, 4, 5};
        MemoryStream stream(data, sizeof(data));
        
        SUBCASE("Read past end") {
            uint8_t buffer[10] = {0};
            CHECK(stream.read(buffer, 10) == 5);
            CHECK(buffer[0] == 1);
            CHECK(buffer[4] == 5);
            CHECK(buffer[5] == 0); // Unchanged
        }
        
        SUBCASE("Seek past end") {
            stream.seek(10, SEEK_SET);
            CHECK(stream.tell() == 10);
            
            uint8_t value;
            CHECK(stream.read(&value, 1) == 0); // No data to read
        }
    }
    
    TEST_CASE("FileStream placeholder test") {
        // This test creates a temporary file to test file I/O
        // Will be replaced with actual FileStream implementation
        
        const char* test_file = "test_io_stream_temp.bin";
        
        SUBCASE("Write and read file") {
            // Write test data
            {
                std::ofstream out(test_file, std::ios::binary);
                uint32_t magic = 0x12345678;
                uint16_t values[] = {0x1234, 0x5678, 0x9ABC};
                
                out.write(reinterpret_cast<char*>(&magic), sizeof(magic));
                out.write(reinterpret_cast<char*>(values), sizeof(values));
            }
            
            // Read back
            {
                std::ifstream in(test_file, std::ios::binary);
                uint32_t magic;
                uint16_t values[3];
                
                in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
                in.read(reinterpret_cast<char*>(values), sizeof(values));
                
                CHECK(magic == 0x12345678);
                CHECK(values[0] == 0x1234);
                CHECK(values[1] == 0x5678);
                CHECK(values[2] == 0x9ABC);
            }
            
            // Cleanup
            std::filesystem::remove(test_file);
        }
    }
}