# GAgent SDK 例程

面向客户二次开发的独立例程仓库。每个例程都是完整 ESP-IDF 工程，无需内部固件源码。

| 例程 | 硬件 | 演示内容 |
| --- | --- | --- |
| [C3 基础联网](examples/c3_iot_basic/README.md) | ESP32-C3，4MB Flash，最低 v1.1 | BLE 配网、数据点收发、可扩展业务回调 |
| [C5 基础联网](examples/c5_iot_basic/README.md) | ESP32-C5，4MB Flash，最低 v1.2 | BLE 配网、数据点收发、可扩展业务回调 |
| [P4 联网与按键推流](examples/p4_iot_basic/README.md) | WT9932P4C61-TINY，P4＋C61 SDIO、SC2336 | BLE 配网、IoT 收发、RGB 状态灯、按键/数据点控制视频推流 |

例程不包含 PK／PS，使用前请在本地配置自己的产品凭据。摄像头控制已接入真实 SDK，喂食和定时数据点仅在内存中示范；正式产品请替换凭据并实现自己的业务。

## C3 / C5 开发者快速开始

两个基础工程均附带对应芯片的二进制 SDK，以及默认产品的 4 个数据点样例：`power`、`using_led_light`、`power_memory`、`timer_s`。不依赖外部 MCU，不预设 GPIO 外设。以 C3 为例（C5 将目录名换为 `c5_iot_basic`）：

```sh
cd examples/c3_iot_basic
cp main/product_secrets.example.hpp main/product_secrets.hpp
# 用编辑器填写自己的 PK/PS；已有本地配置时不要覆盖。
python3 gagent-sdk/configure_product.py
# 加载本地 ESP-IDF 6.1 环境后：
idf.py build
idf.py -p YOUR_SERIAL_PORT flash monitor
```

脚本按本地 PK 获取平台数据点，PS 不发送到网络；也可使用 `--datapoints product.local.json` 离线导入。生成文件位于 Git 忽略的 `main/generated/`；重新生成不覆盖业务回调。缺少配置或更换 PK 后未重新生成时，会明确阻止构建。平台公开接口目前为 HTTP，详见各工程 README。

默认产品的脱敏 JSON 和协议样例位于各工程 `sample/`。填写该产品的配套本地 PK/PS 后，执行 `python3 gagent-sdk/configure_product.py --sample` 可离线重建。更新公开样例使用 `--export-sample`；它会同步协议代码、脱敏 JSON 和校验信息，不覆盖业务回调。完整步骤见 [C3 README](examples/c3_iot_basic/README.md#3-生成与重建数据点) / [C5 README](examples/c5_iot_basic/README.md#3-生成与重建数据点)。

SDK 校验：根目录执行 `python3 tools/install_sdk.py --verify-only` 校验全部例程；加 `--example c3_iot_basic` 或 `--example c5_iot_basic` 可单独检查。各基础工程有自己的 `sdk.lock.json`。C61 独立工程暂缓。

## P4 摄像头快速开始

1. 准备 `sdk.lock.json` 指定提交的 ESP-IDF 6.1 和对应工具链，使用 CMake 3.24 或更新版本，加载 `export.sh` 环境。
2. 仓库已包含匹配的精简摄像头 SDK，无需另行获取 SDK 包。在仓库根目录校验：

```sh
python3 tools/install_sdk.py --verify-only
```

3. 复制本地凭据模板，填写自己的 PK/PS（须匹配示例的数据点定义）；`product_secrets.hpp` 已被 Git 忽略，缺少有效长度的凭据会在编译时报错：

```sh
cp examples/p4_iot_basic/main/product_secrets.example.hpp examples/p4_iot_basic/main/product_secrets.hpp
```

已有本地配置时不要覆盖；按[例程说明](examples/p4_iot_basic/README.md)配置产品。

4. 构建，连接测试板并烧录。首次构建需要联网访问乐鑫组件仓库：

```sh
cd examples/p4_iot_basic
IDF_COMPONENT_MANAGER=1 idf.py build
idf.py -p YOUR_SERIAL_PORT flash monitor
```

C61 无线从机须已有兼容 ESP-Hosted 2.12.11 的固件；本流程只构建和烧录 P4。

## 灯效与按键

- 配网、联网中或掉线重连：红灯闪烁，亮灭各 500 ms。
- IoT MQTT 已连接、未请求推流：绿灯常亮。
- 推流请求已接受、连接中或重试中：蓝灯闪烁，亮灭各 250 ms。
- IoT 在线且摄像头实际推流：蓝灯常亮；开始请求入队不等于推流成功。
- BOOT（GPIO35，低有效）：短按松手后开始/停止推流；按住满 5 秒清除 Wi-Fi 并重启进入 BLE 配网，保留设备身份和绑定信息。长按不会顺带触发推流；上电按住须先松开。

RGB 灯为 GPIO51 的 WS2812，GRB 顺序。App 的 `start_stream` 与按键共用控制逻辑。测试后端沿用板卡项目的 `http://101.126.88.194:8080`，需要为测试产品和设备提供可用会话。

## 从哪里修改

- `main/product_secrets.hpp`：本地 PK/PS（Git 忽略）；公开模板为 `main/product_secrets.example.hpp`。
- `main/product_config.hpp`：凭据校验、蓝牙名称、云地址。
- `main/gizwits_product.cc`：数据点下发、按键控制与状态采样。
- `main/hardware.cc`、`main/board_ui.hpp`：GPIO/RMT、消抖和灯效规则。
- `main/board.cc`：SDK 与可选摄像头服务的启动装配。
- `main/gizwits_protocol.*`：生成的数据点编解码，由平台定义重新生成。

代码带中文注释。业务状态与 SDK 操作由同一个 Worker 管理；其他任务可通过 `request_pairing()` 请求重新配网。

## 版本与验收

例程源码、中文文档和精简 SDK 均进入 Git。P4 保留脱敏数据点快照；C3/C5 按开发者的本地 PK 生成，数据点文件不进入 Git。各例程的锁定文件校验 SDK，P4 沿用根目录 `sdk.lock.json`；`main/idf_component.yml` 声明第三方组件的精确版本，`dependencies.lock` 固定组件内容。构建通过 ESP-IDF Component Manager 下载依赖到 `managed_components/`，该目录与构建产物一同被忽略。

SDK 注册 7 个 GAgent 组件，核心、云协议和相机库共用，另按 C5/C6/C61 选择无线适配静态库。定制的 WebRTC 接入代码以摄像头组件内部的配套静态库提供，不展开独立源码组件。mDNS、`esp-sr`、`esp-dl`、Hosted、音视频编解码等 Registry 组件不再随 SDK 提交。第三方组件下载后仍会占用本地磁盘；后续构建可复用下载结果。不要关闭 Component Manager，也不要手改 `managed_components/`。

- [上板验收](docs/上板验收.md)
- [例程维护与 SDK 升级](docs/维护说明.md)

无需摄像头的项目可由 SDK 提供方提供基础包及对应模板；本例程的锁定文件指向摄像头包，不能直接混用两种包。


## P4 无线芯片选择

当前 SDK 同时提供 C5、C6、C61 的 P4 无线适配 `.a`。例程默认仍为 C61。更换无线从机时，在 `examples/p4_iot_basic/board.json` 修改 `p4_hosted.chip`，并填写实际 SDIO 接线、复位及供电参数；无需手工替换单个 `.a`。

导入脚本在 ESP-IDF 配置前选择对应库。更换型号须使用新的构建目录，并移走旧的工作 `sdkconfig`；库缺失或工作配置与型号不一致会直接报错。C5/C6 的编译和链接通过，不代表已经在对应实物板卡上验收。

所有无线库都运行在 P4 上；C5/C6/C61 本身须安装与主机匹配的 ESP-Hosted 固件。独立供电 GPIO 还需在 `main/board.cc` 的 ApplicationConfig 中设置 `hosted_power`，详见 [SDK 适配说明](examples/p4_iot_basic/gagent-sdk/README.md#p4-v3-无线二进制变体)。

摄像头型号、通讯引脚、分辨率、帧率和动态码率配置，见可离线打开的 [P4 摄像头 SDK 配置手册](docs/sdk-p4-camera.html)。初始化参数由 `board.json` 生成；SDK 不再固定摄像头接线及输出参数。
