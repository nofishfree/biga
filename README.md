# BigA

Windows 透明点击穿透准星/图片叠层工具。C++20 + Qt 6 Widgets + CMake，全局输入通过 Win32 Raw Input（`WM_INPUT`）读取。

## 构建（静态单文件）

要求：

- Windows 10/11 x64
- Visual Studio 2022/2026，安装“使用 C++ 的桌面开发”
- CMake 3.21+
- **静态 Qt 6.8.3 MSVC x64**，本机位置：

```text
D:\Qt\6.8.3\msvc2022_64_static
```

静态 Qt 使用 `-static -static-runtime` 构建，应用程序以 `/MT` 静态链接。
CMake 会自动检测静态 Qt，并嵌入 Windows 平台、样式以及 GIF/ICO/JPEG 插件。

```powershell
cmake -S . -B build/build-static -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_PREFIX_PATH="D:/Qt/6.8.3/msvc2022_64_static"
cmake --build build/build-static --config Release --parallel
```

产物是**单个 exe**：

```text
build\build-static\Release\BigA.exe
dist\BigA.exe
```

不需要 `windeployqt`，也无需复制 Qt DLL 或 VC++ 运行库。可把 `BigA.exe`
直接复制到其他目录或其他 Windows 10/11 x64 电脑运行。

注意：对外分发静态链接的 LGPL Qt 需要遵守 LGPL 的重新链接等义务，或使用商业授权。

### 运行位置说明

在当前的 DSH 工作区环境中，直接从 `E:\workspace\biga` 内启动的进程可能被工作区
沙箱降为 Low Integrity；Windows 不允许这类进程注册通知区域图标，因此托盘图标
不会出现。请把单文件 exe 复制到工作区外再运行，例如：

```text
C:\Users\Administrator\AppData\Local\Programs\BigA\BigA.exe
```

工程已部署到该目录，并创建了桌面和开始菜单的 `BigA` 快捷方式。重新编译后
只需把单个 `BigA.exe` 覆盖到上述部署目录。

## 数据位置

- 配置：`%APPDATA%\BigA\user-config.json`
- 当前图片：`%APPDATA%\BigA\current-image.img`（托盘“还原白点”会删除该文件）
- 日志：`%APPDATA%\BigA\app.log`

配置使用临时文件 + `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` 原子替换，并只在启动时读取一次。读取器只接受当前 schema；文件缺失、字段缺失或类型/范围不兼容时整体使用代码默认值，不做旧版本迁移或部分字段兜底，并立即把默认配置写回同一文件（旧格式文件被覆盖）。图片路径不写入配置，配置里也没有任何图片数据；图片只存在于 `current-image.img`，还原白点即删除它，因此关闭再启动不会再显示旧图片。固定文件按内容识别格式，GIF 只显示首帧。

## 状态显示窗口

托盘菜单“状态显示”可勾选一个独立的透明置顶小窗口：

- 显示当前武器编号，右上上标为弹药状态，右下下标为容量；刀只显示大号 3。
- 左键单击托盘图标显示/隐藏状态窗口；双击仍打开设置，不会误切换状态。
- 鼠标左右键**同时按下**并拖动才移动窗口；只按一个键、单击、滚轮等一律穿透到
  下面的窗口。可拖动判定区是窗口居中的固定 50×50 方块。
- 窗口位置在拖动结束时保存；拖动过程中任一键抬起即结束拖动。
- 默认显示在主屏右下角；位置不在任何屏幕时回退右下角。
- 设置界面的“参数”组提供 0.5~3.0、步进 0.1 的缩放滑块，拖动实时预览，
  设置关闭时保存。
- 开关状态、位置、缩放分别保存在 `statusOverlayEnabled`、
  `statusOverlayX/Y`、`statusOverlayScale` 配置字段中。
- 状态窗口在 GUI 主线程延迟创建；创建失败只记录日志，不影响主功能，
  再次勾选托盘“状态显示”可重试。

## 主窗口可见性

- 隐藏只由“按住右键”触发：按住右键期间按 `ScopeDelayMs` 与弹药状态/容量判断是否
  隐藏，松开右键一律恢复显示。
- 运行时容量为 0（或弹药状态为 0）时不再“始终隐藏”：按下右键期间保持隐藏，
  抬起右键立即显示，与其他武器一致。
- 状态显示窗口不受主窗口隐藏规则影响。

## 图标

`resources/app.png` 用于托盘图标，`resources/app.ico` 用于可执行文件和图标回退。两者当前都是根据界面风格生成的占位图标。任务要求使用用户提供的 `.ico`，但工作区中未包含该文件；将原始图标覆盖保存为上述文件后重新构建即可。

## 文档

- [架构与实现方案](docs/architecture.md)
- [设置窗口布局](docs/settings-layout.md)
- [构建与运行验证](docs/verification.md)

## 目录速览

```text
CMakeLists.txt
dist/
  BigA.exe
resources/
  app.ico
  app.png
  app.rc
  resources.qrc
  styles/settings.qss
src/
  main.cpp
  App.*
  AppPaths.*
  ConfigManager.*
  Constants.h
  KeyBinding.*
  Log.*
  MainOverlayWindow.*
  RawInputManager.*
  RuntimeState.h
  SettingsWindow.*
  StatusOverlayWindow.*
  TrayController.*
  UserConfig.*
  widgets/
    CapacityEdit.*
    FramelessTitleBar.*
    KeyBindingEdit.*
    ParameterEdit.*
docs/
  architecture.md
  settings-layout.md
```
