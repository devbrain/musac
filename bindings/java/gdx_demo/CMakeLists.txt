cmake_minimum_required(VERSION 3.10)
project(musac_gdx_demo VERSION 1.0.0)

# Find Java
find_package(Java REQUIRED)
include(UseJava)

# Set Java version
set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.8" "-target" "1.8")

# ============================================================================
# MUSAC JAVA BINDINGS
# ============================================================================
message(STATUS "Building Musac Java bindings...")

# Collect all Java sources
file(GLOB_RECURSE MUSAC_JAVA_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/com/musac/*.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/com/musac/decoders/*.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/com/musac/gdx/*.java"
)

# Create JAR using UseJava module
add_jar(musac_core
    SOURCES ${MUSAC_JAVA_SOURCES}
    OUTPUT_NAME musac-core-1.0.0
    OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core/build/libs
)

# ============================================================================
# GRADLE WRAPPER
# ============================================================================
function(gradle_build TARGET_NAME GRADLE_TASK)
    add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gradlew ${GRADLE_TASK}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building ${TARGET_NAME} with Gradle..."
    )
endfunction()

# ============================================================================
# DESKTOP BUILD
# ============================================================================
message(STATUS "Configuring Desktop build...")

# Desktop JAR build
gradle_build(desktop_jar "lwjgl3:jar")
add_dependencies(desktop_jar musac_core)

# Desktop run
add_custom_target(run_desktop
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gradlew lwjgl3:run
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS desktop_jar
    COMMENT "Running desktop version..."
)

message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  musac_core    - Build Musac Java bindings JAR")
message(STATUS "  desktop_jar   - Build desktop JAR")
message(STATUS "  run_desktop   - Run desktop version")
message(STATUS "")