# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-src")
  file(MAKE_DIRECTORY "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-src")
endif()
file(MAKE_DIRECTORY
  "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-build"
  "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-subbuild/bullet-populate-prefix"
  "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-subbuild/bullet-populate-prefix/tmp"
  "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-subbuild/bullet-populate-prefix/src/bullet-populate-stamp"
  "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-subbuild/bullet-populate-prefix/src"
  "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-subbuild/bullet-populate-prefix/src/bullet-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-subbuild/bullet-populate-prefix/src/bullet-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/jonathanst-georges/CLionProjects/Physics-Engine/cmake-build-debug/_deps/bullet-subbuild/bullet-populate-prefix/src/bullet-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
