# CoffeePilot ESP32-S3-N16R8 咖啡机控制器

这是按 Arduino IDE 结构实现的 ESP32-S3 控制器骨架，面向欧姆龙 E5CC、增压泵、压力变送器、手动旋钮和自动萃取按钮。当前版本重点是把安全边界、并发路径和手机操作界面先搭稳，再接真实水路整定。

当前固件版本为 `0.4.0`。目标硬件为第三方 **ESP32-S3-N16R8**：16 MB Flash、8 MB OPI PSRAM。

## 相比原方案的调整

1. **温度与压力职责分离**：E5CC 继续做锅炉温度 PID，ESP32-S3 只写温度设定值、读回 PV，并把实时闭环留给压力。
2. **唯一正常泵入口**：网页、旋钮、自动状态机都不能直接写 PWM，最终统一经过 `PumpArbiter` 和 `PumpDriver::setVoltage()`；安全任务只保留紧急断电旁路。
3. **命令与实时控制解耦**：HTTP 回调只解析参数和投递事件，Modbus 写入、PID 和泵输出全部在专用任务中完成。
4. **手机页面改为驾驶舱式单屏**：温度、压力、状态、泵电压、本杯计时和 90 秒曲线集中显示；配方与设备设置分页，所有触控目标按手机单手操作设计。
5. **首版不依赖第三方 WebSocket 库**：内置 `WebServer` 提供 REST，页面每 750 ms 拉取一次状态。第一版先保证零额外库、可恢复和易排错；后续再升级 WebSocket/SSE 时不用改动控制层。
6. **故障锁存与复位条件**：超压、超温、压力传感器失效、E5CC 失联和泵超时都会锁存。复位前检查压力已释放、温度回落、通信恢复且旋钮关断。
7. **启动安全默认值**：上电先关闭泵；旋钮开关默认按接地有效的低电平处理；泵使能默认关断。
8. **控制任务心跳兜底**：安全任务在另一核心检查控制任务心跳。若控制任务卡死，先切断泵，再重启系统。
9. **独立设备热点**：上电后立即启动 `CoffeePilot` AP，家庭 Wi-Fi 作为可选 STA 同时工作；STA 断线不会关闭 AP，网页始终保留本地入口。
10. **多配方与主页快捷切换**：最多保存 8 个萃取配方，参数写入 NVS；主页可在开始萃取前选择当前配方。
11. **设置页分组**：锅炉温度与压力 PID 集中在一组，Wi-Fi 连接和设备热点可以在手机页面配置并持久保存。

## Arduino 工程结构

```text
CoffeeController/
├── CoffeeController.ino       # 任务编排、网络、输入、Modbus 和应用事件
├── UserConfig.h               # 先修改这里：引脚、Wi-Fi、传感器、E5CC 参数
├── Config.h                   # 周期、阈值和派生常量
├── AppTypes.h                 # 状态、故障位、配方和快照类型
├── ControllerState.*          # 互斥保护的运行时数据
├── SettingsStore.*            # NVS 参数持久化与边界校验
├── AppEvents.*                # Web/输入到控制层的命令队列
└── src/                       # Arduino 自动递归编译的模块目录
    ├── app/
    │   └── EspressoStateMachine.* # 预浸泡、建压、萃取、停止和故障状态
    ├── services/
    │   ├── PressureLoop.*     # 带抗饱和的压力 PI
    │   ├── PumpArbiter.*      # 安全 > 手动 > 自动 > 待机
    │   └── SafetyMonitor.*    # 故障检测和锁存
    ├── drivers/
    │   ├── E5ccModbus.*       # 无第三方库的 Modbus RTU
    │   ├── PumpDriver.*       # PWM + RC 输出和泵使能
    │   └── PressureSensor.*   # ADC、分压还原、中值滤波
    └── web/
        ├── WebController.*    # HTTP API
        └── WebApp.h           # 内嵌移动端页面
```

## 默认任务分配

| 任务 | 核心 | 周期 | 职责 |
|---|---:|---:|---|
| Control | 1 | 50 ms，PID 100 ms | 采样压力、压力环、泵仲裁、唯一正常输出 |
| Modbus | 1 | 500 ms | 读 E5CC PV，目标变化时写 SP |
| Safety | 0 | 50 ms | 阈值检测、故障锁存、控制任务心跳兜底 |
| App | 0 | 25–50 ms | 状态机、按钮/Web 事件 |
| Input | 0 | 20 ms | 按钮、旋钮开关和旋钮 ADC |
| Web | 0 | 事件驱动 | HTTP 和手机页面 |

Wi-Fi 协议栈仍在 Core 0，压力控制和 E5CC 轮询放在 Core 1。网页访问量不会直接阻塞压力环。

## 先核对硬件

默认引脚只是 ESP32-S3-N16R8 示例，必须在 [UserConfig.h](UserConfig.h) 中与实物逐项核对。

| 功能 | 默认 GPIO | 说明 |
|---|---:|---|
| RS485 RX / TX / DE | 18 / 17 / 16 | MAX485 的 RO、DI、DE+RE |
| 泵 PWM | 4 | 经 RC 滤波后接泵 0–3 V 输入 |
| 泵使能 | 5 | 低电平在上电时关闭泵 |
| 压力 ADC | 1 | ADC1，必须确认最终输入不超过 3.3 V |
| 旋钮 ADC | 2 | ADC1 |
| 旋钮有效开关 | 6 | 默认低电平有效，内部上拉 |
| 自动萃取按钮 | 7 | 低电平有效，内部上拉 |
| 状态灯 | 48 | 若开发板 LED 不在该脚，可改为 `-1` 禁用 |

关键硬件限制：

- 压力变送器若是 0.5–4.5 V 输出，禁止直接接 ADC。默认按 10 kΩ / 20 kΩ 分压配置，计算结果为 `Vadc = Vsensor × 2/3`，4.5 V 时约 3.0 V。
- RC 滤波后若泵输入阻抗较低，应增加运放缓冲。软件上限不能代替硬件限压。
- 泵使能、SSR 和主电源必须有 MCU 复位、断电或 PWM 线断开时自动关闭的硬件设计。
- 必须保留独立急停和机械泄压路径。软件超压保护只是第二道保护。
- E5CC 的从站号、波特率、校验位、PV 地址和 SP 可写地址必须在实机上确认。默认值为 `1 / 9600 / 8E1 / 0x2000 / 0x2103 / ×0.1`。

## Arduino IDE 使用

1. 安装 **ESP32 by Espressif Systems** 开发板包。本工程按 ESP32 Arduino Core 3.x 编写，也保留了 2.x 的 LEDC 兼容分支。
2. 打开 `CoffeeController/CoffeeController.ino`。
3. 开发板选择 `ESP32S3 Dev Module`。
4. 建议参数：Flash Size `16MB`、PSRAM `OPI PSRAM`、Partition Scheme 选择适合 16 MB Flash 的方案、USB CDC On Boot `Enabled`。
5. `UserConfig.h` 中的家庭 Wi-Fi 和热点密码只作为首次启动默认值；之后可在手机“设备设置”中修改并保存。
6. 先只接 USB，不接泵主电源，执行编译和上传。
7. 手机连接热点 `CoffeePilot`，密码 `CoffeePilot-1234`，浏览器打开 `http://192.168.4.1`。
8. 若已配置家庭 Wi-Fi，也可访问 `http://coffee.local` 或串口日志中显示的 STA IP。

投产前必须通过 `UserConfig.h` 更换默认 AP 密码。AP 使用 WPA2；网页不包含账户登录，不应将设备直接暴露到公网。

## API

| 方法 | 路径 | 用途 |
|---|---|---|
| GET | `/api/status` | 温度、压力、状态、故障、计时、通信信息 |
| GET | `/api/settings` | 配方列表、当前配方、温度目标、PID、网络摘要 |
| POST | `/api/settings` | 保存锅炉目标温度和 PID；萃取期间返回 409 |
| POST | `/api/recipes` | 选择、保存或删除配方，最多 8 个 |
| POST | `/api/network` | 保存家庭 Wi-Fi 或设备热点配置并重新连接 |
| POST | `/api/brew` | `start`、`stop` 或 `toggle` |
| POST | `/api/fault/reset` | 在安全条件满足后清除锁存 |
| GET | `/api/health` | 固件健康检查 |

命令接口只投递事件，不在 HTTP 回调内直接操作 Modbus、PID 或 PWM。

## 联调顺序

1. **只接 USB**：确认网页、日志、按钮和旋钮状态正常，泵 PWM 始终为 0。
2. **E5CC**：读到合理 PV，修改目标温度后确认面板 SP 变化。
3. **PWM + RC**：万用表测 0%、50%、91% 对应直流电压，不接泵。
4. **压力传感器**：用泄压状态和已知压力点核对 bar 换算。
5. **旋钮开环**：确认旋钮有效时优先级最高、松开后回到自动条件，不让自动程序自动续跑。
6. **压力 PI**：先 P，再加 I，最后考虑 D；观察超调、稳态误差和泵噪声。
7. **自动序列**：先空水路短时间测试，再逐步延长萃取时间。
8. **故障注入**：拔传感器、断 RS485、持续超压、长按泵和卡死控制任务，逐项确认泵会关闭并锁存。

## 当前仍需确认

- E5CC 的真实通信参数和 SP 写入地址。
- 泵控制器输入阻抗、0–3 V 映射是否线性。
- 压力变送器实际输出范围、量程和供电。
- 旋钮开关的有效电平。
- 泵和 SSR 在 MCU 复位、USB 插拔和 3.3 V 掉电时是否一定失电。

## Git 工作流

```powershell
git status
git diff
git add .
git commit -m "feat: initial CoffeePilot controller"
```

实际 Wi-Fi 密码、API 令牌和现场整定数据不要提交到仓库。当前默认 Wi-Fi 凭据位于 `UserConfig.h`，生产使用前应改到个人配置并避免公开。
