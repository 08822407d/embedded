# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7")
  file(MAKE_DIRECTORY "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7")
endif()
file(MAKE_DIRECTORY
  "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7/build"
  "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7"
  "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7/tmp"
  "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7/src/GFX02Z1_test_CM7-stamp"
  "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7/src"
  "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7/src/GFX02Z1_test_CM7-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7/src/GFX02Z1_test_CM7-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/cheyh/projs/embedded/STM32/Nucleo-H755ZIQ/GFX02Z1_test/CM7/src/GFX02Z1_test_CM7-stamp${cfgdir}") # cfgdir has leading slash
endif()
