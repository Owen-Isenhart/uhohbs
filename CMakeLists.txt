cmake_minimum_required(VERSION 3.28...3.30)

# Pull project metadata (name, version) from buildspec.json
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

# Options for feature toggling
option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)
option(ENABLE_TESTS "Build unit tests" ON)

include(compilerconfig)
include(defaults)
include(helpers)

# Define the plugin library
add_library(${CMAKE_PROJECT_NAME} MODULE)

# --- Core Dependencies ---
find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# --- Frontend API ---
if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

# --- Qt6 Configuration ---
if(ENABLE_QT)
  # Find the required Qt6 components
  find_package(Qt6 COMPONENTS Widgets Core REQUIRED)

  # Link the libraries
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)

  # Enable automatic processing of Qt macros (MOC, UIC, RCC)
  # This MUST be set so the compiler can handle Q_OBJECT
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )

  # Suppress common Qt-related compiler warnings if using Clang
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
endif()

# --- Sources ---
# Ensure your main entry point is included here
target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp src/dump.cpp src/dump_dock.cpp)

# --- Plugin Finalization ---
# This helper function sets the output name to 'uhohbs'
# and handles the .so suffix and metadata
set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(ENABLE_TESTS)
  enable_testing()

  add_executable(uhohbs_tests)
  target_sources(uhohbs_tests PRIVATE src/dump.cpp tests/test_dump.cpp)
  target_include_directories(uhohbs_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_link_libraries(uhohbs_tests PRIVATE OBS::libobs)

  if(ENABLE_FRONTEND_API)
    target_link_libraries(uhohbs_tests PRIVATE OBS::obs-frontend-api)
  endif()

  if(TARGET plugin-support)
    target_link_libraries(uhohbs_tests PRIVATE plugin-support)
  endif()

  add_test(NAME uhohbs_tests COMMAND uhohbs_tests)

  if(WIN32)
    set_property(
      TEST uhohbs_tests
      APPEND
      PROPERTY
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<JOIN:$<TARGET_RUNTIME_DLL_DIRS:uhohbs_tests>,$<SEMICOLON>>"
    )
  endif()
endif()
