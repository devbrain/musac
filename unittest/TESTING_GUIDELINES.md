# Musac Testing Guidelines

## Overview
This document provides comprehensive guidelines for writing and maintaining tests in the musac audio library. Following these conventions ensures consistency, readability, maintainability, and clean compilation across the test suite.

## Test Organization

### Directory Structure
```
unittest/
├── unit/              # Isolated unit tests using mocks
│   └── core/         # Core library components
├── integration/       # Tests with real components
│   ├── core/         # Core functionality
│   ├── backends/     # Backend implementations
│   ├── codecs/       # Codec integration
│   └── sdk/          # SDK integration
├── mock_backends.hh  # Mock backend implementations
├── mock_components.hh # Mock decoders, sources, streams
└── test_fixtures.hh   # RAII test fixtures
```

**Note**: Only create subdirectories when you have actual test files to place in them. Empty directories should be avoided.

### File Naming Convention
- Unit tests: `test_{component}_unit.cc`
- Integration tests: `test_{component}.cc` or `test_{feature}.cc`
- Mock implementations: `mock_{component}.hh`
- Test fixtures: `test_fixtures.hh`
- Test helpers: `{functionality}_helpers.hh`

## Test Naming Conventions

### Test Suite Names
Use consistent hierarchical naming pattern `Component::Category::Type`:

```cpp
TEST_SUITE("Component::Category::Type") {
    // Examples:
    // "AudioDevice::Unit"                    // Unit tests
    // "AudioDevice::Integration"             // Integration tests
    // "AudioDevice::BackendV2::Integration"  // Specific feature tests
    // "ThreadSafety::Phase1::Integration"    // Phased testing
    // "ErrorScenarios::Device"               // Error handling tests
}
```

### Test Case Names
Use descriptive names that clearly state what is being tested:

```cpp
TEST_CASE("should_[expected_behavior]_when_[condition]") {
    // Examples:
    // "should_throw_runtime_error_when_backend_not_initialized"
    // "should_clamp_gain_when_value_exceeds_range"
    // "should_handle_concurrent_stream_creation_without_deadlock"
}
```

### Subcase Names
Use condition-oriented descriptions:

```cpp
SUBCASE("[condition/scenario]") {
    // Examples:
    // "backend_fails_during_initialization"
    // "with_custom_audio_spec"
    // "extreme_volume_values"
    // "concurrent_stream_destruction"
}
```

## Documentation Standards

### Test File Header
Every test file should begin with a documentation header:

```cpp
/**
 * @file test_component_name.cc
 * @brief Unit/Integration tests for ComponentName
 * 
 * Test Coverage:
 * - Feature 1: Basic functionality
 * - Feature 2: Error handling
 * - Feature 3: Edge cases
 * - Feature 4: Thread safety
 * 
 * Dependencies:
 * - Mock/Real backends
 * - Test fixtures used
 */
```

### Test Case Documentation
Document complex test scenarios inline:

```cpp
TEST_CASE("should_handle_device_switching_during_playback") {
    /**
     * Verifies that the audio system can gracefully switch between
     * devices while streams are actively playing, ensuring no data
     * loss or memory leaks occur during the transition.
     */
    
    // Arrange: Create active streams
    // Act: Switch devices
    // Assert: Verify continuity
}
```

## Mock Infrastructure

### Available Mock Components

1. **Mock Backends** (`mock_backends.hh`):
   - `mock_backend_v2`: Basic mock backend
   - `mock_backend_v2_enhanced`: Backend with error injection
   - `mock_stream`: Mock audio stream interface

2. **Mock Components** (`mock_components.hh`):
   - `test_decoder`: Generates test patterns (silence, sine, noise)
   - `mock_audio_source`: Moveable audio source mock
   - `mock_decoder_with_errors`: Decoder with error injection
   - `mock_io_stream`: IO stream with error simulation
   - `memory_io_stream`: Memory-based IO for testing

3. **Test Fixtures** (`test_fixtures.hh`):
   - `audio_test_fixture_v2`: Basic RAII fixture
   - `audio_test_fixture_threadsafe`: Thread-safe fixture with cleanup delay

### Mock Usage Patterns

```cpp
// Create mock with error injection
auto backend = create_failing_backend(
    fail_init,        // Backend init fails
    fail_enumerate,   // Device enumeration fails
    fail_open_device, // Device open fails
    fail_create_stream // Stream creation fails
);

// Create mock source with specific duration
auto source = create_mock_source(44100);  // 1 second at 44.1kHz

// Create decoder with error simulation
auto decoder = std::make_unique<mock_decoder_with_errors>();
decoder->fail_on_decode = true;
decoder->wrong_format = true;
```

## Clean Compilation Guidelines

### Avoiding Warnings

1. **Type Conversions**: Always use explicit casts
```cpp
// Bad: Implicit conversion warning
float volume = (j % 11) / 10.0f;

// Good: Explicit cast
float volume = static_cast<float>(j % 11) / 10.0f;
```

2. **Sign Conversions**: Match types in loops and containers
```cpp
// Bad: Signed/unsigned mismatch
for (int i = 0; i < vector.size(); ++i)

// Good: Use size_t for container indices
for (size_t i = 0; i < vector.size(); ++i)
```

3. **Unused Return Values**: Handle [[nodiscard]] attributes
```cpp
// Bad: Ignoring nodiscard return value
CHECK_NOTHROW(stream.is_playing());

// Good: Cast to void to acknowledge
CHECK_NOTHROW((void)stream.is_playing());
```

4. **Unused Variables**: Remove or mark as unused
```cpp
// Bad: Variable set but not used
float volume = stream.volume();

// Good: Use the variable or remove it
CHECK(stream.volume() >= 0.0f);
```

## Error Scenario Testing

### Comprehensive Error Coverage

Test files should include error scenarios:

```cpp
TEST_SUITE("ErrorScenarios::Component") {
    TEST_CASE("should_handle_backend_failures_gracefully") {
        SUBCASE("backend_fails_during_initialization") { }
        SUBCASE("backend_fails_during_enumeration") { }
        SUBCASE("backend_fails_during_device_open") { }
    }
    
    TEST_CASE("should_handle_invalid_operations") {
        SUBCASE("operations_on_closed_device") { }
        SUBCASE("invalid_device_id") { }
        SUBCASE("extreme_parameter_values") { }
    }
    
    TEST_CASE("should_handle_resource_exhaustion") {
        SUBCASE("too_many_devices_opened") { }
        SUBCASE("too_many_streams_created") { }
        SUBCASE("out_of_memory_conditions") { }
    }
}
```

### Error Injection Patterns

```cpp
// Simulate specific failures
auto backend = std::make_shared<mock_backend_v2_enhanced>();
backend->fail_init = true;
CHECK_THROWS_AS(backend->init(), std::runtime_error);
CHECK_THROWS_WITH(backend->init(), "Mock backend init failed");

// Test recovery from errors
backend->fail_init = false;
CHECK_NOTHROW(backend->init());
```

## Thread Safety Testing

### Concurrent Test Helpers

Use the provided helper for concurrent testing:

```cpp
TEST_CASE("should_handle_concurrent_operations") {
    auto device = create_test_device();
    
    run_concurrent_test([&device]() {
        // Operation to run concurrently
        device.process_sample();
    }, 
    10,    // thread_count
    1000   // iterations_per_thread
    );
    
    // Verify consistency after concurrent access
    CHECK(device.is_consistent());
}
```

### Race Condition Testing

```cpp
TEST_CASE("should_handle_concurrent_stream_destruction") {
    auto stream = std::make_shared<audio_stream>(...);
    std::atomic<bool> stop{false};
    
    // Thread modifying stream
    std::thread modifier([stream, &stop]() {
        while (!stop) {
            stream->set_volume(0.5f);
            std::this_thread::yield();
        }
    });
    
    // Destroy stream while being accessed
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    stream.reset();
    
    stop = true;
    modifier.join();
    
    // Should not crash
    CHECK(true);
}
```

## Test Stability

### Avoiding Flaky Tests

1. **Skip problematic tests with clear reason**:
```cpp
TEST_CASE("should_handle_concurrent_system_operations" * doctest::skip(true)) {
    // SKIPPED: audio_system is not thread-safe for concurrent init/done calls
}
```

2. **Use reasonable limits for resource tests**:
```cpp
// Instead of trying to allocate 10GB
size_t reasonable_size = 1024 * 1024;  // 1MB
```

3. **Add timeouts for potentially hanging operations**:
```cpp
auto future = std::async(std::launch::async, long_operation);
auto status = future.wait_for(std::chrono::seconds(5));
CHECK(status == std::future_status::ready);
```

## Performance Considerations

### Unit Test Performance

Unit tests should be fast (< 100ms):

```cpp
TEST_CASE("should_complete_quickly") {
    // Use smaller data sets for unit tests
    const size_t SMALL_BUFFER = 1024;  // Not 1,000,000
    
    // Use fewer iterations
    const int ITERATIONS = 100;  // Not 10,000
}
```

### Integration Test Performance

Integration tests can be slower but should still be reasonable:

```cpp
TEST_CASE("integration_test") {
    // Add progress indication for long tests
    for (int i = 0; i < 1000; ++i) {
        if (i % 100 == 0) {
            MESSAGE("Progress: " << i << "/1000");
        }
        // ... test operation
    }
}
```

## Best Practices

### RAII and Resource Management

Always use RAII for resource management:

```cpp
TEST_CASE("resource_management") {
    audio_test_fixture_v2 fixture;  // Automatic init/cleanup
    
    auto device = audio_device::open_default_device(fixture.backend);
    // Device automatically cleaned up at scope exit
    
    CHECK(device.is_valid());
}
```

### Test Independence

Each test must be independent:

```cpp
TEST_CASE("independent_test_1") {
    // This test should not depend on any other test
    audio_system::init(create_test_backend());
    // ... test operations
    audio_system::done();  // Clean up
}

TEST_CASE("independent_test_2") {
    // Should work regardless of test_1's execution
    audio_system::init(create_test_backend());
    // ... test operations
    audio_system::done();  // Clean up
}
```

### Clear Assertions

Use specific, meaningful assertions:

```cpp
// Good: Clear what's being tested
CHECK(stream.volume() == doctest::Approx(0.5f).epsilon(0.001f));

// Better: With custom message
CHECK_MESSAGE(
    stream.is_playing(),
    "Stream should be playing after calling play()"
);

// Best: Multiple related checks with context
SUBCASE("volume_clamping") {
    stream.set_volume(2.0f);
    CHECK(stream.volume() <= 1.0f);
    CHECK(stream.volume() >= 0.0f);
}
```

## Running Tests

### Command Line Options

```bash
# Run all tests
./bin/musac_unittest

# Run specific test suite
./bin/musac_unittest -ts="AudioDevice::*"

# Run with verbose output
./bin/musac_unittest -s

# List all test cases
./bin/musac_unittest -ltc

# Run specific test case
./bin/musac_unittest -tc="*concurrent*"

# Generate XML report for CI
./bin/musac_unittest -r=xml
```

### Debugging Failed Tests

```bash
# Run with debugger
gdb ./bin/musac_unittest
(gdb) run -tc="failing_test_name"

# Run with sanitizers
cmake -DMUSAC_ENABLE_SANITIZERS=ON ..
./bin/musac_unittest

# Run with valgrind
valgrind --leak-check=full ./bin/musac_unittest
```

## Checklist for New Tests

Before submitting new tests, ensure:

- [ ] **Naming**: Follows Component::Category::Type pattern
- [ ] **Documentation**: Has file header and complex test documentation  
- [ ] **Compilation**: Builds without warnings (-Wall -Wextra)
- [ ] **Independence**: Doesn't depend on other tests
- [ ] **Cleanup**: Properly manages all resources (RAII)
- [ ] **Stability**: No random failures or timing dependencies
- [ ] **Performance**: Unit tests < 100ms, integration tests reasonable
- [ ] **Coverage**: Includes positive, negative, and edge cases
- [ ] **Assertions**: Clear, specific assertions with good messages
- [ ] **Mocks**: Uses mock infrastructure for isolation
- [ ] **Thread Safety**: Tests concurrent scenarios where applicable
- [ ] **Error Handling**: Tests error conditions and recovery

## Recent Improvements (2025)

1. **Mock Infrastructure**: Comprehensive mock backends and components
2. **Error Scenarios**: Dedicated error scenario test coverage
3. **Naming Standardization**: All test suites follow Component::Category pattern
4. **Warning-Free**: All tests compile without warnings
5. **Test Organization**: Clear separation of unit vs integration tests
6. **Thread Safety**: Enhanced concurrent testing patterns
7. **Stability**: Problematic tests identified and fixed or skipped with reasons