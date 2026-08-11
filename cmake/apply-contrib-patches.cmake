if (NOT Git_FOUND)
    message (FATAL_ERROR "git not found; cannot apply contrib patches.")
endif ()

# Table of patches to apply at configure time.
# Format: "ARCH|cmake/patches/<name>.patch|contrib/<submodule>"
# Use ARCH_ANY to apply on all architectures, or a specific arch variable
# from cmake/arch.cmake (e.g. ARCH_S390X, ARCH_AMD64) to restrict the patch.
set (CONTRIB_PATCHES
    "ARCH_S390X|cmake/patches/llvm-project-s390x.patch|contrib/llvm-project"
)

# Keep in sync with cmake/arch.cmake
set (KNOWN_ARCHES ARCH_ANY ARCH_AMD64 ARCH_AARCH64 ARCH_PPC64LE ARCH_S390X ARCH_RISCV64 ARCH_LOONGARCH64 ARCH_E2K)

foreach (PATCH_ENTRY ${CONTRIB_PATCHES})
    string (REPLACE "|" ";" PATCH_PARTS "${PATCH_ENTRY}")
    list (GET PATCH_PARTS 0 PATCH_ARCH)
    list (GET PATCH_PARTS 1 PATCH_REL)
    list (GET PATCH_PARTS 2 CONTRIB_REL)

    if (PATCH_ARCH STREQUAL "ARCH_ANY")
        # proceed unconditionally
    elseif (NOT PATCH_ARCH IN_LIST KNOWN_ARCHES)
        message (FATAL_ERROR "Unknown arch guard '${PATCH_ARCH}' in CONTRIB_PATCHES entry: ${PATCH_ENTRY}")
    elseif (NOT ${${PATCH_ARCH}})
        message (VERBOSE "Skipping patch ${PATCH_REL} — only for ${PATCH_ARCH}")
        continue ()
    endif ()

    set (PATCH_FILE "${ClickHouse_SOURCE_DIR}/${PATCH_REL}")
    set (WORK_DIR   "${ClickHouse_SOURCE_DIR}/${CONTRIB_REL}")

    # If the patch is already applied, skip silently.
    execute_process (
        COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${PATCH_FILE}"
        WORKING_DIRECTORY "${WORK_DIR}"
        RESULT_VARIABLE ALREADY_APPLIED
        ERROR_QUIET
        OUTPUT_QUIET)

    if (ALREADY_APPLIED EQUAL 0)
        message (STATUS "Patch already applied: ${PATCH_REL} — skipping")
    else ()
        execute_process (
            COMMAND "${GIT_EXECUTABLE}" apply "${PATCH_FILE}"
            WORKING_DIRECTORY "${WORK_DIR}"
            RESULT_VARIABLE APPLY_RESULT
            ERROR_VARIABLE  APPLY_ERROR
            OUTPUT_QUIET)
        if (NOT APPLY_RESULT EQUAL 0)
            message (FATAL_ERROR
                "Failed to apply patch ${PATCH_REL}:\n${APPLY_ERROR}"
                "\nTo recover from a partial application:"
                "\n  cd ${WORK_DIR} && git checkout -- .")
        endif ()
        message (STATUS "Applied patch: ${PATCH_REL}")
    endif ()
endforeach ()
