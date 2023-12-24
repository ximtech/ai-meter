cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(AI-Meter)

#######################################################################
#######################################################################

# Copy sd-car directory to flash drive, use for development
# IMPORTANT! Set flash drive mount root path
set(SD_CARD_PATH E:/)

# if(EXISTS ${SD_CARD_PATH})
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sd-card/db DESTINATION ${SD_CARD_PATH})
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sd-card/html DESTINATION ${SD_CARD_PATH})
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sd-card/log DESTINATION ${SD_CARD_PATH})
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sd-card/photo DESTINATION ${SD_CARD_PATH})

# file(GLOB CONFIG_PROPERTIES ${CMAKE_CURRENT_SOURCE_DIR}/sd-card/*.properties)
# file(COPY ${CONFIG_PROPERTIES} DESTINATION ${SD_CARD_PATH})
# endif()

#######################################################################
#######################################################################