#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <string>
#include <memory>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <musac/sdk/io_stream.h>

#include <musac/codecs/decoder_aiff.hh>
#include <musac/codecs/decoder_voc.hh>
#include <musac/codecs/decoder_drwav.hh>
#include <musac/codecs/decoder_modplug.hh>
#include <musac/codecs/decoder_seq.hh>
#include <musac/codecs/decoder_cmf.hh>
#include <musac/codecs/decoder_opb.hh>
#include <musac/codecs/decoder_vgm.hh>

using namespace musac;

struct TestData {
    std::string name;
    std::vector<uint8_t> input;
    std::vector<float> output;
    unsigned int channels;
    unsigned int rate;
    bool output_limited;
    size_t limit_samples;
};

std::string sanitize_name(const std::string& filename) {
    std::string result;
    for (char c : filename) {
        if (std::isalnum(c)) {
            result += c;
        } else {
            result += '_';
        }
    }
    // Ensure the name doesn't start with a digit (invalid C identifier)
    if (!result.empty() && std::isdigit(result[0])) {
        result = "file_" + result;
    }
    return result;
}

void write_array_to_file(std::ofstream& out, const std::string& array_name, 
                        const std::vector<uint8_t>& data) {
    out << "const uint8_t " << array_name << "[] = {\n";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) out << "    ";
        out << "0x" << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(data[i]);
        if (i < data.size() - 1) out << ", ";
        if ((i + 1) % 16 == 0) out << "\n";
    }
    if (data.size() % 16 != 0) out << "\n";
    out << "};\n";
    out << "const size_t " << array_name << "_size = " << std::dec << data.size() << ";\n\n";
}

void write_float_array_to_file(std::ofstream& out, const std::string& array_name, 
                              const std::vector<float>& data) {
    out << "const float " << array_name << "[] = {\n";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 8 == 0) out << "    ";
        out << std::scientific << std::setprecision(6) << data[i] << "f";
        if (i < data.size() - 1) out << ", ";
        if ((i + 1) % 8 == 0) out << "\n";
    }
    if (data.size() % 8 != 0) out << "\n";
    out << "};\n";
    out << "const size_t " << array_name << "_size = " << std::dec << data.size() << ";\n\n";
}

std::unique_ptr<decoder> create_decoder(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "aiff") {
        return std::make_unique<decoder_aiff>();
    } else if (ext == "voc") {
        return std::make_unique<decoder_voc>();
    } else if (ext == "wav") {
        return std::make_unique<decoder_drwav>();
    } else if (ext == "mod") {
        return std::make_unique<decoder_modplug>();
    } else if (ext == "mid" || ext == "mus" || ext == "xmi") {
        return std::make_unique<decoder_seq>();
    } else if (ext == "cmf") {
        return std::make_unique<decoder_cmf>();
    } else if (ext == "opb") {
        return std::make_unique<decoder_opb>();
    } else if (ext == "vgz") {
        return std::make_unique<decoder_vgm>();
    }
    
    return nullptr;
}

bool process_file(const std::string& filename, TestData& test_data) {
    // Initialize test data
    test_data.output_limited = false;
    test_data.limit_samples = 0;
    // Read input file
    std::ifstream input_file(filename, std::ios::binary);
    if (!input_file) {
        std::cerr << "Failed to open input file: " << filename << std::endl;
        return false;
    }
    
    input_file.seekg(0, std::ios::end);
    size_t file_size = input_file.tellg();
    input_file.seekg(0, std::ios::beg);
    
    test_data.input.resize(file_size);
    input_file.read(reinterpret_cast<char*>(test_data.input.data()), file_size);
    input_file.close();
    
    // Create decoder
    auto dec = create_decoder(filename);
    if (!dec) {
        std::cerr << "Unsupported file type: " << filename << std::endl;
        return false;
    }
    
    // Create SDL_IOStream from memory (non-const version)
    auto stream = io_from_memory(test_data.input.data(), test_data.input.size());
    if (!stream) {
        std::cerr << "Failed to create io_stream" << std::endl;
        return false;
    }
    
    // Debug output removed for cleaner operation
    
    // Open decoder
    if (!dec->open(stream.get())) {
        std::cerr << "Failed to open decoder for: " << filename << std::endl;
        return false;
    }
    
    test_data.channels = dec->get_channels();
    test_data.rate = dec->get_rate();
    
    // Decode audio with a limit to prevent excessive processing time
    const size_t chunk_size = 4096;
    const size_t max_samples = 44100 * 2; // Limit to 2 seconds of audio at 44.1kHz
    std::vector<float> chunk(chunk_size);
    bool call_again = true;
    
    while (call_again && test_data.output.size() < max_samples) {
        unsigned int decoded = dec->decode(chunk.data(), chunk_size, call_again, test_data.channels);
        if (decoded > 0) {
            // Don't exceed the maximum sample limit
            size_t samples_to_add = std::min(static_cast<size_t>(decoded), 
                                            max_samples - test_data.output.size());
            test_data.output.insert(test_data.output.end(), 
                                  chunk.begin(), 
                                  chunk.begin() + samples_to_add);
        }
    }
    
    test_data.output_limited = (test_data.output.size() >= max_samples);
    test_data.limit_samples = max_samples;
    
    if (test_data.output_limited) {
        std::cout << "Note: Output limited to " << max_samples << " samples (2 seconds)" << std::endl;
    }
    
    stream->close();
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [output_header.h]" << std::endl;
        return 1;
    }
    
    
    std::string input_filename = argv[1];
    std::string output_filename = argc > 2 ? argv[2] : "test_data.h";
    
    TestData test_data;
    test_data.name = sanitize_name(input_filename.substr(input_filename.find_last_of("/\\") + 1));
    
    if (!process_file(input_filename, test_data)) {
        return 1;
    }
    
    // Write output header file
    std::ofstream out(output_filename);
    if (!out) {
        std::cerr << "Failed to create output file: " << output_filename << std::endl;
        return 1;
    }
    
    out << "#pragma once\n\n";
    out << "#include <cstdint>\n";
    out << "#include <cstddef>\n\n";
    
    out << "// Test data generated from: " << input_filename << "\n";
    out << "// Channels: " << test_data.channels << "\n";
    out << "// Sample rate: " << test_data.rate << " Hz\n";
    if (test_data.output_limited) {
        out << "// WARNING: Output was limited to " << test_data.limit_samples 
            << " samples (" << (test_data.limit_samples / test_data.rate) << " seconds)\n";
        out << "// This is a partial decode for testing purposes only\n";
    }
    out << "\n";
    
    write_array_to_file(out, test_data.name + "_input", test_data.input);
    write_float_array_to_file(out, test_data.name + "_output", test_data.output);
    
    out << "const unsigned int " << test_data.name << "_channels = " << test_data.channels << ";\n";
    out << "const unsigned int " << test_data.name << "_rate = " << test_data.rate << ";\n";
    out << "const bool " << test_data.name << "_output_limited = " << (test_data.output_limited ? "true" : "false") << ";\n";
    
    out.close();
    
    std::cout << "Generated test data for: " << input_filename << std::endl;
    std::cout << "Input size: " << test_data.input.size() << " bytes" << std::endl;
    std::cout << "Output size: " << test_data.output.size() << " samples" << std::endl;
    std::cout << "Channels: " << test_data.channels << std::endl;
    std::cout << "Sample rate: " << test_data.rate << " Hz" << std::endl;
    
    return 0;
}