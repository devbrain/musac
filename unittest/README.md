# Musac Unit Tests

This directory contains unit tests for the musac audio library using the doctest framework.

## Structure

```
unittest/
├── CMakeLists.txt      # Build configuration
├── test_main.cc        # Main test runner
├── core/               # Core functionality tests (future SDL migration)
│   ├── test_types.cc
│   ├── test_endian.cc
│   ├── test_io_stream.cc
│   ├── test_audio_format.cc
│   └── test_memory.cc
├── sdk/                # SDK component tests
│   ├── test_audio_converter.cc
│   ├── test_samples_converter.cc
│   └── test_decoder_base.cc
└── codecs/             # Codec decoder tests
    ├── test_decoder_aiff.cc
    ├── test_decoder_voc.cc
    └── test_decoder_wav.cc
```

## Building and Running Tests

### Quick Start
```bash
# From musac root directory
./run_tests.sh
```

### Manual Build
```bash
mkdir build
cd build
cmake .. -DMUSAC_BUILD_TESTS=ON
cmake --build . --target musac_unittest
./unittest/musac_unittest
```

### Running Specific Tests
```bash
# List all test cases
./build/unittest/musac_unittest --list-test-cases

# Run only Core tests
./build/unittest/musac_unittest --test-case="Core::*"

# Run only SDK tests
./build/unittest/musac_unittest --test-case="SDK::*"

# Run specific test suite
./build/unittest/musac_unittest --test-suite="Core::Endian"

# Run with verbose output
./build/unittest/musac_unittest --success
```

### Test Output Options
```bash
# XML output for CI
./build/unittest/musac_unittest --reporters=xml --out=test_results.xml

# JUnit format
./build/unittest/musac_unittest --reporters=junit --out=junit_results.xml

# Console with colors
./build/unittest/musac_unittest --reporters=console --use-colour=yes
```

## Writing New Tests

### Test Structure
```cpp
#include <doctest/doctest.h>

TEST_SUITE("Component::Feature") {
    TEST_CASE("Description of test") {
        // Setup
        int value = 42;
        
        // Test
        CHECK(value == 42);
        
        SUBCASE("Specific scenario") {
            value *= 2;
            CHECK(value == 84);
        }
    }
}
```

### Assertions
- `CHECK(expression)` - Test continues on failure
- `REQUIRE(expression)` - Test stops on failure
- `CHECK_FALSE(expression)` - Inverted check
- `CHECK_THROWS(expression)` - Expects exception
- `CHECK_NOTHROW(expression)` - Expects no exception
- `CHECK(value == doctest::Approx(expected))` - Floating point comparison

## Test Categories

### Core Tests
These tests are placeholders for the future SDL migration. They test:
- Basic type definitions and properties
- Endian conversion utilities
- I/O stream abstractions
- Audio format definitions
- Memory utilities

### SDK Tests
Tests for the SDK components:
- Audio format conversion
- Sample conversion to float
- Decoder base class functionality

### Codec Tests
Tests for specific audio format decoders:
- AIFF decoder (including 8SVX)
- VOC decoder (Creative Voice)
- WAV decoder (via dr_wav)

## Dependencies

The tests depend on:
- doctest (fetched automatically if not found)
- musac_sdk
- musac_codecs
- SDL3 (will be removed after migration)
- failsafe

## Future Work

1. **SDL Migration Tests**: The core tests are prepared for testing the SDL-free implementations
2. **Performance Tests**: Add benchmarking for critical paths
3. **Integration Tests**: Test complete audio pipeline
4. **Fuzz Testing**: Add fuzzing for file format parsers
5. **Coverage**: Set up code coverage reporting