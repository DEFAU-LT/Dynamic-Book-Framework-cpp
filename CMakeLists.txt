# This file configures the "build" of this SKSE plugin
# - how to compile the C++ plugin (in addition to settings found in CMakePresets.json)
# - where to output the resulting .dll file

# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21) # Adjusted back to 3.21 as it supports target_sources well

# [BEGIN set_project_name]
# Set your project name. This will be the name of your SKSE .dll file.
project(DynamicBookFramework VERSION 0.0.1 LANGUAGES CXX)
# [END set_project_name]

# <<< ADDED: Configure Version Files from Templates >>>
# Define where the template files are located (assuming a 'cmake' subfolder)

# --- Configure Version Files from Templates ---
set(VERSION_TEMPLATE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
if(NOT EXISTS "${VERSION_TEMPLATE_DIR}/version.h.in")
    message(FATAL_ERROR "Version template file not found: ${VERSION_TEMPLATE_DIR}/version.h.in")
endif()
if(NOT EXISTS "${VERSION_TEMPLATE_DIR}/version.rc.in")
    message(FATAL_ERROR "Resource template file not found: ${VERSION_TEMPLATE_DIR}/version.rc.in")
endif()
configure_file(
    "${VERSION_TEMPLATE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/Version.h" # Generated Version.h
    @ONLY
)
configure_file(
    "${VERSION_TEMPLATE_DIR}/version.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
    @ONLY
)
# <<< END ADDED SECTION >>>

#---------------------------------------------------------
# Output Folder Setup (Optional Deployment)
#---------------------------------------------------------
# (Your existing OUTPUT_FOLDER logic remains here unchanged)
# If you're not using a mod manager...
if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()
# If you're using Mod Organizer 2 or Vortex...
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
endif()
# Otherwise, you can set OUTPUT_FOLDER manually...
# set(OUTPUT_FOLDER "C:/path/to/any/folder")
#---------------------------------------------------------

# Automatically find all .cpp files in the src directory
file(GLOB_RECURSE PLUGIN_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# --- Setup SKSE Plugin Target ---
find_package(CommonLibSSE CONFIG REQUIRED)

# Define target and add main source files 
add_commonlibsse_plugin(${PROJECT_NAME} 
    SOURCES 
        ${PLUGIN_SOURCES}               # All .cpp files found in src/
        
        
        # Generated files:
        "${CMAKE_CURRENT_BINARY_DIR}/include/Version.h"
        "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
)


# Update Include Directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/include        # For generated Version.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include        # Main directory for your project's headers
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SKSEMCP 
    #${SKSE_MCP_INCLUDE_DIRS}
)

# <<< CHANGED: Set C++ Standard to 23 (Recommended) >>>
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) 

if (MSVC)
    add_definitions(-D_SILENCE_ALL_CXX23_DEPRECATION_WARNINGS) # Optional
endif()

# Configure Precompiled Header
target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h)  # Ensure PCH.h is in src/ if using that path

#---------------------------------------------------------
# Post-Build Copy Commands (Optional Deployment)
#---------------------------------------------------------
# (Your existing post-build copy logic remains here unchanged)
if(DEFINED OUTPUT_FOLDER)
    # ... (rest of your post-build copy commands) ...
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")
    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")
    add_custom_command( # Copy DLL
        TARGET "${PROJECT_NAME}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug") # Copy PDB
        add_custom_command(
            TARGET "${PROJECT_NAME}" POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()
#---------------------------------------------------------