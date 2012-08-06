cmake_minimum_required (VERSION 2.6)
project (gumstix-dsp-spitest)

set (gumstix-dsp-spitest_VERSION_MAJOR 0)
set (gumstix-dsp-spitest_VERSION_MINOR 1)

add_executable(gumstix-dsp-spitest gumstix-dsp-spitest.c)
target_link_libraries(gumstix-dsp-spitest rt)

install (TARGETS gumstix-dsp-spitest DESTINATION bin)
