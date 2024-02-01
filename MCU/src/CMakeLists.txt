# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

##################################################################
##################################################################

execute_process(COMMAND git log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M")

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file from Github instead of a checkout)
if ("${GIT_REV}" STREQUAL "")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
else()
    execute_process(
        COMMAND bash -c "git diff --quiet --exit-code || echo +"
        OUTPUT_VARIABLE GIT_DIFF)
    execute_process(
        COMMAND git describe --exact-match --tags
        OUTPUT_VARIABLE GIT_TAG ERROR_QUIET)
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH)

    string(STRIP "${GIT_REV}" GIT_REV)
    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
    string(STRIP "${GIT_DIFF}" GIT_DIFF)
    string(STRIP "${GIT_TAG}" GIT_TAG)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
endif()

set(VERSION 
"#pragma once
static const char* GIT_REV=\"${GIT_REV}${GIT_DIFF}\";
static const char* GIT_TAG=\"${GIT_TAG}\";
static const char* GIT_BRANCH=\"${GIT_BRANCH}\";
static const char* BUILD_TIME=\"${BUILD_TIME}\";")

if ("${GIT_TAG}" STREQUAL "") # Tag not set, show branch
    set(VERSION_HTML "Development-Branch: ${GIT_BRANCH} (Commit: ${GIT_REV}${GIT_DIFF})\n${GIT_REV}")
else() # Tag is set, ignore branch
    set(VERSION_HTML "Release: ${GIT_TAG} (Commit: ${GIT_REV}${GIT_DIFF})\n${GIT_REV}")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/version.h)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/version.h VERSION_)
else()
    set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/version.h "${VERSION}")
endif()

#######################################################################
#######################################################################

set(COMPONENTS_DIR ../components)

set(SD_CARD_PATH ${COMPONENTS_DIR}/sd-card)
set(LED_PATH ${COMPONENTS_DIR}/led)
set(ESP_INFO_PATH ${COMPONENTS_DIR}/esp)
set(NTP_TIME_PATH ${COMPONENTS_DIR}/time)
set(SERVER_PATH ${COMPONENTS_DIR}/server)
set(CAMERA_PATH ${COMPONENTS_DIR}/camera)
set(CLIENT_PATH ${COMPONENTS_DIR}/client)
set(WIFI_PATH ${COMPONENTS_DIR}/wifi)

set(DIRS_TO_INCLUDE ${SD_CARD_PATH} 
                    ${LED_PATH} 
                    ${ESP_INFO_PATH} 
                    ${NTP_TIME_PATH} 
                    ${SERVER_PATH}
                    ${CAMERA_PATH}
                    ${CLIENT_PATH}
                    ${WIFI_PATH}
                    )

set(SOURCES AppConfig.h
            ${SD_CARD_PATH}/SDCard.c
            ${LED_PATH}/StatusLed.c
            ${ESP_INFO_PATH}/EspConfig.c
            ${ESP_INFO_PATH}/PSRAM.c
            ${ESP_INFO_PATH}/Encryptor.c
            ${NTP_TIME_PATH}/NTPTime.c
            ${SERVER_PATH}/SoftAPServer.c
            ${SERVER_PATH}/ServerUtils.c
            ${CAMERA_PATH}/CameraControl.c
            ${WIFI_PATH}/WifiService.c
            ${CLIENT_PATH}/RestClient.c
            ${CLIENT_PATH}/TelegramApiClient.c
            )

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources} 
                        ${SOURCES} 
                        INCLUDE_DIRS "." ${DIRS_TO_INCLUDE}
                        REQUIRES esp32-camera esp32-idf-sqlite3 esp-nn
                        )