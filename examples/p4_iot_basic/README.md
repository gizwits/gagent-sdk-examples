# P4 配网、IoT 收发与按键推流示例

用于 WT9932P4C61-TINY：ESP32-P4、16 MB Flash、32 MB PSRAM，C61 SDIO 接线继承仓库的 `gagent_cam_esp32p4`。MCU 串口不参与这个示例。本例程使用摄像头 SDK，演示联网收发与按键推流。C61 需已有兼容 ESP-Hosted 2.12.11 的从机固件。

这是可复制到仓库外的 ESP-IDF 工程。`main/product_config.hpp` 内置获准公开的测试 PS，PK 位于生成的 `main/gizwits_protocol.hpp`；均只属于测试应用，不编入通用 SDK 库。正式产品请替换产品配置及数据点。

## 构建与测试

使用 SDK 清单指定提交的 ESP-IDF 6.1 和工具链。在本目录安装 SDK 为 `gagent-sdk/`，先加载 IDF 环境，再运行：

```sh
IDF_COMPONENT_MANAGER=0 idf.py build
idf.py -p YOUR_SERIAL_PORT flash monitor
```

首次使用原 MCU 固件的设备，先备份原设备 Flash。若 GData 中已有其他产品身份或不同 PS，应用会拒绝启动；明确清理测试设备的 `GData_0`、`GData_1` 后重新启动。升级本示例无需反复清理；不要为了重新配网擦除设备身份。示例不自动修改 C61 从机固件。

1. 无 Wi-Fi 配置时自动进入 BLE V2 配网，用支持该协议和测试产品的机智云 App 配网。
2. 已保存 Wi-Fi 配置时自动重连。观察无线、云注册、MQTT 日志与 `IoT MQTT ready`；仅 Wi-Fi 获得 IP 不算上云成功。
3. 修改 `feed_portion`，日志出现 `RX datapoint: feed_portion`；在 App 查询/收到上报时核对数值。
4. 摄像头 SDK 下，按 BOOT 或设置 `start_stream=true` 开始推流，再按一次或设置 `false` 停止。基础 SDK 不接受启动摄像头请求。`feed_now`、定时等其余字段仍只更新内存，不执行喂食或定时动作；只读字段可在 `sample()` 中接入传感器。
5. 状态变化最多每秒上报一次，每 10 秒完整上报。`TX ... submitted` 仅表示发送尝试，最终以 App 收到为准。
6. 断开再恢复路由器，检查自动恢复；重启板子检查 Wi-Fi 保留、业务数据恢复默认。

## 指示灯与按键

硬件依据 WT9932P4C61-TINY 原理图和板级定义：RGB 为 GPIO51 的 WS2812（GRB），BOOT 为 GPIO35，低电平按下。

| 状态 | 灯效 |
| --- | --- |
| BLE 配网、联网中或掉线重连 | 红灯闪烁，亮/灭各 500 ms |
| IoT MQTT 已连接，未实际推流 | 绿灯常亮 |
| IoT 在线且摄像头实际推流 | 蓝灯常亮 |

配网/掉线的红闪优先于推流状态。按键有 30 ms 消抖，持续按住只触发一次，上电按住需要先松开。请求入队、正在建立连接或连接失败时不会误亮蓝灯；正在连接时再按一次可取消。

`start_stream` 表示已接受的推流意图，断线期间由 SDK 负责恢复；蓝灯依据实际摄像头状态，而非这个布尔值。启动请求被拒绝（例如离线、繁忙）时不改变数据点，日志显示返回码。停止请求成功后，SDK 异步释放会话，蓝灯在实际停止后熄灭。

摄像头包使用 SC2336、1024×600/30 fps、初始 1 Mbps，无检测模型。测试后端为 `http://101.126.88.194:8080`，必须为当前 PK/设备提供可用会话；App 的观看入口及后端联调需实际验收。后端、摄像头接线和参数目前编译进 SDK，修改例程 `board.json` 不会改变库内配置，需重新导出匹配包。

## 客户编辑位置

- `main/product_config.hpp`：PS、云地址、蓝牙名称；PK 由数据点生成器维护。
- `main/gizwits_product.cc`：`apply()` 接收数据点字段，`sample()` 更新传感器/业务状态。已接入按键、状态灯和可选摄像头控制，其余数据点保留内存示范。
- `main/hardware.cc`、`main/board_ui.hpp`：GPIO/RMT 驱动、消抖和灯效优先级。
- `main/board.cc`：已提供完整启动装配，默认 BLE 首次配网、关闭自动 OTA 下载。

业务回调运行在 GAgent Worker，必须非阻塞。其他任务或按键处理可调用 `product::request_pairing()`；它只设置原子请求，由 Worker 进入 BLE 配网，不直接跨任务操作 SDK。BOOT 按键用于切换推流，不用于触发配网。

产品定义升级时在工程外执行随 SDK 附带的工具：

```sh
python3 gagent-sdk/tools/generate_soc_board.py --pk 00000000000000000000000000000000 --datapoints main/datapoints.json --template-dir . --update
```

只更新协议、快照和生成元信息，保留业务回调、凭据、启动与分区。更换 PK 应创建新模板，不能用 `--update` 覆盖另一个产品。模板配置与生成源码都属于应用，SDK 库中不包含测试产品身份。

## 容量与兼容性

`partitions.csv` 和本工程 `sdkconfig.defaults` 由客户维护；Flash 容量、分区可修改，但必须保留双 GData、系统 NVS、双 OTA 槽及启用的 coredump 分区。构建校验布局、容量及镜像能否放入每个 OTA 槽。更改布局只针对重新烧录的设备，不代表 OTA 能自动迁移旧分区。

其余预编译库依赖的 SDK 配置保持清单一致。该包对应当前 C61 SDIO 接线；改变 Hosted 接线、芯片、BLE 协议或 ABI 配置需要重新导出匹配的 SDK。`board.json` 是这组硬件接线的声明，Flash 实际容量以 sdkconfig 为准。

SDK 下的文件有哈希校验；请编辑复制出的工程，勿直接修改 `gagent-sdk/examples/`。正常构建只使用 SDK 的库、头文件和随包第三方依赖，不需要原仓库。
