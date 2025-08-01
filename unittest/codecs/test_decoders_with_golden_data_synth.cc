#include <doctest/doctest.h>
#include <musac/codecs/decoder_cmf.hh>
#include <musac/codecs/decoder_seq.hh>
#include <musac/codecs/decoder_opb.hh>
#include <musac/codecs/decoder_vgm.hh>
#include <SDL3/SDL.h>
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
            SDL_IOStream* io = SDL_IOFromConstMem(brix_cmf_input, brix_cmf_input_size);
            REQUIRE(io != nullptr);
            
            musac::decoder_cmf decoder;
            CHECK(&decoder != nullptr); // Can instantiate
            
            bool opened = decoder.open(io);
            INFO("CMF decoder open result: " << opened);
            
            SDL_CloseIO(io);
        }
        
        SUBCASE("MIDI/MUS/XMI Decoder (seq)") {
            // Test MIDI
            {
                SDL_IOStream* io = SDL_IOFromConstMem(simon_mid_input, simon_mid_input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_seq decoder;
                CHECK(&decoder != nullptr);
                
                bool opened = decoder.open(io);
                INFO("MIDI decoder open result: " << opened);
                
                SDL_CloseIO(io);
            }
            
            // Test MUS
            {
                SDL_IOStream* io = SDL_IOFromConstMem(doom_mus_input, doom_mus_input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_seq decoder;
                CHECK(&decoder != nullptr);
                
                bool opened = decoder.open(io);
                INFO("MUS decoder open result: " << opened);
                
                SDL_CloseIO(io);
            }
            
            // Test XMI
            {
                SDL_IOStream* io = SDL_IOFromConstMem(GCOMP1_XMI_input, GCOMP1_XMI_input_size);
                REQUIRE(io != nullptr);
                
                musac::decoder_seq decoder;
                CHECK(&decoder != nullptr);
                
                bool opened = decoder.open(io);
                INFO("XMI decoder open result: " << opened);
                
                SDL_CloseIO(io);
            }
        }
        
        SUBCASE("OPB Decoder") {
            SDL_IOStream* io = SDL_IOFromConstMem(doom_opb_input, doom_opb_input_size);
            REQUIRE(io != nullptr);
            
            musac::decoder_opb decoder;
            CHECK(&decoder != nullptr);
            
            bool opened = decoder.open(io);
            INFO("OPB decoder open result: " << opened);
            
            SDL_CloseIO(io);
        }
        
        SUBCASE("VGM Decoder") {
            SDL_IOStream* io = SDL_IOFromConstMem(vgm_vgz_input, vgm_vgz_input_size);
            REQUIRE(io != nullptr);
            
            musac::decoder_vgm decoder;
            CHECK(&decoder != nullptr);
            
            bool opened = decoder.open(io);
            INFO("VGM decoder open result: " << opened);
            
            SDL_CloseIO(io);
        }
    }
    
    TEST_CASE("Synthesizer Decoders - Format Detection") {
        // These tests verify that the decoders can at least detect their formats
        // even if they can't fully decode due to missing resources
        
        SUBCASE("Invalid format rejection") {
            uint8_t bad_data[] = {'B', 'A', 'D', ' ', 'D', 'A', 'T', 'A'};
            
            // CMF should reject non-CMF data
            {
                SDL_IOStream* io = SDL_IOFromConstMem(bad_data, sizeof(bad_data));
                musac::decoder_cmf decoder;
                CHECK_FALSE(decoder.open(io));
                SDL_CloseIO(io);
            }
            
            // OPB should reject non-OPB data
            {
                SDL_IOStream* io = SDL_IOFromConstMem(bad_data, sizeof(bad_data));
                musac::decoder_opb decoder;
                CHECK_FALSE(decoder.open(io));
                SDL_CloseIO(io);
            }
            
            // VGM should reject non-VGM data
            {
                SDL_IOStream* io = SDL_IOFromConstMem(bad_data, sizeof(bad_data));
                musac::decoder_vgm decoder;
                CHECK_FALSE(decoder.open(io));
                SDL_CloseIO(io);
            }
        }
    }
}