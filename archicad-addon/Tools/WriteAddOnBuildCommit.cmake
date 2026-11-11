# Writes OUTPUT_FILE, a header defining ADDON_BUILD_COMMIT: the short hash of the git commit
# of SOURCE_DIR, which the About dialog shows after the version. It is empty when GIT_EXECUTABLE
# is not set or SOURCE_DIR is not a git checkout (a source archive), so the build never fails
# for it. Run on every build:
#   cmake -DGIT_EXECUTABLE=<git> -DSOURCE_DIR=<dir> -DOUTPUT_FILE=<header> -P WriteAddOnBuildCommit.cmake
# The header is only rewritten when the commit changed, so an unchanged one recompiles nothing.

cmake_minimum_required (VERSION 3.17)

set (buildCommit "")
if (GIT_EXECUTABLE)
    execute_process (
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=8 HEAD
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE gitResult
        OUTPUT_VARIABLE gitOutput
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if (gitResult EQUAL 0 AND gitOutput MATCHES "^[0-9a-f]+$")
        set (buildCommit "${gitOutput}")
    endif ()
endif ()

set (header "#pragma once\n\n#define ADDON_BUILD_COMMIT \"${buildCommit}\"\n")
set (oldHeader "")
if (EXISTS "${OUTPUT_FILE}")
    file (READ "${OUTPUT_FILE}" oldHeader)
endif ()
if (NOT header STREQUAL oldHeader)
    file (WRITE "${OUTPUT_FILE}" "${header}")
endif ()
