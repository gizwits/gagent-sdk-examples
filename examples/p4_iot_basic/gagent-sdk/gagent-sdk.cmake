# Include before ESP-IDF project.cmake; call gagent_sdk_check() after project().
set(GAGENT_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}")
file(READ "${GAGENT_SDK_DIR}/sdk-manifest.json" sdk_manifest)
string(JSON GAGENT_SDK_CAMERA GET "${sdk_manifest}" board camera enabled)
if(GAGENT_SDK_CAMERA)
    if(CMAKE_VERSION VERSION_LESS 3.24)
        message(FATAL_ERROR "Camera SDK requires CMake 3.24 or newer for static library groups")
    endif()
    # ESP-IDF uses Generic, where CMake does not predefine GNU RESCAN.
    set(CMAKE_LINK_GROUP_USING_gagent_rescan "LINKER:--start-group" "LINKER:--end-group")
    set(CMAKE_LINK_GROUP_USING_gagent_rescan_SUPPORTED TRUE)
endif()
list(APPEND EXTRA_COMPONENT_DIRS "${GAGENT_SDK_DIR}/components")
# Apply the same revision policy to both application and bootloader.
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bootloader_components")
file(COPY "${GAGENT_SDK_DIR}/components/gagent_chip_policy"
    DESTINATION "${CMAKE_SOURCE_DIR}/bootloader_components")
function(gagent_sdk_check)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    if(GAGENT_SDK_CAMERA)
        include("${GAGENT_SDK_DIR}/cmake/camera_h264.cmake")
    endif()
    set(board_args)
    if(EXISTS "${CMAKE_SOURCE_DIR}/board.json")
        list(APPEND board_args --board "${CMAKE_SOURCE_DIR}/board.json")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/board.json")
    endif()
    execute_process(COMMAND "${Python3_EXECUTABLE}" -B "${GAGENT_SDK_DIR}/check.py"
        --sdk "${GAGENT_SDK_DIR}" --sdkconfig "${SDKCONFIG}"
        --idf "$ENV{IDF_PATH}" --compiler "${CMAKE_CXX_COMPILER}" --target "${IDF_TARGET}" ${board_args}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY)
    add_custom_target(gagent_sdk_image_check ALL
        COMMAND "${Python3_EXECUTABLE}" -B "${GAGENT_SDK_DIR}/check.py"
            --sdk "${GAGENT_SDK_DIR}" --sdkconfig "${SDKCONFIG}"
            --idf "$ENV{IDF_PATH}" --compiler "${CMAKE_CXX_COMPILER}" --target "${IDF_TARGET}" ${board_args}
            --app-bin "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    add_dependencies(gagent_sdk_image_check gen_project_binary)
endfunction()
