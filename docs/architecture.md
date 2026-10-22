# 架构与实现方案

## 1. 分层

```text
Win32 输入 / 窗口层
  RawInputManager        WM_INPUT -> KeyBinding / 右键按下抬起
  MainOverlayWindow      透明置顶点击穿透窗口（白点或原始尺寸图片）

状态与配置层
  UserConfig             持久化模型 + 默认值 + JSON 映射
  RuntimeState           当前武器、弹药状态、运行时容量副本
  KeyBinding             键盘/鼠标输入标识、规范化、显示名、匹配
  ConfigManager          单次读取、异步原子保存、等待保存结束
  AppPaths / Log         数据目录、固定文件路径、文件日志

UI / 控制层
  App                    生命周期、热键业务、右键开镜隐藏状态机、退出顺序
  TrayController         托盘菜单、单击切换状态、双击设置
  SettingsWindow         非模态单例设置窗口，关闭时提交
  widgets/*              键入框、容量框、参数框、无边框标题栏
```

### 类职责

| 类 | 职责 |
| --- | --- |
| `App` | 单例编排器。创建窗口/托盘/RawInput/定时器；处理武器、切换、重置、容量与右键隐藏规则；统一退出。 |
| `MainOverlayWindow` | 只覆盖内容区域。白点由 `paintEvent` 绘制；图片按原始尺寸绘制；透明、置顶、无边框、Tool、点击穿透、不抢焦点、居中。 |
| `RawInputManager` | 创建原生消息窗口，注册键盘/鼠标 `RIDEV_INPUTSINK`，解析 `WM_INPUT`，忽略键盘自动重复，输出 `KeyBinding` 与右键状态。 |
| `StatusOverlayWindow` | 独立的透明置顶状态窗口。自绘当前武器数字、右上上标和右下下标；整个窗口可左键拖动；尺寸只随缩放比例和内容变化；保存/恢复左上角坐标。 |
| `ConfigManager` | 启动读取一次 `user-config.json`；当前 schema 不兼容时整体使用默认值；后台专用线程串行保存最新快照；临时文件 + Win32 原子替换；所有异常在内部捕获并写日志；提供等待接口。 |
| `UserConfig` | 题目给定的全部字段、默认值、当前 schema 的严格 JSON 序列化/反序列化；不包含图片路径。 |
| `RuntimeState` | `CurrentWeapon`、三个弹药状态、三个运行时容量副本。 |
| `KeyBinding` | 键盘扫描码 + 扩展标志 + 规范化 VK；鼠标中键/侧键/滚轮；显示名；JSON；相等匹配。 |
| `TrayController` | 托盘图标和固定顺序菜单；左键单击切换状态显示（延迟到双击间隔后），双击打开设置；“状态显示”可勾选项。 |
| `SettingsWindow` | 760px 宽的无边框单例设置窗口；置顶（`WS_EX_TOPMOST`），但保持可聚焦、可输入、可拖动；键入框、容量框、参数框、状态显示缩放滑块；关闭时提交并发出 `configCommitted`。 |
| `KeyBindingEdit` | 只读按键显示框；左键进入捕获态；右键清空；捕获态高亮；空绑定显示“未绑定”。 |
| `CapacityEdit` | 0~9 单字符输入；按合法数字立即写入并失焦；其他按键吞掉并继续等待；Tab/Enter 失焦；不做失焦修正。 |
| `ParameterEdit` | 只允许十进制数字；聚焦全选；Tab/Enter/其他控件失焦；空/非法恢复原值。 |
| `FramelessTitleBar` | 无标题文字标题栏；只放关闭“×”；按住空白区拖动窗口。 |

## 2. 主窗口

### 几何

- 白点模式：窗口 `DotSize × DotSize`，左上角为
  `primaryScreen.geometry().x + floor((screenW - dotW) / 2)`，纵向同理。
- 图片模式：窗口 `QImage::size()`，图片按 `1:1` 目标矩形绘制；不做
  `KeepAspectRatio`/fit 缩放；大于屏幕时允许负坐标、超出屏幕。
- 屏幕区域使用主屏**完整** `geometry()`，不是 `availableGeometry()`。

### 窗口属性

Qt：

```cpp
Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool
| Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus
Qt::WA_TranslucentBackground
Qt::WA_NoSystemBackground
Qt::WA_ShowWithoutActivating
Qt::WA_TransparentForMouseEvents
```

原生扩展样式：

```text
WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW
去掉 WS_EX_APPWINDOW
```

显示时使用 `ShowWindow(SW_SHOWNOACTIVATE)` 与
`SetWindowPos(HWND_TOPMOST, SWP_NOACTIVATE)`。`WM_MOUSEACTIVATE` 返回
`MA_NOACTIVATE`，`WM_NCHITTEST` 返回 `HTTRANSPARENT`。

白点完全由代码绘制：透明背景上使用抗锯齿白色椭圆，默认 3px。图片则直接
`drawImage` 到同尺寸矩形，保留 PNG Alpha。

### 图片持久化

1. 文件对话框过滤 `png/jpg/jpeg/bmp/gif`。
2. `QImageReader::setDecideFormatFromContent(true)`，不依赖扩展名；GIF 读取首帧。
3. 验证可解码后，用 `QSaveFile` 将原始字节覆盖到固定文件
   `%APPDATA%\BigA\current-image.img`。
4. 启动时固定文件存在且可解码则显示图片，否则显示白点。
5. “还原白点”只切换显示状态，不删除固定文件。

### 状态显示窗口

`StatusOverlayWindow` 是独立于主叠层的第二个透明置顶窗口：

- 无标题栏、无边框、`Qt::Tool`、`WS_EX_NOACTIVATE`、`WS_EX_TOOLWINDOW`，
  使用 `HWND_TOPMOST`；不设置 `WS_EX_TRANSPARENT`，整个窗口可接收左键拖动。
- 自绘当前武器数字：一号~手枪映射为 1~4；非刀状态在数字右上方绘制弹药状态、
  右下方绘制容量；刀只绘制大号 3。
- 主数字 40px、上下标 16px（1.0 比例），纯白色填充，无描边、无阴影。
- 窗口尺寸由字体度量和内容决定，仅随缩放比例、武器/状态/容量变化；没有拖边 resize。
- 缩放比例来自 `UserConfig::StatusOverlayScale`，范围 0.5~3.0、步进 0.1。
- 左上角位置保存在 `StatusOverlayX/Y`；默认主屏可用区域右下角。启动恢复时若坐标
  不在任何屏幕可用区域，则回退右下角。缩放和设置关闭时会把窗口约束回屏幕内。
- 拖动结束通过 `dragFinished` 信号写入配置并异步保存。
- 创建在主线程延迟执行（`QTimer::singleShot(0, ...)`），构造和初始化异常被捕获，
  失败只记录日志，不影响主窗口、托盘、RawInput、设置。托盘的“状态显示”再次勾选时
  会重试创建。

托盘菜单顺序为“选择图片”“还原白点”“状态显示”“设置”“退出”。“状态显示”
为可勾选项，默认 false，勾选/取消立即保存。

托盘图标左键单击在系统双击间隔结束后切换状态显示窗口；若期间收到双击，
取消单击切换并打开设置，因此双击不会误切换状态。

配置增量：

```text
StatusOverlayEnabled  bool，默认 false
StatusOverlayX/Y      int，默认 INT_MIN（尚未保存位置的哨兵值）
StatusOverlayScale    double，默认 1.0，读取时限制在 0.5~3.0 并保留一位小数
```

勾选/取消、拖动结束、设置关闭分别通过 `SaveUserConfigAsync()` 保存。

## 3. RawInput

`RawInputManager` 创建自己的原生顶层窗口，使用：

```cpp
RAWINPUTDEVICE keyboard{0x01, 0x06, RIDEV_INPUTSINK, sinkHwnd};
RAWINPUTDEVICE mouse   {0x01, 0x02, RIDEV_INPUTSINK, sinkHwnd};
RegisterRawInputDevices(...)
```

消息窗口为 1×1、位于屏幕外、`WS_EX_LAYERED | WS_EX_TRANSPARENT |
WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW`，Alpha=0，并显示但不激活，确保各 Windows
配置下 `WM_INPUT` 都投递到该窗口且用户不可见。窗口过程只处理 `WM_INPUT`，
随后调用 `DefWindowProcW` 完成系统清理。

键盘：

- 先按 `RI_KEY_BREAK` 处理抬起，再从按下集合移除。
- 使用 `扫描码 + E0 + scan` 组成身份集合，过滤自动重复。
- 忽略 `VK 0/0xFF` 伪键；Esc 不产生绑定，只发出 `escapePressed`。
- 小键盘通过扫描码规范化为 `VK_NUMPAD0..9/VK_ADD/VK_SUBTRACT/VK_MULTIPLY/
  VK_DIVIDE/VK_DECIMAL`，NumLock 开/关都映射到同一绑定。
- 扩展标志来自 `RI_KEY_E0`。

鼠标：

- 右键按下/抬起分别发信号；重复按下做状态去重。
- 中键、XButton1、XButton2 按下加入普通 `KeyBinding` 输入流。
- 滚轮的 `usButtonData` 正负决定“滚轮上/下”。
- 左键不参与热键；右键固定用于隐藏/恢复与键入框删除绑定。

信号由 `App` 使用 `Qt::QueuedConnection` 连接，避免在窗口过程中直接弹模态
确认框造成消息过程重入。

## 4. KeyBinding

字段：

```cpp
enum class KeyBindingType { None, Keyboard, MouseMiddle, MouseX1, MouseX2,
                            WheelUp, WheelDown };
quint16 scanCode;
bool extended;
quint16 virtualKey;
```

- 空绑定 `None` 不参与匹配。
- 键盘相等条件：扫描码、扩展标志、规范化 VK 全部相等。
- 鼠标相等条件：设备/方向类型相等。
- JSON 示例：

```json
{ "type": "keyboard", "scanCode": 72, "extended": true, "virtualKey": 38 }
{ "type": "mouseMiddle" }
{ "type": "wheelUp" }
{ "type": null }
```

### 捕获

- 鼠标左键进入捕获，显示高亮边框与“请按键…”。
- 合法单键到达后立即写回绑定并退出捕获。
- Esc 由 RawInput 单独上报，取消捕获并恢复显示原值。
- 右键任意时刻清空当前绑定，退出捕获，不进入捕获态。
- 若新按键已在其他槽位，弹出“该键已绑定到 X，是否解绑并重新绑定？”：
  - 取消：保留原绑定，继续捕获；
  - 确认：清空旧槽位并写入新槽位。

## 5. 运行规则

`RuntimeState::CurrentWeapon`：

```text
0 = 一号武器
1 = 二号武器
2 = 刀
3 = 手枪
```

- 武器键：仅改 `CurrentWeapon`。
- 切换键：当前为刀则忽略；容量为 0 则忽略；否则
  `(state + 1) % runtimeCapacity`。
- 重置键：`CurrentWeapon=0`，三个弹药状态归零；容量副本不变。
- 容量+1/-1：
  - 当前为刀：立即返回，不改容量、不动现有计时器、不保存。
  - 否则修改当前武器运行时容量，`clamp(0, 9)`；到边界仍重启 3 秒计时。
  - 3 秒到期后把三个运行时容量同步回 `UserConfig`，调用统一异步保存。
- 设置关闭：停止容量防抖、容量副本取界面值、重置四个运行时状态、保存。

### 右键开镜隐藏状态机

```text
容量优先规则（任何时刻）:
  if (CurrentWeapon != 刀 && capacity == 0)
      hide 主窗口
      该规则不受 rightHeld / ScopeDelayMs 影响；
      右键按下、保持、抬起都不会重新显示。

右键按下:
  rightHeld = true
  取消已有 QTimer，按 ScopeDelayMs 重新 singleShot
  应用容量优先规则

计时器到期:
  if (!rightHeld) return
  if (CurrentWeapon == 刀) hide
  else if (ammoState == capacity - 1) 不隐藏
  else hide

右键保持期间，延迟已结束且按下已绑定功能的按键:
  用最新 CurrentWeapon / ammoState / capacity 立即重新判断
  容量为 0 -> 立即 hide
  需要隐藏 -> hide；已经不需要隐藏 -> show

右键抬起:
  rightHeld = false
  取消计时器
  容量为 0 -> hide
  否则 show overlay
```

如果功能键在 ScopeDelayMs 到期前按下，普通开镜隐藏不提前执行；仍在计时的
singleShot 到期后会使用最新数据判断。容量从 0 恢复到非 0 时立即按当前右键/
开镜状态重新判断；容量变为 0 时立即隐藏。

`ScopeDelayMs` 只用于开镜隐藏，不改变容量为 0 的强制隐藏规则。

## 6. 保存与退出

### 读取

- 启动只调用一次 `loadOnce()`；读取 `%APPDATA%\BigA\user-config.json`。
- 读取器只接受当前 schema：所有字段必须存在，布尔/整数/绑定的类型与取值范围必须
  有效；额外字段忽略。
- 文件不存在、JSON 解析失败、字段缺失、类型错误或范围错误时，整体读取失败，
  `ConfigManager` 保持 `UserConfig::defaults()` 并写一条警告日志。
- 不保留旧版本迁移、逐字段默认或部分成功读取逻辑；旧格式文件不会与默认值混合。

### 异步保存

`ConfigManager` 内部有一个专用工作线程，接口：

```cpp
void SaveUserConfigAsync();   // 不抛异常
void waitForPendingSaves();
```

- `SaveUserConfigAsync()` 复制当前 `UserConfig` 快照入队；队列中尚未开始执行的旧
  快照会被最新快照覆盖，正在执行的写完后立即写最新快照。
- 工作线程将 JSON 写入 `user-config.json.tmp`，`flush` 后
  `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` 原子替换。
- 所有异常在入队与工作线程两侧捕获，写 `app.log`，不向主流程抛出。

### 设置关闭

`SettingsWindow::closeEvent`：

1. 若仍在捕获，取消捕获（绑定已在捕获成功时即时更新）。
2. 强制提交仍聚焦的两个参数编辑框。
3. 汇总所有控件为 `UserConfig`。
4. `emit configCommitted(config)`。

`App::onSettingsConfigCommitted`：

1. 规范化容量/白点/延迟/状态显示缩放并写入 `ConfigManager`。
2. 保留由托盘和拖动维护的 `StatusOverlayEnabled/StatusOverlayX/StatusOverlayY`。
3. 三个运行时容量 = 界面容量。
4. `CurrentWeapon=0`，三个弹药状态=0。
5. 立即应用白点大小；若状态窗口已创建，应用新缩放并刷新内容、约束位置。
6. 停止容量防抖计时器。
7. `SaveUserConfigAsync()`。

### 退出

托盘“退出”调用 `App::requestQuit()`：

1. 若设置窗口可见，先 `close()`，走设置关闭提交与保存。
2. 关闭状态显示窗口。
3. 若容量防抖仍在计时，停止、同步容量、再调用保存。
4. `waitForPendingSaves()` 等待所有保存结束；保存失败也已记录，不阻断退出。
5. 注销 RawInput，`QCoreApplication::quit()`。

`App` 析构时再执行一次可见设置窗口关闭、状态窗口关闭、挂起防抖提交与等待，
覆盖非托盘退出路径。

## 7. 单实例

`main()` 在创建 `QApplication` 前：

```cpp
SetLastError(ERROR_SUCCESS);
CreateMutexW(nullptr, FALSE, L"Local\\BigA.SingleInstance.<GUID>");
GetLastError() == ERROR_ALREADY_EXISTS -> 直接 return 0
```

显式清除 LastError，避免 `CreateMutexW` 新建对象时残留上次 Win32 错误导致误判。
