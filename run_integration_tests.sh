#!/bin/bash

# Run integration tests with various sanitizers and configurations

set -e

echo "=== Running Phase 5 Integration Tests ==="
echo

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to run tests
run_test() {
    local name=$1
    local binary=$2
    local filter=$3
    
    echo -e "${YELLOW}Running $name...${NC}"
    if $binary --test-case-exclude="*" --test-case="$filter" --duration --no-breaks; then
        echo -e "${GREEN}✓ $name passed${NC}\n"
        return 0
    else
        echo -e "${RED}✗ $name failed${NC}\n"
        return 1
    fi
}

# Check if debug build exists
if [ ! -f "cmake-build-debug/bin/musac_unittest" ]; then
    echo "Debug build not found. Building..."
    ninja -C cmake-build-debug musac_unittest
fi

# Run regular debug build
echo "=== Standard Debug Build ==="
run_test "Phase 5 Integration Tests" \
    "cmake-build-debug/bin/musac_unittest" \
    "*phase5_integration*"

# Check if ThreadSanitizer build exists
if [ -d "cmake-build-tsan" ]; then
    echo "=== ThreadSanitizer Build ==="
    if [ ! -f "cmake-build-tsan/bin/musac_unittest" ]; then
        echo "Building with ThreadSanitizer..."
        cmake -S . -B cmake-build-tsan -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1" \
            -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
        ninja -C cmake-build-tsan musac_unittest
    fi
    
    echo "Running with ThreadSanitizer..."
    export TSAN_OPTIONS="halt_on_error=0:history_size=7:second_deadlock_stack=1"
    run_test "ThreadSanitizer Tests" \
        "cmake-build-tsan/bin/musac_unittest" \
        "*phase5_integration*"
else
    echo -e "${YELLOW}Skipping ThreadSanitizer tests (build directory not found)${NC}"
fi

# Run specific test cases
echo "=== Individual Test Cases ==="

test_cases=(
    "real world usage pattern - music player"
    "stress test - concurrent operations"
    "edge case - rapid init/done cycles with active streams"
    "callback synchronization - complex scenario"
    "memory stress - create/destroy thousands of streams"
    "shutdown order combinations"
)

for test in "${test_cases[@]}"; do
    run_test "$test" \
        "cmake-build-debug/bin/musac_unittest" \
        "*$test*" || true
done

# Generate summary
echo
echo "=== Test Summary ==="
echo "All integration tests completed."
echo "Check the output above for any failures or warnings."

# Run a quick stress test
echo
echo "=== Quick Stress Test ==="
echo "Running stress test for 30 seconds..."
timeout 30s cmake-build-debug/bin/musac_unittest \
    --test-case="*stress test - concurrent operations*" \
    --no-breaks || true

echo
echo "Integration testing complete!"