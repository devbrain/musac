// Main test runner for musac unit tests
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// Custom doctest configuration
namespace doctest {
    struct TestRunnerConfig {
        TestRunnerConfig() {
            // Set default options
            // Can be overridden by command line arguments
        }
    };
}

// You can add global test fixtures or setup here if needed