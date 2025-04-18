# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/pl450b/esp/esp-idf/components/bootloader/subproject"
  "/home/pl450b/Projects/superDARN_sensor/adc_avg_test/build/bootloader"
  "/home/pl450b/Projects/superDARN_sensor/adc_avg_test/build/bootloader-prefix"
  "/home/pl450b/Projects/superDARN_sensor/adc_avg_test/build/bootloader-prefix/tmp"
  "/home/pl450b/Projects/superDARN_sensor/adc_avg_test/build/bootloader-prefix/src/bootloader-stamp"
  "/home/pl450b/Projects/superDARN_sensor/adc_avg_test/build/bootloader-prefix/src"
  "/home/pl450b/Projects/superDARN_sensor/adc_avg_test/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/pl450b/Projects/superDARN_sensor/adc_avg_test/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/pl450b/Projects/superDARN_sensor/adc_avg_test/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
