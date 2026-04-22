cmake_minimum_required(VERSION 3.25)

project(BeepBox
  VERSION 0.0.0 # x-release-please-version
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

# --- beeping-core (dual-mode: Conan find_package or FetchContent fallback) ---

option(BEEPBOX_USE_EXTERNAL_BEEPING_CORE
  "Resolve beeping-core via find_package() instead of FetchContent"
  OFF)

# Auto-enable when a Conan toolchain is active
if(DEFINED CMAKE_TOOLCHAIN_FILE AND
   CMAKE_TOOLCHAIN_FILE MATCHES "conan_toolchain\\.cmake$")
  set(BEEPBOX_USE_EXTERNAL_BEEPING_CORE ON CACHE BOOL "" FORCE)
endif()

include(FetchContent)

if(BEEPBOX_USE_EXTERNAL_BEEPING_CORE)
  find_package(BeepingCore REQUIRED CONFIG)
else()
  FetchContent_Declare(
    BeepingCore
    GIT_REPOSITORY https://github.com/beeping-io/beeping-core.git
    GIT_TAG        v0.0.0
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(BeepingCore)
endif()

# --- libebur128 (FetchContent always — not on ConanCenter) ---

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
  src/WavWriter.cpp
  src/WavReader.cpp
)

target_include_directories(beepbox_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if(BEEPBOX_USE_EXTERNAL_BEEPING_CORE)
  target_link_libraries(beepbox_lib PUBLIC BeepingCore::BeepingCore ebur128)
else()
  target_link_libraries(beepbox_lib PUBLIC BeepingCore ebur128)
endif()
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

# --- Drogon HTTP framework ---

option(BEEPBOX_BUILD_SERVER "Build the beepbox-server HTTP executable" ON)

if(BEEPBOX_BUILD_SERVER)
  # --- Build metadata (git SHA + timestamp) ---
  execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE BEEPBOX_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(NOT BEEPBOX_GIT_SHA)
    set(BEEPBOX_GIT_SHA "unknown")
  endif()
  string(TIMESTAMP BEEPBOX_BUILD_TIME "%Y-%m-%dT%H:%M:%SZ" UTC)

  # Drogon minimal: no ORM, no examples, no tests
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_CTL OFF CACHE BOOL "" FORCE)
  set(BUILD_ORM OFF CACHE BOOL "" FORCE)
  set(BUILD_DROGON_SHARED OFF CACHE BOOL "" FORCE)
  set(BUILD_DOC OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    drogon
    GIT_REPOSITORY https://github.com/drogonframework/drogon.git
    GIT_TAG        v1.9.8
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(drogon)

  # cpp-httplib for outgoing HTTPS (HttpKeyStore validator). Header-only,
  # uses OpenSSL which Drogon already pulls in.
  FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.3
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(cpp-httplib)

  find_package(OpenSSL REQUIRED)

  add_executable(beepbox-server
    src/server_main.cpp
    src/ApiKeyAuth.cpp
    src/HttpKeyStoreFactory.cpp
    src/RateLimiter.cpp
    src/Metrics.cpp
    src/Tracing.cpp
  )

  target_include_directories(beepbox-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_compile_definitions(beepbox-server PRIVATE
    BEEPBOX_GIT_SHA="${BEEPBOX_GIT_SHA}"
    BEEPBOX_BUILD_TIME="${BEEPBOX_BUILD_TIME}"
    BEEPBOX_VERSION="${PROJECT_VERSION}"
  )

  target_link_libraries(beepbox-server PRIVATE
    beepbox_lib
    drogon
    httplib::httplib
    OpenSSL::SSL
    OpenSSL::Crypto
  )
endif()

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

# --- Sanitizers ---

option(BEEPBOX_ENABLE_TSAN "Build with ThreadSanitizer (Clang/GCC only)" OFF)

if(BEEPBOX_ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -g)
  add_link_options(-fsanitize=thread)
endif()

# --- Tests ---

if(BUILD_TESTING)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(BeepBoxTests
    tests/Base32Tests.cpp
    tests/ParamsTests.cpp
    tests/SchedulerTests.cpp
    tests/GeneratorTests.cpp
    tests/RoundTripTests.cpp
    tests/ConcurrencyTests.cpp
    tests/ApiKeyAuthTests.cpp
    tests/HttpKeyStoreTests.cpp
    tests/RateLimiterTests.cpp
    tests/MetricsTests.cpp
    tests/TracingTests.cpp
    src/ApiKeyAuth.cpp
    src/RateLimiter.cpp
    src/Metrics.cpp
    src/Tracing.cpp
  )

  target_link_libraries(BeepBoxTests PRIVATE beepbox_lib Catch2::Catch2WithMain)
  target_include_directories(BeepBoxTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  # --- Server endpoint integration tests (require beepbox-server running) ---
  if(BEEPBOX_BUILD_SERVER)
    add_executable(ServerEndpointTests
      tests/ServerEndpointTests.cpp
    )
    target_link_libraries(ServerEndpointTests PRIVATE Catch2::Catch2WithMain)
    target_include_directories(ServerEndpointTests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    # These tests require a running server — tagged [server] for selective runs
    catch_discover_tests(ServerEndpointTests)
  endif()

  if(NOT WIN32)
    target_link_libraries(BeepBoxTests PRIVATE pthread)
  endif()

  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
  include(Catch)
  catch_discover_tests(BeepBoxTests)

  # CLI smoke tests
  add_test(NAME CLI_GenerateBeeps
           COMMAND BeepBox -k 00001 -o ${CMAKE_BINARY_DIR}/test_gen.wav -d 2.3 -m inaudible)

  add_test(NAME CLI_DryRun
           COMMAND BeepBox -k 00001 -o ${CMAKE_BINARY_DIR}/test_dry.wav -n)

  # Graceful shutdown test
  if(BEEPBOX_BUILD_SERVER)
    add_test(NAME Server_GracefulShutdown
             COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/shutdown_test.sh)
    set_tests_properties(Server_GracefulShutdown PROPERTIES TIMEOUT 20)
  endif()

  add_test(NAME CLI_TooShortFails
           COMMAND BeepBox -k 00001 -o ${CMAKE_BINARY_DIR}/test_fail.wav -d 1.0)
  set_tests_properties(CLI_TooShortFails PROPERTIES WILL_FAIL TRUE)
endif()
