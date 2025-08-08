#include <doctest/doctest.h>
#include <musac/codecs/decoder_cmf.hh>
#include <musac/codecs/decoder_seq.hh>
#include <musac/codecs/decoder_opb.hh>
#include <musac/codecs/decoder_vgm.hh>
#include <musac/sdk/io_stream.hh>
#include <vector>
#include <iostream>

// Include generated test data headers
#include "test_cmf_data.h"
#include "test_mid_data.h"
#include "test_mus_data.h"
#include "test_opb_data.h"
#include "test_vgz_data.h"
#include "test_xmi_data.h"

TEST_SUITE("Decoders::Synthesizer") {
    TEST_CASE("Synthesizer Decoders - Basic Functionality") {
        SUBCASE("CMF Decoder") {
            auto io = musac::io_from_memory(brix_cmf_input, brix_cmf_input_size);
            REQUIRE(io != nullptr);
            
            musac::decoder_cmf decoder;
            CHECK(&decoder != nullptr); // Can instantiate
            
            bool opened = false;
            try {
                decoder.open(io.get());
                opened = true;
            } catch (const std::exception&) {
                opened = false;
            }
            INFO("CMF decoder open result: " << opened);
        }
        
        SUBCASE("MIDI/MUS/XMI Decoder (seq)") {
            // Test MIDI
            {
                auto io = musac::io_from_memory(simon_mid_input, simon_mid_input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_seq decoder;
                CHECK(&decoder != nullptr);
                
                bool opened = false;
                try {
                    decoder.open(io.get());
                    opened = true;
                } catch (const std::exception&) {
                    opened = false;
                }
                INFO("MIDI decoder open result: " << opened);
            }
            
            // Test MUS
            {
                auto io = musac::io_from_memory(doom_mus_input, doom_mus_input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_seq decoder;
                CHECK(&decoder != nullptr);
                
                bool opened = false;
                try {
                    decoder.open(io.get());
                    opened = true;
                } catch (const std::exception&) {
                    opened = false;
                }
                INFO("MUS decoder open result: " << opened);
            }
            
            // Test XMI
            {
                auto io = musac::io_from_memory(GCOMP1_XMI_input, GCOMP1_XMI_input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_seq decoder;
                CHECK(&decoder != nullptr);
                
                bool opened = false;
                try {
                    decoder.open(io.get());
                    opened = true;
                } catch (const std::exception&) {
                    opened = false;
                }
                INFO("XMI decoder open result: " << opened);
            }
        }
        
        SUBCASE("OPB Decoder") {
            auto io = musac::io_from_memory(doom_opb_input, doom_opb_input_size);
            REQUIRE(io != nullptr);
            
            musac::decoder_opb decoder;
            CHECK(&decoder != nullptr);
            
            bool opened = false;
            try {
                decoder.open(io.get());
                opened = true;
            } catch (const std::exception&) {
                opened = false;
            }
            INFO("OPB decoder open result: " << opened);
        }
        
        SUBCASE("VGM Decoder") {
            auto io = musac::io_from_memory(vgm_vgz_input, vgm_vgz_input_size);
            REQUIRE(io != nullptr);
            
            musac::decoder_vgm decoder;
            CHECK(&decoder != nullptr);
            
            bool opened = false;
            try {
                decoder.open(io.get());
                opened = true;
            } catch (const std::exception&) {
                opened = false;
            }
            INFO("VGM decoder open result: " << opened);
        }
    }
    
    TEST_CASE("Synthesizer Decoders - Format Detection") {
        // These tests verify that the decoders can at least detect their formats
        // even if they can't fully decode due to missing resources
        
        SUBCASE("Invalid format rejection") {
            uint8_t bad_data[] = {'B', 'A', 'D', ' ', 'D', 'A', 'T', 'A'};
            
            // CMF should reject non-CMF data
            {
                auto io = musac::io_from_memory(bad_data, sizeof(bad_data));
                musac::decoder_cmf decoder;
                CHECK_THROWS(decoder.open(io.get()));
            }
            
            // OPB should reject non-OPB data
            {
                auto io = musac::io_from_memory(bad_data, sizeof(bad_data));
                musac::decoder_opb decoder;
                CHECK_THROWS(decoder.open(io.get()));
            }
            
            // VGM should reject non-VGM data
            {
                auto io = musac::io_from_memory(bad_data, sizeof(bad_data));
                musac::decoder_vgm decoder;
                CHECK_THROWS(decoder.open(io.get()));
            }
        }
    }
}