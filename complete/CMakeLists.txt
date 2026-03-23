# Mandatory line, sets the minimum version of CMake that should be used with this repository.
# 3.22 is the minimum CMake version required by JUCE.
# 3.25 is required for the MSVC_DEBUG_INFORMATION_FORMAT feature to work.
# To verify your version run
# $ cmake --version
cmake_minimum_required(VERSION 4.3.0 FATAL_ERROR)

# Enable setting CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)

# Sets the minimum deployment target*
# macOS 10.11 is the minimum required by JUCE
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "" FORCE)
# Builds a binary that works on Apple Silicon (arm64) and Intel-based Macs
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)

# Sets a few variables, like PROJECT_NAME.
project(JucePluginDevelopmentCourseTremoloPlugin VERSION 8.0.12 LANGUAGES CXX)

# 设置JUCE为静态链接模式
set(JUCE_SHARED_CODE OFF CACHE BOOL "Use static linking for JUCE" FORCE)

# Set the C++ standard only if this project is standalone;
# otherwise, let clients decide.
if(PROJECT_IS_TOP_LEVEL)
  if (NOT CMAKE_CXX_STANDARD)
    # Always use the newest C++ standard on green-field projects if possible.
    set(CMAKE_CXX_STANDARD 23)
  endif()

  # Links to C++ runtime statically
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # Embeds debug information
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>" CACHE STRING "" FORCE)
  
  # Add UTF-8 encoding support for Chinese comments
  if(MSVC)
    # For MSVC compiler
    add_compile_options("/utf-8")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  else()
    # For GCC/Clang compilers
    add_compile_options(
      "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-finput-charset=UTF-8>"
      "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fexec-charset=UTF-8>"
    )
  endif()
endif()

# I like to download the dependencies to the same folder as the project.
# If you want to install them system-wide, set CPM_SOURCE_CACHE with a path to the dependencies
# either as an environment variable or pass it with -DCPM_SOURCE_CACHE=<path> when invoking cmake.
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# Downloads CPM if not already downloaded. CPM is an easy-to-use package manager nicely integrated with CMake.
include(cmake/cpm.cmake)

# This commands downloads AND configures JUCE. It sets up some variables, like JUCE_SOURCE_DIR.
cpmaddpackage(
  NAME
    JUCE
  GIT_TAG
    8.0.12
  GITHUB_REPOSITORY
    juce-framework/JUCE
  SOURCE_DIR
    ${LIB_DIR}/juce
)

# Adds compiler warning utilities & other.
include(cmake/CompilerWarnings.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Sets the Windows VST3 installation location to one
  # that doesn't require elevated permissions
  set_directory_properties(PROPERTIES JUCE_VST3_COPY_DIR "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
endif()

# Adds a plugin target.
set(TREMOLO_PLUGIN_NAME "PlugIntoMix")
juce_add_plugin(
  TremoloCoursePlugin
  PRODUCT_NAME
    ${TREMOLO_PLUGIN_NAME}
  VERSION
    1.0.0 # 0 forces AU hosts to always refresh
  COMPANY_NAME
    "IIsaac_music"
  PLUGIN_MANUFACTURER_CODE
    Wfsd
  PLUGIN_CODE
    Trcp
  FORMATS
    Standalone
    AU
    VST3
  IS_SYNTH
    FALSE
  NEEDS_MIDI_INPUT
    TRUE
  NEEDS_MIDI_OUTPUT
    FALSE
  VST3_AUTO_MANIFEST
    FALSE
  COPY_PLUGIN_AFTER_BUILD
    FALSE
)

# Configures the JUCE sources included in the plugin target.
target_compile_definitions(TremoloCoursePlugin
  PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_add_binary_data(tremolo_plugin_assets
  HEADER_NAME
    "TremoloPluginAssets.h"
  NAMESPACE
    tremolo::assets
  SOURCES
    assets/Background.png
    assets/Logo.png
    assets/setting.png
    assets/BT/bt.png
    assets/BT/jj.png
    assets/BT/tone.png
    assets/HCR/001.png
    assets/HCR/HCR_pnglist.png
    assets/HCR/tone.png
    assets/Inter-Bold.ttf
    assets/Inter-Medium.ttf
)

add_library(tremolo::tremolo_plugin_assets ALIAS tremolo_plugin_assets)

juce_add_binary_data(tremolo_plugin_assets_gggg
  HEADER_NAME
    "TremoloPluginAssetsGGGG.h"
  NAMESPACE
    tremolo::assets_gggg
  SOURCES
    assets/GGGG/GGGG_pnglist.png
    assets/GGGG/tone.png
)

add_library(tremolo::tremolo_plugin_assets_gggg ALIAS tremolo_plugin_assets_gggg)

juce_add_binary_data(tremolo_plugin_assets_wb
  HEADER_NAME
    "TremoloPluginAssetsWB.h"
  NAMESPACE
    tremolo::assets_wb
  SOURCES
    assets/WB/WB_pnglist.png
    assets/WB/tone.png
)

add_library(tremolo::tremolo_plugin_assets_wb ALIAS tremolo_plugin_assets_wb)

juce_add_binary_data(tremolo_plugin_assets_ds
  HEADER_NAME
    "TremoloPluginAssetsDS.h"
  NAMESPACE
    tremolo::assets_ds
  SOURCES
    assets/DS/DS_pnglist.png
    assets/DS/tone.png
)

add_library(tremolo::tremolo_plugin_assets_ds ALIAS tremolo_plugin_assets_ds)

juce_add_binary_data(tremolo_plugin_assets_zsz
  HEADER_NAME
    "TremoloPluginAssetsZSZ.h"
  NAMESPACE
    tremolo::assets_zsz
  SOURCES
    assets/ZSZ/ZSZ_pnglist.png
    assets/ZSZ/tone.png
)

add_library(tremolo::tremolo_plugin_assets_zsz ALIAS tremolo_plugin_assets_zsz)

juce_add_module(tremolo_plugin ALIAS_NAMESPACE tremolo)

  # Additional definitions required by all users of this module.
  target_compile_definitions(tremolo_plugin
    INTERFACE
      TREMOLO_PLUGIN_NAME="${TREMOLO_PLUGIN_NAME}"
  )

  # Disable JUCE assertions in debug mode to avoid breakpoint exceptions
  target_compile_definitions(TremoloCoursePlugin PRIVATE
    $<$<CONFIG:Debug>:JUCE_DISABLE_ASSERTIONS=1>
    $<$<CONFIG:Debug>:JUCE_DISABLE_CAUTIOUS_PARAMETER_ID_CHECKING=1>
  )

# Enables strict C++ warnings.
target_compile_options(tremolo_plugin INTERFACE "${PROJECT_WARNINGS_CXX}")

# Add UTF-8 encoding support for Chinese comments
if(MSVC)
  target_compile_options(tremolo_plugin INTERFACE "/utf-8")
else()
  target_compile_options(tremolo_plugin INTERFACE "-finput-charset=UTF-8" "-fexec-charset=UTF-8")
endif()

target_link_libraries(tremolo_plugin INTERFACE tremolo::tremolo_plugin_assets)
target_link_libraries(tremolo_plugin INTERFACE tremolo::tremolo_plugin_assets_gggg)
target_link_libraries(tremolo_plugin INTERFACE tremolo::tremolo_plugin_assets_wb)
target_link_libraries(tremolo_plugin INTERFACE tremolo::tremolo_plugin_assets_ds)
target_link_libraries(tremolo_plugin INTERFACE tremolo::tremolo_plugin_assets_zsz)

target_link_libraries(
  TremoloCoursePlugin
  PRIVATE
    tremolo::tremolo_plugin
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Zip the .vst3 bundle with the README.txt file
get_target_property(VST3_BUNDLE_PATH TremoloCoursePlugin_VST3 JUCE_PLUGIN_ARTEFACT_FILE)
cmake_path(GET VST3_BUNDLE_PATH PARENT_PATH VST3_BUNDLE_OUTPUT_DIR)
add_custom_command(
  TARGET TremoloCoursePlugin_VST3
  POST_BUILD
  WORKING_DIRECTORY ${VST3_BUNDLE_OUTPUT_DIR}
  # For some reason, if the README.txt is in a different directory, zipping it fails, thus, we need to copy it first
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/assets/README.txt" ${VST3_BUNDLE_OUTPUT_DIR}
  COMMAND ${CMAKE_COMMAND} -E tar cvf "${VST3_BUNDLE_OUTPUT_DIR}/Tremolo.zip" --format=zip -- "${VST3_BUNDLE_PATH}" README.txt
)

# In Visual Studio this command provides a nice grouping of source files in "filters".
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)

option(BUILD_EXAMPLES "When on, the supplied example apps will be added as targets and built" OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(BUILD_TESTS "When on, will download the GoogleTest framework and add the automated tests project" OFF)
if(BUILD_TESTS)
  # This command allows running tests from the "build" folder (the one where CMake generates the project to).
  enable_testing()

  # Adds all the targets configured in the "test" folder.
  add_subdirectory(test)

  # Add UTF-8 encoding support for Chinese comments
  if(MSVC)
    # For MSVC compiler
    add_compile_options("/utf-8")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  else()
    # For GCC/Clang compilers
    add_compile_options(
      "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-finput-charset=UTF-8>"
      "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fexec-charset=UTF-8>"
    )
  endif()
endif()