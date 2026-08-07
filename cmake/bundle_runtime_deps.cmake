# Copy the DLLs that EXECUTABLE actually depends on into OUTPUT_DIR.
# Used by the post-build step and by `cmake --install`.
#
# Inputs:
#   EXECUTABLE     .exe to scan
#   OUTPUT_DIR     destination folder
#   SEARCH_DIRS    ;-separated list of dirs to look in (compiler bin, etc.)
#   STAMP          optional; skip the scan while newer than every TRIGGER_FILES
#   TRIGGER_FILES  optional; ;-separated inputs that invalidate STAMP

if (NOT EXECUTABLE OR NOT OUTPUT_DIR)
    message(FATAL_ERROR "bundle_runtime_deps: EXECUTABLE and OUTPUT_DIR are required")
endif ()

# Skip the ~1s scan below when nothing that can change the DLL set has changed.
# Not keyed on EXECUTABLE: every link rewrites it, but its mtime says nothing
# about whether its imports changed.
if (STAMP AND EXISTS "${STAMP}")
    set(_stale FALSE)
    foreach (_trigger IN LISTS TRIGGER_FILES)
        if ("${_trigger}" IS_NEWER_THAN "${STAMP}")
            set(_stale TRUE)
            break()
        endif ()
    endforeach ()
    if (NOT _stale)
        return()
    endif ()
endif ()

file(GET_RUNTIME_DEPENDENCIES
     RESOLVED_DEPENDENCIES_VAR   _resolved
     UNRESOLVED_DEPENDENCIES_VAR _unresolved
     EXECUTABLES "${EXECUTABLE}"
     DIRECTORIES ${SEARCH_DIRS}
     PRE_EXCLUDE_REGEXES  "api-ms-.*" "ext-ms-.*"
     POST_EXCLUDE_REGEXES ".*[Ss]ystem32/.*\\.[Dd][Ll][Ll]" ".*SysWOW64/.*\\.[Dd][Ll][Ll]"
)

foreach (_dll IN LISTS _resolved)
    file(COPY "${_dll}" DESTINATION "${OUTPUT_DIR}" FOLLOW_SYMLINK_CHAIN)
endforeach ()

if (_unresolved)
    message(STATUS "bundle_runtime_deps: skipped (system) ${_unresolved}")
endif ()

if (STAMP)
    file(TOUCH "${STAMP}")
endif ()
