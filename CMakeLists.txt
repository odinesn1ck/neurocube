cmake_minimum_required(VERSION 3.21)
option(NEUROCUBE_DEBUG_INPUT "Enable synthetic input for hardware protocol tests only" OFF)
option(NEUROCUBE_PICO "Build RP2350 firmware" OFF)
if(NEUROCUBE_PICO)
  set(PICO_BOARD pico2 CACHE STRING "SDK CPU baseline; board pins defined by our HAL")
  set(PICO_FLASH_SIZE_BYTES 16777216 CACHE STRING "Waveshare flash size")
  include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
endif()
project(neurocube VERSION 0.3.3 LANGUAGES C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(neurocube_core
  src/domain/records.cpp
  src/domain/crawl_stories.cpp
  src/application/registry.cpp
  src/application/gestures.cpp
  src/application/idle.cpp
  src/application/app.cpp
  src/games/games.cpp
  src/presentation/framebuffer.cpp
  src/presentation/ui.cpp
  src/presentation/crawl.cpp)
target_include_directories(neurocube_core PUBLIC include)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(neurocube_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(NEUROCUBE_PICO)
  pico_sdk_init()
  # Core0 uses all 4 KiB of SCRATCH_Y; large persistent buffers live in .bss.
  target_compile_options(neurocube_core PRIVATE -fstack-usage)
  find_package(Git QUIET)
  set(NEUROCUBE_REVISION "unknown")
  if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE NEUROCUBE_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  add_executable(neurocube src/infrastructure/board.cpp src/infrastructure/main.cpp src/infrastructure/display_power.cpp)
  target_link_libraries(neurocube PRIVATE neurocube_core pico_stdlib hardware_spi hardware_i2c hardware_adc hardware_pwm hardware_flash pico_flash pico_unique_id pico_rand)
  target_compile_options(neurocube PRIVATE -fstack-usage)
  target_compile_definitions(neurocube PRIVATE PICO_STACK_SIZE=4096 PICO_FLASH_SIZE_BYTES=16777216 PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=0)
  if(NEUROCUBE_DEBUG_INPUT)
    target_compile_definitions(neurocube PRIVATE NEUROCUBE_DEBUG_INPUT=1)
  endif()
  target_compile_definitions(neurocube PRIVATE NEUROCUBE_REVISION="${NEUROCUBE_REVISION}")
  pico_enable_stdio_usb(neurocube 1)
  pico_enable_stdio_uart(neurocube 0)
  # Keep RP2350-E10 workaround away from the records journal.
  set_target_properties(neurocube PROPERTIES PICOTOOL_EXTRA_UF2_ARGS "--abs-block;0x100fff00")
  pico_add_extra_outputs(neurocube)
else()
  enable_testing()
  add_executable(core_tests tests/core_tests.cpp)
  target_link_libraries(core_tests PRIVATE neurocube_core)
  add_test(NAME core_tests COMMAND core_tests)
  add_executable(neurocube_sim src/infrastructure/simulator.cpp)
  target_link_libraries(neurocube_sim PRIVATE neurocube_core)
  if(MINGW)
    target_link_options(core_tests PRIVATE -static)
    target_link_options(neurocube_sim PRIVATE -static)
  endif()
  if(WIN32)
    target_link_libraries(neurocube_sim PRIVATE user32 gdi32)
  endif()
endif()
