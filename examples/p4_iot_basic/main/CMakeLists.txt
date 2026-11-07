# IDF 依赖扫描在独立 CMake 进程执行，在组件内读取清单以确定可选依赖。
file(READ "${CMAKE_CURRENT_LIST_DIR}/../gagent-sdk/sdk-manifest.json" sdk_manifest)
string(JSON sdk_camera GET "${sdk_manifest}" board camera enabled)
set(camera_deps)
if(sdk_camera)
    list(APPEND camera_deps gagent_camera)
endif()
idf_component_register(SRCS "app_main.cc" "board.cc" "gizwits_product.cc" "gizwits_protocol.cc" "hardware.cc"
    INCLUDE_DIRS "." "../gagent-sdk/board"
    REQUIRES gagent_core gagent_esp gagent_cloud_gizwits gagent_chip_policy esp_app_format esp_hw_support esp_driver_gpio esp_driver_rmt ${camera_deps})
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
