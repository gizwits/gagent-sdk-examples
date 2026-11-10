# GAgent 二进制 SDK

本包包含 gagent_core / gagent_esp、选定的 gagent_cloud_gizwits 或 gagent_cloud_aliyun 静态库与公开头文件，以及按 board 启用的 aispeech／gagent_camera 库。P4 v3 包另外提供按从机型号选择的 gagent_wireless / gagent_p4 静态库；这些库仍运行在 P4 上。
SDK 仅包含 GAgent 核心组件。mDNS、JSON、MQTT、Hosted 和媒体 Registry 组件不随包提供，第三方版本记录在 sdk-manifest.json 和 example-dependencies.yml 中；样例将它们声明到 main/idf_component.yml，并提交 Component Manager 生成的 dependencies.lock。首次构建需联网，不要关闭 Component Manager。
不包含核心业务实现源码。客户例程在独立 `gagent-sdk-examples` 仓库维护；本包提供离线数据点生成器，接口是当前 C++17 API，不兼容旧 libgagent.a 的 ABI。

SDK 使用 `tools/package_sdk.py --build-dir <已完成构建目录> --output <新目录> --verify` 单独导出并验证。`build_firmware.py --package` 只生成固件与校验材料，不附带 SDK。

## 接入 ESP-IDF 工程

1. 使用 sdk-manifest.json 指定的 ESP-IDF 提交、工具链版本和芯片目标（以清单为准，包含 P4）。摄像头包使用 CMake 3.24 或更新版本的静态库分组功能。
2. 将本包放在工程的 gagent-sdk 目录，将 partitions.csv 复制到工程根目录。
   启用自定义根证书的 board 同时携带 `certs/custom-bundle.pem`，默认配置使用工程内的相对路径，校验脚本会检查实际证书内容。
3. 在根 CMakeLists.txt 的 project.cmake 引入之前，加入：

```cmake
set(IDF_TARGET esp32c3)
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/gagent-sdk/sdkconfig.defaults")
include("${CMAKE_CURRENT_LIST_DIR}/gagent-sdk/gagent-sdk.cmake")
```

4. 在 `project(...)` 后调用 `gagent_sdk_check()`。使用新构建目录，避免已有 sdkconfig 覆盖默认配置。
5. 应用组件声明 `REQUIRES gagent_core gagent_esp gagent_cloud_gizwits`（阿里云包改用 `gagent_cloud_aliyun`）；需要语音时再加 `aispeech`。
   应用使用 C++17、关闭异常和 RTTI；这些选项也由导入组件传播。
6. 应用自行实现 app_main 和启动流程，调用公开的 EspApplication 等接口。
   board/ 下的生成头文件提供本包构建时的参数参考，不包含 app_main 或自动启动代码。
   将 board/ 加入 INCLUDE_DIRS，包含 `selected_cloud.hpp`，创建生命周期覆盖应用的 `firmware::SelectedCloud`，
   将 `&provider.provider()` 赋给 `platform::ApplicationConfig::application.provider`。
   阿里云实例、区域和凭据默认值通过构造时的 `firmware::CloudOptions` 传入。

SDK 按 board、云平台和容量配置编译。不要混用两家云的二进制组件或覆盖导出的 `GAGENT_MQTT_*` 容量定义；共用 MCU 库已经包含两种握手解析器。更换主芯片、IDF、编译器、库依赖的 SDK 配置或编译进库的模组版本，需要重新生成 SDK。P4 v3 包可直接选择清单中已提供的无线从机变体，无需重新生成公共库。客户 PK/PS 和数据点属于应用，修改它们无需重新生成库。
检查脚本核验文件哈希、工具链版本、IDF 提交和库依赖的配置键。v2 导出包允许客户修改 Flash 容量和分区布局，校验必要的存储分区、双 OTA 槽及最终镜像容量；2MB 精简包仍不支持。
不要把已有 IDF 源码版同名组件和本包同时加入工程。


C5 工程需保留随包提供的 `gagent_chip_policy` 组件，并同时用于应用和 bootloader，确保最低芯片修订版一致。导入脚本会自动复制 bootloader 配置组件，不要单独降低任一镜像的最低修订版。

## P4 独立例程

从独立 `gagent-sdk-examples` 仓库选择例程，把 SDK 放到工程的 `gagent-sdk/`。SDK 包不携带例程源码。随包含 P4 平台库，ESP-Hosted 由样例声明下载；基础包不包含摄像头和媒体依赖；摄像头包增加预编译 gagent_camera；经过适配的 WebRTC、Peer、渲染和平台抽象代码作为该组件内部的私有静态库提供，不增加独立源码组件；其余媒体 Registry 依赖由样例声明下载，当前仅支持不带识别模型的推流。C5/C6/C61 从机须安装与主机匹配的 ESP-Hosted 固件（含所需 Wi-Fi/BLE 功能），不能使用普通应用固件替代。

按示例 README 构建和测试。示例内置经授权的测试产品 PK/PS，使用 BLE V2 配网，提供联网收发、GPIO35 按键和 GPIO51 指示灯。摄像头包将 `start_stream` 接到实际推流；基础包拒绝启动请求。其余喂食/定时数据点仅为内存示范。例程文件可编辑，SDK 原始包内文件保持不变，以免触发哈希校验。

`gagent-sdk.cmake` 自动复制芯片修订策略到客户 bootloader 组件目录。P4 的 Flash/分区默认为测试板的 16MB 布局；这不是库的容量限制。v2 包的 Hosted 接线仍须匹配库清单；v3 包按下述规则选择无线库和校验板级配置。

## P4 v3 无线二进制变体

目录布局：

```text
components/gagent_core/             公共协议库
components/gagent_esp/              公共 ESP 平台库、公开头文件
components/gagent_camera/           公共相机库及媒体支持库
components/gagent_wireless/         无线库导入组件、公开头文件
components/gagent_p4/               Hosted 库导入组件、公开头文件
radio/esp32c5/                      libgagent_wireless.a、libgagent_p4.a、sdkconfig.defaults
radio/esp32c6/                      同上（仅列出实际构建并导出的型号）
radio/esp32c61/                     同上
```

`gagent_wireless` 包含 Wi-Fi、BLE Host、Airlink 无线入口和 EspApplication 装配代码；EspApplication 嵌入无线对象，必须与无线实现作为同一变体选择。公共库不因从机型号重复交付。所有 `.a` 均为 ESP32-P4 目标，不能拿原生 C5 固件库代替。

客户工程有 `board.json` 时，修改其中的 `p4_hosted.chip` 为 `esp32c5`、`esp32c6` 或 `esp32c61`，同时填写实际引脚。导入脚本在 ESP-IDF 配置前选择包内对应库及默认配置。没有 `board.json` 的工程可在引入 `gagent-sdk.cmake` 前设置 `GAGENT_SDK_RADIO`，或者使用 `idf.py -D GAGENT_SDK_RADIO=esp32c5 build`。两处同时指定时必须一致；未指定时采用清单的 `default_radio`。

切换型号时使用新的构建目录，并移走旧的工作 `sdkconfig`（ESP-IDF 通常放在项目根目录）；默认配置不会强行覆盖已有 `sdkconfig`。缺少型号库、型号与工作配置不一致、ABI 配置不一致均报错，不回退到另一型号。

`board.json` 中同一传输方式的 SDIO/SPI 引脚、复位电平、时钟和延时可修改，生成的覆盖配置只写入构建目录 `gagent-sdk/sdk-radio.defaults`，不修改 SDK 包。当前一个型号变体固定一种传输方式；更换 SDIO/SPI 需导出对应传输方式的包。不能随意修改 NimBLE 容量、Wi-Fi 能力、FreeRTOS、TLS 等其他配置。

独立电源控制属于应用的运行时输入。创建 EspApplication 前，按 `board.json` 的值设置：

```cpp
platform::ApplicationConfig config;
config.hosted_power = {22, true, 20}; // 示例：GPIO22，高电平上电，等待20ms
// 无独立电源 GPIO 时保持默认 {-1, true, 20}。
```

该字段不会被 `board.json` 自动写入客户手写的 C++ 代码。仓库内自动生成的 MCU board_application.hpp 会传递它。供电参数在 Hosted 连接后不可更换；Wi-Fi 与 BLE 共用连接，停止 BLE 不会断开 Wi-Fi。

清单记录每个变体的完整配置。导出器合并多个构建时，要求工具链、IDF、依赖版本和相机设置一致，所有公共头文件一致，并对公共静态库去掉调试信息后比较代码、数据、符号与重定位；不一致则拒绝合并。该检查保证本次包内公共库可共用，不承诺任意 SDK 版本之间的 C++ ABI 兼容。

摄像头功能必须与 SDK 清单一致。例程可根据 `GAGENT_SDK_CAMERA` 自动链接摄像头，且配置 `ApplicationConfig::camera`；ABI 宏由库组件传播。摄像头接线、视频参数与后端配置编译进库，需要修改时重新导出 SDK。消费者保留包内 H.264 修正脚本，由 `gagent_sdk_check()` 在构建目录生成修正源文件，不修改 SDK 包本身。

更新已有工程的数据点协议时，使用 `tools/generate_soc_board.py --pk <PK> --datapoints <JSON> --project-dir <工程目录> --update`。仅覆盖生成的协议及元数据文件，保留产品配置和业务实现。新增工程请复制独立例程并适配产品业务；生成器不再创建带固定摄像头数据点的客户工程。`--template-dir` 仅保留为更新参数的兼容别名。

### P4 camera runtime configuration (ABI 1)

The offline [camera and radio configuration reference](docs/sdk-p4-camera.html) contains the complete field tables and a board.json example generator. It is shipped in camera SDK packages.

`board_config_abi: 1` packages generate a `gagent_board_binding` component in the consumer build directory. Hosted power defaults come from the same `p4_hosted` configuration as the bus pins; `ApplicationConfig::hosted_power.pin == -2` selects those defaults. Explicit power overrides remain available before connection.

`camera_config_abi: 1` packages support the curated CSI profiles in `tools/camera_profiles.json`: SC2336, OV2710, OV5647 and SC202CS. The consumer compiles the selected ESP sensor driver and native mode. No GAgent implementation sources are compiled; the camera archive is shared across these selections. Use a fresh build/sdkconfig when changing the sensor or native mode. A supported profile is not a claim of hardware verification on every module.

`gagent/camera/config.h` exposes copied, versioned configuration structs, profile enumeration, validation, initialization-time configuration, optional synchronous hardware probing, and asynchronous bitrate policy updates. Configuration locks at the first probe or service sync. The initial release supports H.264, integer output frame rates up to 30 fps, bounded scaling, GOP/QP settings and optional RTCP adaptation. It does not implement DVP/UVC, arbitrary cropping, external I2C ownership, image controls or a CBR/VBR selector. Unsupported values are rejected. Full resolution/hardware reconfiguration currently requires an application restart; stopping a stream does not unlock configuration.

A successful `gagent_camera_set_bitrate()` means queued. Observe `completed_id` and `error` through `gagent_camera_get_bitrate_status()`; the policy survives stream reconnections but not device reboot. `current_bps` is the encoder target, not measured throughput. `gagent_camera_get_config()` returns startup configuration; `probe.output` is the planned encoder output, not evidence that frames have been encoded.
