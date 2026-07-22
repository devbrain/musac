// Regression tests for malformed/hostile codec inputs: truncated command
// streams, lying size fields, and compressed payloads. These exercise the
// checked-length paths in the VGM player and the Fibonacci-delta decoder in
// the 8SVX codec; run them under ASan to verify no out-of-bounds access.

#include <doctest/doctest.h>
#include <musac/codecs/decoder_vgm.hh>
#include <musac/codecs/decoder_8svx.hh>
#include <musac/sdk/io_stream.hh>
#include <vector>
#include <cstdint>
#include <cmath>

namespace {

// Builds a minimal valid VGM v1.10 image: 0x40-byte header with a YM2413
// clock (so a chip gets created) followed by the given command bytes.
std::vector<uint8_t> makeVgm(const std::vector<uint8_t>& commands) {
    std::vector<uint8_t> data(0x40, 0);

    auto writeU32LE = [&data](size_t pos, uint32_t value) {
        data[pos] = static_cast<uint8_t>(value & 0xFF);
        data[pos + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        data[pos + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        data[pos + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    };

    data[0] = 'V'; data[1] = 'g'; data[2] = 'm'; data[3] = ' ';
    writeU32LE(0x08, 0x00000110);        // version 1.10 -> data starts at 0x40
    writeU32LE(0x10, 3579545);           // YM2413 clock
    writeU32LE(0x18, 44100);             // total samples

    data.insert(data.end(), commands.begin(), commands.end());
    writeU32LE(0x04, static_cast<uint32_t>(data.size() - 4)); // EOF offset
    return data;
}

// Decode a bit of audio; the assertion here is simply "no crash / no OOB".
void drainDecoder(musac::decoder_vgm& decoder) {
    std::vector<float> buf(512);
    bool more = true;
    for (int i = 0; i < 64 && more; ++i) {
        if (decoder.decode(buf.data(), buf.size(), more, 2) == 0) {
            break;
        }
    }
}

} // namespace

// createTest8SVX is defined in test_decoder_8svx.cc
extern std::vector<uint8_t> createTest8SVX(uint32_t numSamples,
                                           uint32_t sampleRate,
                                           uint8_t compressionType);

TEST_SUITE("Codecs::Robustness") {

    TEST_CASE("VGM - minimal valid file loads and decodes") {
        auto data = makeVgm({0x51, 0x30, 0x10,  // YM2413 register write
                             0x62,              // wait 735 samples
                             0x66});            // end of data
        auto io = musac::io_from_memory(data.data(), data.size());

        musac::decoder_vgm decoder;
        CHECK_NOTHROW(decoder.open(io.get()));
        CHECK(decoder.is_open());
        drainDecoder(decoder);
    }

    TEST_CASE("VGM - truncated command operands do not read out of bounds") {
        SUBCASE("register write cut after opcode") {
            auto data = makeVgm({0x62, 0x51});  // 0x51 needs 2 operand bytes
            auto io = musac::io_from_memory(data.data(), data.size());
            musac::decoder_vgm decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            drainDecoder(decoder);
        }
        SUBCASE("register write cut after first operand") {
            auto data = makeVgm({0x62, 0x51, 0x30});
            auto io = musac::io_from_memory(data.data(), data.size());
            musac::decoder_vgm decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            drainDecoder(decoder);
        }
        SUBCASE("16-bit wait cut short") {
            auto data = makeVgm({0x62, 0x61, 0xFF});  // 0x61 needs 2 bytes
            auto io = musac::io_from_memory(data.data(), data.size());
            musac::decoder_vgm decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            drainDecoder(decoder);
        }
        SUBCASE("data block header cut short") {
            auto data = makeVgm({0x62, 0x67, 0x66});  // block type+size missing
            auto io = musac::io_from_memory(data.data(), data.size());
            musac::decoder_vgm decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            drainDecoder(decoder);
        }
    }

    TEST_CASE("VGM - data block size larger than remaining input") {
        // 0x67 0x66 type=0x00, size=0xFFFFFF00 but almost no payload follows
        auto data = makeVgm({0x62, 0x67, 0x66, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x01});
        auto io = musac::io_from_memory(data.data(), data.size());
        musac::decoder_vgm decoder;
        CHECK_NOTHROW(decoder.open(io.get()));
        drainDecoder(decoder);
    }

    TEST_CASE("VGM - lying EOF-offset field") {
        SUBCASE("EOF offset of zero is rejected") {
            // Total size would be 4 bytes: the header can't fit -> must fail,
            // not shrink the buffer and then parse the header out of bounds
            auto data = makeVgm({0x66});
            data[0x04] = 0; data[0x05] = 0; data[0x06] = 0; data[0x07] = 0;
            auto io = musac::io_from_memory(data.data(), data.size());
            musac::decoder_vgm decoder;
            CHECK_THROWS(decoder.open(io.get()));
        }
        SUBCASE("huge EOF offset is clamped to the real size") {
            auto data = makeVgm({0x62, 0x66});
            data[0x04] = 0xFF; data[0x05] = 0xFF; data[0x06] = 0xFF; data[0x07] = 0xFF;
            auto io = musac::io_from_memory(data.data(), data.size());
            musac::decoder_vgm decoder;
            CHECK_NOTHROW(decoder.open(io.get()));
            drainDecoder(decoder);
        }
    }

    TEST_CASE("VGM - data offset pointing past end of file is rejected") {
        auto data = makeVgm({0x66});
        // version >= 1.50 makes the +34 data-offset field authoritative
        data[0x08] = 0x50; data[0x09] = 0x01;
        data[0x34] = 0xFF; data[0x35] = 0xFF; data[0x36] = 0xFF; data[0x37] = 0x0F;
        auto io = musac::io_from_memory(data.data(), data.size());
        musac::decoder_vgm decoder;
        CHECK_THROWS(decoder.open(io.get()));
    }

    TEST_CASE("8SVX - Fibonacci-delta decode reads only the compressed size") {
        // 100 compressed bytes expand 2:1 into 200 samples; the decoder must
        // not read 200 bytes from the 100-byte source (heap overflow pre-fix)
        auto testData = createTest8SVX(100, 8363, 1);
        auto io = musac::io_from_memory(testData.data(), testData.size());

        musac::decoder_8svx decoder;
        REQUIRE_NOTHROW(decoder.open(io.get()));
        CHECK(decoder.is_open());

        std::vector<float> output(512);
        bool more = true;
        size_t decoded = decoder.decode(output.data(), output.size(), more, 1);

        CHECK(decoded == 200);
        for (size_t i = 0; i < decoded; ++i) {
            CHECK(output[i] >= -1.0f);
            CHECK(output[i] <= 1.0f);
        }
    }

    TEST_CASE("8SVX - BODY size field larger than actual data is rejected") {
        auto testData = createTest8SVX(100, 8363, 0);
        // BODY chunk size lives 4 bytes after the "BODY" tag; inflate it
        for (size_t i = 0; i + 8 <= testData.size(); ++i) {
            if (testData[i] == 'B' && testData[i + 1] == 'O'
                && testData[i + 2] == 'D' && testData[i + 3] == 'Y') {
                testData[i + 4] = 0x7F; // ~2GB big-endian chunk size
                break;
            }
        }
        auto io = musac::io_from_memory(testData.data(), testData.size());
        musac::decoder_8svx decoder;
        CHECK_THROWS(decoder.open(io.get()));
    }
}
