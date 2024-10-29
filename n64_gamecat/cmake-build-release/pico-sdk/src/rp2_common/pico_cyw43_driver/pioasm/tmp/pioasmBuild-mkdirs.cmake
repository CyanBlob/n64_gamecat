# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/andrew/Development/pico-sdk/tools/pioasm"
  "/Users/andrew/Development/n64_gamecat/n64_gamecat/cmake-build-release/pioasm"
  "/Users/andrew/Development/n64_gamecat/n64_gamecat/cmake-build-release/pioasm-install"
  "/Users/andrew/Development/n64_gamecat/n64_gamecat/cmake-build-release/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/tmp"
  "/Users/andrew/Development/n64_gamecat/n64_gamecat/cmake-build-release/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp"
  "/Users/andrew/Development/n64_gamecat/n64_gamecat/cmake-build-release/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src"
  "/Users/andrew/Development/n64_gamecat/n64_gamecat/cmake-build-release/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/andrew/Development/n64_gamecat/n64_gamecat/cmake-build-release/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/andrew/Development/n64_gamecat/n64_gamecat/cmake-build-release/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
