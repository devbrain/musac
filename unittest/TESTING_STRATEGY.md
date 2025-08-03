# Testing Strategy for Static vs Shared Library Builds

## Problem Statement

When building musac as a shared library, internal classes (like `audio_mixer`) are not exported and cannot be directly tested. Additionally, when tests are in a static library, the linker may eliminate them as "unused" code.

## Solution: Dual Testing Strategy

### 1. Public API Tests (Always Available)
- Test internal behavior through public interfaces
- Located in: `test_mixer_public_api.cc`, etc.
- Work with both static and shared builds
- Focus on integration and behavior testing

### 2. Internal Tests (Static Build Only)
- Direct testing of internal classes
- Only compiled when `MUSAC_BUILD_SHARED=OFF`
- More granular unit testing

### 3. CMake Configuration

```cmake
# In unittest/CMakeLists.txt
set(PUBLIC_TEST_SOURCES
    core/test_mixer_public_api.cc
    core/test_phase1_thread_safety.cc
    # ... other public API tests
)

set(INTERNAL_TEST_SOURCES
    core/test_internal_mixer.cc
    # ... other internal tests
)

if(MUSAC_BUILD_SHARED)
    set(TEST_SOURCES ${PUBLIC_TEST_SOURCES})
else()
    set(TEST_SOURCES ${PUBLIC_TEST_SOURCES} ${INTERNAL_TEST_SOURCES})
endif()

add_executable(musac_unittest ${TEST_SOURCES})
```

### 4. Test Registration Pattern

For static library builds with doctest, use explicit registration:

```cpp
// test_registry.hh
namespace musac::test {
    void force_test_registration();
}

// test_registry.cc
#include "test_registry.hh"

// Reference each test file to force linking
extern void register_mixer_tests();
extern void register_stream_tests();

void musac::test::force_test_registration() {
    // Just having these declarations forces the linker
    // to include the object files
}

// In test_main.cc
int main(int argc, char** argv) {
    musac::test::force_test_registration();
    return doctest::Context(argc, argv).run();
}
```

## Benefits

1. **Portability**: Tests work regardless of build type
2. **Comprehensive**: Internal tests when possible, public API tests always
3. **No ODR violations**: No duplicate symbols between test and production builds
4. **Clear separation**: Easy to understand which tests run when

## Current Implementation

We've implemented public API tests that thoroughly exercise the mixer through:
- Concurrent stream creation/destruction
- Parallel property modifications
- Stress testing with rapid operations
- Race condition detection

These tests verify the same thread safety properties as direct internal tests would, but through the public interface.