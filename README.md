# GAgent SDK 例程

面向客户二次开发的独立例程仓库。每个例程都是完整 ESP-IDF 工程，无需内部固件源码。

| 例程 | 硬件 | 演示内容 |
| --- | --- | --- |
| [P4 联网与按键推流](examples/p4_iot_basic/README.md) | WT9932P4C61-TINY，P4＋C61 SDIO、SC2336 | BLE 配网、IoT 收发、RGB 状态灯、按键/数据点控制视频推流 |

首个例程使用已授权公开的测试 PK／PS。摄像头控制已接入真实 SDK，喂食和定时数据点仅在内存中示范；正式产品请替换凭据并实现自己的业务。

## 快速开始

1. 准备 `sdk.lock.json` 指定提交的 ESP-IDF 6.1 和对应工具链，使用 CMake 3.24 或更新版本，加载 `export.sh` 环境。
2. 仓库已包含匹配的精简摄像头 SDK，无需另行获取 SDK 包。在仓库根目录校验：

```sh
python3 tools/install_sdk.py --verify-only
```

3. 构建，连接测试板并烧录。首次构建需要联网访问乐鑫组件仓库：

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

- `main/product_config.hpp`：产品凭据、蓝牙名称、云地址。
- `main/gizwits_product.cc`：数据点下发、按键控制与状态采样。
- `main/hardware.cc`、`main/board_ui.hpp`：GPIO/RMT、消抖和灯效规则。
- `main/board.cc`：SDK 与可选摄像头服务的启动装配。
- `main/gizwits_protocol.*`：生成的数据点编解码，由平台定义重新生成。

代码带中文注释。业务状态与 SDK 操作由同一个 Worker 管理；其他任务可通过 `request_pairing()` 请求重新配网。

## 版本与验收

例程源码、中文文档、数据点快照和精简 SDK 均进入 Git。`sdk.lock.json` 校验 SDK 文件；`main/idf_component.yml` 声明第三方组件的精确版本，`dependencies.lock` 固定组件内容。构建通过 ESP-IDF Component Manager 下载依赖到 `managed_components/`，该目录与构建产物一同被忽略。

SDK 约 26 MiB，只注册 6 个 GAgent 组件。定制的 WebRTC 接入代码以摄像头组件内部的配套静态库提供，不展开独立源码组件。mDNS、`esp-sr`、`esp-dl`、Hosted、音视频编解码等 Registry 组件不再随 SDK 提交。第三方组件下载后仍会占用本地磁盘；后续构建可复用下载结果。不要关闭 Component Manager，也不要手改 `managed_components/`。

- [上板验收](docs/上板验收.md)
- [例程维护与 SDK 升级](docs/维护说明.md)

无需摄像头的项目可由 SDK 提供方提供基础包及对应模板；本例程的锁定文件指向摄像头包，不能直接混用两种包。
