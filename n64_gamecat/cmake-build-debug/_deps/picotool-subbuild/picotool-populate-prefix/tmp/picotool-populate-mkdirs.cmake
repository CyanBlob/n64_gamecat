# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-src"
  "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-build"
  "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-subbuild/picotool-populate-prefix"
  "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-subbuild/picotool-populate-prefix/tmp"
  "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp"
  "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-subbuild/picotool-populate-prefix/src"
  "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/andrew/Development/n64_gamecat_src/n64_gamecat/cmake-build-debug/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
