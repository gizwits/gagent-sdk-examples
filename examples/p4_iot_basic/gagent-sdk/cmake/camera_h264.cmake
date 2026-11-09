# Apply only after IDF has created the selected encoder target.
idf_component_get_property(camera_h264_lib espressif__esp_h264 COMPONENT_LIB)
idf_component_get_property(camera_h264_dir espressif__esp_h264 COMPONENT_DIR)
set(original "${camera_h264_dir}/hw/src/esp_h264_enc_hw_param.c")
set(patched "${CMAKE_BINARY_DIR}/camera/esp_h264_enc_hw_param.c")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/../tools/patch_camera_h264.py" "${original}" "${patched}"
    COMMAND_ERROR_IS_FATAL ANY)
get_target_property(camera_h264_sources ${camera_h264_lib} SOURCES)
list(FIND camera_h264_sources "${original}" original_index)
if(original_index LESS 0)
    message(FATAL_ERROR "H.264 reference source not found in component target")
endif()
list(REMOVE_AT camera_h264_sources ${original_index})
list(APPEND camera_h264_sources "${patched}")
set_property(TARGET ${camera_h264_lib} PROPERTY SOURCES "${camera_h264_sources}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${original}" "${CMAKE_CURRENT_LIST_DIR}/../tools/patch_camera_h264.py")

target_include_directories(${camera_h264_lib} PRIVATE "${camera_h264_dir}/hw/src")
