cmake_minimum_required(VERSION 3.20)
# 首个例程使用 P4；C61 无线芯片由 SDK 的 Hosted 组件管理。
set(IDF_TARGET esp32p4)
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/gagent-sdk/sdkconfig.defaults;${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")
# 导入静态库、锁定依赖及兼容性检查，不需要内部固件仓库源码。
include("${CMAKE_CURRENT_LIST_DIR}/gagent-sdk/gagent-sdk.cmake")
set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(gagent_soc_basic)
gagent_sdk_check()
