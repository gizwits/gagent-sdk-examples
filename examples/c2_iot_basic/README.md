# ESP32-C2 基础联网工程

直接在 ESP32-C2 上运行产品业务的独立 ESP-IDF 工程。提供 BLE V2 配网、Wi-Fi 自动重连、机智云数据点下发、查询和状态上报。SDK 已附带，不需要内部固件源码。C61 暂不在本批工程范围内。

C2/C3/C5 共用仓库现有的标准 4MB 分区布局，随包 `partitions.csv` 与 SDK 生产端基线保持一致。

默认 **4MB Flash，芯片最低修订版 v2.0**，**26MHz 晶振**，UART 日志 **74880**。C2 沿用专用内存配置，默认使用 BLE 配网，关闭 SoftAP 支持。此 SDK 只对应 4MB 工程，不适用于 2MB Flash 模组。40MHz 晶振板卡需重新构建对应 SDK，不能直接使用此包。

不使用外接 MCU、摄像头、按键或 LED；外设接线由开发者决定。本例程保留双 OTA 分区，但默认关闭自动 OTA 下载。

## 1. 准备环境

- Python 3.10 或更新版本，用于产品配置脚本；只使用标准库。
- ESP-IDF 6.1，固定提交 `2f8327156dd1f3c010413a6d47c9f86a607be498`。
- 对应 ESP-IDF 安装的 RISC-V 工具链 GCC 15.2.0，以及 CMake 3.24 或更新版本。

执行所安装 ESP-IDF 的 `export.sh`（Windows 使用对应的 `export.ps1`/`export.bat`）加载环境。精确版本、目标芯片及 SDK 哈希记录在本工程 `sdk.lock.json` 和 `gagent-sdk/sdk-manifest.json`；构建时会自动检查。SDK 不能与其他芯片的包混用。

## 2. 填写自己的产品凭据

在本工程目录执行一次：

```sh
cp main/product_secrets.example.hpp main/product_secrets.hpp
```

用编辑器打开 `main/product_secrets.hpp`，填写平台创建的产品 PK 和 PS（各 32 位十六进制字符串）。已有本地配置时不要重复复制。不要将实际凭据写到 README、`sdkconfig` 或命令行参数中。

```cpp
#define GAGENT_SOC_PRODUCT_KEY "这里填写自己的产品PK"
#define GAGENT_SOC_PRODUCT_SECRET "这里填写自己的产品PS"
```

这两行只是格式说明；必须填写实际的 32 位值。公开工程不附带可联网的测试凭据。

## 3. 生成与重建数据点

仓库的 `sample/` 已提供默认测试产品的 4 个数据点，以及对应的 `standard` 协议编解码样例：

| 数据点 | 类型 | 范围 / 用途 |
| --- | --- | --- |
| `power` | bool | 开关状态 |
| `using_led_light` | bool | 指示灯使能状态 |
| `power_memory` | bool | 断电记忆设置 |
| `timer_s` | uint32 | 0–86400 秒 |

这 4 个字段均可读写。基础工程只维护字段的内存状态，尚未实现实体开关、LED、持久化断电记忆或计时动作；请在业务回调中接入。`sample/datapoints.json` 的 PK 是脱敏占位符，`manifest.json` 保存产品匹配摘要，公开样例不包含实际 PK/PS。

### 使用随仓库提供的默认样例

先在 `main/product_secrets.hpp` 填入默认测试产品的配套 PK/PS，然后在本工程目录执行：

```sh
python3 gagent-sdk/configure_product.py --sample
idf.py build
```

`--sample` 完全离线，校验本地 PK 与样例的产品匹配，再从公开 JSON 重新生成可编译的 `main/generated/`。无需手动复制协议文件。自己的产品请使用下面的在线/JSON 入口，不能只换 PK 后套用其他产品的协议。

### 从平台重建（默认入口）

产品定义变更后，保持本地 PK/PS 正确，重新执行：


```sh
python3 gagent-sdk/configure_product.py
```

脚本读取本地 PK/PS，使用平台的[公开数据点接口](https://eusite.gizwits.com/zh-cn/document/m2m/i_02_datapoint/)获取当前产品定义。该接口目前使用 HTTP，只发送 PK，**PS 始终留在本地**。如果网络受限或希望离线工作，从平台导出同一产品的 JSON，保存为 `product.local.json`：

```sh
python3 gagent-sdk/configure_product.py --datapoints product.local.json
# 使用上次成功保存的数据点，不访问网络
python3 gagent-sdk/configure_product.py --from-cache
```

### 更新可提交的公开样例

需要同步 `sample/` 中的 JSON 和协议代码时，执行：

```sh
# 联网获取当前产品定义，再生成本地代码与公开样例
python3 gagent-sdk/configure_product.py --export-sample
# 或从已经确认的本地 JSON 重建两份结果
python3 gagent-sdk/configure_product.py --datapoints product.local.json --export-sample
```

`--export-sample` 会替换整个 `sample/`，去除实际 PK/PS，并更新文件校验值及产品匹配摘要。提交前检查样例字段变化；不要直接把 `main/generated/` 拷入 Git。只更新本地可编译文件时无需此选项。

支持 SDK 生成器已有的 `standard` 和 `var_len` 数据点协议，完整 P0 报文上限 512 字节。生成前校验产品匹配、字段布局和大小；失败时保留原有文件。

生成结果位于 `main/generated/`：

| 文件 | 用途 |
| --- | --- |
| `gizwits_protocol.hpp/.cc` | 数据点类型、字段枚举和编解码 |
| `datapoints.json` | 本次成功生成的数据点快照，可离线复用 |
| `generation.json` | 协议类型及字段数量 |
| `product_identity.hpp` | 绑定生成时使用的产品，防止误用其他产品的数据点 |

`sample/` 是可提交、可阅读的公开样例；`main/generated/` 是带本地产品绑定信息的实际编译输入，始终由 Git 忽略。

生成目录完全由工具管理。不要手工编辑它；修改平台定义后重新运行脚本。更换 PK 后也必须重新生成，否则编译会报错。`main/gizwits_product.cc` 等业务源码不会被脚本覆盖；删除或重命名数据点后，须同步修改引用该字段的业务代码。

## 4. 编译与烧录

```sh
idf.py build
idf.py -p YOUR_SERIAL_PORT flash monitor
```

首次构建需要联网下载 `main/idf_component.yml` 和 `dependencies.lock` 锁定的 mDNS、JSON、MQTT 等组件。不要关闭 ESP-IDF Component Manager。本工程使用标准 UART0 控制台（TX GPIO20、RX GPIO19），波特率为 74880；C2 没有原生 USB，需使用板载或外接 USB 转串口。

`idf.py flash` 会写入本例程的分区布局，测试曾运行其他固件的板卡前先备份设备数据。已有其他产品身份时，例程会拒绝继续启动；有意切换产品时，按板卡调试流程备份并清理 `GData_0`、`GData_1`，不要把擦除身份作为日常重新配网操作。

首次启动没有 Wi-Fi 配置时自动进入 BLE 配网。使用支持 BLE V2、且已关联当前产品的机智云 App 配网。观察 `IoT MQTT ready` 确认云连接；随后通过 App 下发可写字段、查询设备状态，并检查变更上报。没有实体外设时，可写字段保存在内存中，重启恢复默认值。

## 5. 接入产品业务

| 编辑位置 | 何时调用 / 作用 |
| --- | --- |
| `main/gizwits_product.cc::initialize()` | 启动时设置业务初始值 |
| `apply(candidate, written)` | 校验并执行数据点下发，失败返回 `false` |
| `sample(candidate, now)` | 采集传感器或消费其他任务的事件，修改待上报状态 |
| `main/product_config.hpp` | 蓝牙名称、云地址及产品属性 |
| `main/board.cc` | SDK 启动装配、配网方式和 OTA 策略 |

字段名称与平台一致，C++ 成员名会加 `value_`，枚举加 `dp_`。例如产品定义了 `power` 后，可在 `apply()` 中用 `written[static_cast<std::size_t>(datapoints::Field::dp_power)]` 判断是否被写入，再读取 `candidate.value_power` 控制外设。生成前不要假定任何固定字段存在。

回调运行于 GAgent Worker，必须保持非阻塞；其他任务/中断通过有界队列传递事件，由 `sample()` 消费。变化上报尝试间隔至少 1 秒，每 10 分钟完整上报；发布成功表示已交给发送流程，不等于云端确认。

## 6. 分享与版本管理

`main/product_secrets.hpp`、`main/generated/`、`*.local.json`、构建产物和下载组件已被本工程的 `.gitignore` 忽略。可以把本目录完整复制为独立工程，但打包分享时也要主动排除这些本地文件，Git 忽略规则不会替你过滤手工压缩包。

提交业务源码、`sample/` 和 SDK；其他开发者填写自己的 PK/PS 后按上面的步骤重新生成。不要直接修改 `gagent-sdk/` 中的文件，它们受哈希校验保护。此工程已进行主机测试与真实 ESP-IDF 编译/链接验证，尚未进行实机配网和云端收发验收。
