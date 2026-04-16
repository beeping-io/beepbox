cmake_minimum_required(VERSION 3.25)

project(BeepBox
  VERSION 0.0.0
  DESCRIPTION "C++20 tool for encoding data over sound — CLI + library"
  HOMEPAGE_URL "https://github.com/beeping-io/beepbox"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTING "Build tests" ON)
set(BUILD_SHARED_LIBS OFF)

enable_testing()

# --- Dependencies via FetchContent ---

include(FetchContent)

FetchContent_Declare(
  BeepingCore
  GIT_REPOSITORY https://github.com/beeping-io/beeping-core.git
  GIT_TAG        v0.0.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(BeepingCore)

FetchContent_Declare(
  libebur128
  GIT_REPOSITORY https://github.com/jiixyj/libebur128.git
  GIT_TAG        v1.2.6
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(libebur128)

# --- System dependency: libsndfile ---

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)

# --- libbeepbox (pure logic, no I/O) ---

add_library(beepbox_lib STATIC
  src/Base32.cpp
  src/Params.cpp
  src/Scheduler.cpp
  src/Generator.cpp
  src/Mixer.cpp
  src/VRand.cpp
)

target_include_directories(beepbox_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(beepbox_lib PUBLIC BeepingCore ebur128)
target_compile_features(beepbox_lib PUBLIC cxx_std_20)

# --- BeepBox CLI (thin wrapper with file I/O) ---

add_executable(BeepBox
  src/main.cpp
  src/LoudnessStats.cpp
)

target_include_directories(BeepBox PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${SNDFILE_INCLUDE_DIRS}
  ${libebur128_SOURCE_DIR}/ebur128
)

target_link_directories(BeepBox PRIVATE ${SNDFILE_LIBRARY_DIRS})
target_link_libraries(BeepBox PRIVATE
  beepbox_lib
  ${SNDFILE_LIBRARIES}
)

# --- Install ---

include(GNUInstallDirs)

install(TARGETS beepbox_lib BeepBox
  EXPORT BeepBoxTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/beepbox/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/beepbox
  FILES_MATCHING PATTERN "*.h"
)

# --- Tests ---

if(BUILD_TESTING)
  add_test(NAME BeepBox_GenerateBeeps
           COMMAND BeepBox -k 00001 -o ${CMAKE_BINARY_DIR}/test_gen.wav -d 2.3 -m inaudible)

  add_test(NAME BeepBox_DryRun
           COMMAND BeepBox -k 00001 -o ${CMAKE_BINARY_DIR}/test_dry.wav -n)

  add_test(NAME BeepBox_TooShortFails
           COMMAND BeepBox -k 00001 -o ${CMAKE_BINARY_DIR}/test_fail.wav -d 1.0)
  set_tests_properties(BeepBox_TooShortFails PROPERTIES WILL_FAIL TRUE)
endif()
