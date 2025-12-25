// Main test runner for musac unit tests
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>
#include <cstdlib>

#ifdef _WIN32
#include <stdlib.h>
#define portable_setenv(name, value) _putenv_s(name, value)
#else
#define portable_setenv(name, value) setenv(name, value, 1)
#endif

int main(int argc, char** argv) {
    // Set SDL to use dummy audio driver for testing
    // This avoids needing actual audio hardware and prevents the null backend issues
    portable_setenv("SDL_AUDIODRIVER", "dummy");

    // Also set video driver to dummy to avoid any GUI dependencies
    portable_setenv("SDL_VIDEODRIVER", "dummy");
    
    // Run the tests
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    
    int res = context.run();
    
    if(context.shouldExit()) {
        return res;
    }
    
    return res;
}