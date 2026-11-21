# 设置窗口布局

本窗口按最初的需求稿实现，之后的调整如下：取消独立的“切换与重置”分组，
其内容全部并入“参数”；“容量+1/容量-1”也从“武器热键”移入“参数”；“参数”
新增“状态+1/状态-1”（默认 `→`/`←`）。界面纵向只剩两个分组卡片。2026-11-21
又按“够用就行”整体压缩了控件与窗口尺寸，并把武器热键的容量标签统一为“容量”、
“重置/恢复”简化为“重置”，参数栏的容量与状态改成左右两列成对排列。

## 1. 布局树

```text
SettingsWindow (QWidget, frameless, fixed width 540, height=sizeHint)
└─ QVBoxLayout (margins 0, spacing 0)
   ├─ FramelessTitleBar (height 30)
   │  └─ QHBoxLayout
   │     ├─ addStretch
   │     └─ QPushButton "×" (38×30, objectName=closeButton)
   └─ settingsContent
      └─ QVBoxLayout (margins 12,10,12,12; spacing 12)
         ├─ sectionCard "武器热键"
         ├─ sectionCard "参数"
         └─ addStretch(1)
```

## 2. QGridLayout 草图

两张卡片的列宽不完全相同：`武器热键` 右侧要放容量标签和容量数字框，
`参数` 每行并排放两组“标签 + 控件”，因此把第 2 列当标签列、第 3 列当控件列。

### 组 1：武器热键（容量数字框在第 1、2、4 行，容量标签统一显示“容量”）

```text
       col0        col1          col2          col3       col4
row0   一号武器    Weapon1Key1   Weapon1Key2   容量       Weapon1Capacity
row1   二号武器    Weapon2Key1   Weapon2Key2   容量       Weapon2Capacity
row2   刀          KnifeKey1     KnifeKey2
row3   手枪        PistolKey1    PistolKey2    容量       PistolCapacity
```

列宽：col0 标签 110、col1/col2 键入框 116、col3 容量标签 40、col4 数字框 76。

### 组 2：参数（每行两组：标签 + 控件；左列容量、右列状态）

```text
       col0              col1              col2              col3
row0   切换              ToggleKey         重置              ResetKey
row1   容量+1            CapacityIncKey    状态+1            AmmoStateIncKey
row2   容量-1            CapacityDecKey    状态-1            AmmoStateDecKey
row3   白点大小          DotSize           开镜延迟(ms)      ScopeDelay
row4   状态显示缩放比例  QSlider (colspan=3) ->                [1.0]
```

列宽：col0/col2 标签 110、col1/col3 控件 116（数字框固定 76 宽，左对齐落在
116 宽的列里）。水平间距 8，垂直间距 6。

缩放滑块：

- 范围 0.5~3.0
- 步进 0.1（QSlider 内部使用 5~30 的整数刻度）
- 默认 1.0
- 右侧数值标签固定显示一位小数
- 拖动时实时发出预览信号；只有设置窗口关闭时才写入配置

## 3. 尺寸与间距

| 项目 | 尺寸 |
| --- | --- |
| 窗口宽度 | 540px |
| 窗口高度 | 约 422px（由 sizeHint 计算） |
| 标题栏高度 | 30px |
| 关闭按钮 | 38×30px |
| 内容左右边距 | 12px |
| 内容上下边距 | 10 / 12px |
| 分组间距 | 12px |
| 卡片内边距 | 10 / 8 / 10 / 10px |
| 标签列 | 110px，右对齐、垂直居中 |
| 键入框 | 116×26px |
| 容量标签 | 40px，右对齐 |
| 数字框 | 76×26px |
| 缩放数值标签 | 34px |
| 行间距 | 6px |

窗口高度由 `sizeHint()` 计算，默认在主屏可用区域居中。标题栏只负责拖动和关闭，
没有标题文字、最小化、最大化。

## 4. 键入框状态与样式

### 正常 / 空绑定

- 只读 `QLineEdit`，`Qt::NoFocus`，手型鼠标指针。
- 有绑定：显示 `KeyBinding::displayName()`，如 `B`、`小键盘 5`、`鼠标中键`、`滚轮上`。
- 空绑定：显示“未绑定”，灰蓝色。
- 禁用原生右键上下文菜单。

### 捕获态

- 鼠标左键点击进入捕获态。
- 文本显示“请按键…”，2px 蓝色高亮边框、深蓝背景。
- 捕获到合法单键后立即退出捕获并写入。
- Esc 取消捕获、恢复原值。
- 右键任意时刻清空当前绑定并退出捕获。
- 重复绑定确认逻辑保持原有行为。

## 5. 数字输入框焦点/失焦

### 三个容量框

- 聚焦全选。
- 数字键 0~9：立即写入并失焦。
- 其他按键吞掉，保持焦点继续等待。
- Tab / Enter：失焦。
- 不做失焦修正；全选未输入直接失焦保留原值。

### 白点大小 / 开镜延迟

- 聚焦全选。
- 输入中只拒绝非数字，不修正范围。
- Tab / Enter / 点击其他控件失焦。
- 空或非法：恢复进入焦点时的原值；合法则接受非负整数。
- 设置关闭时强制提交仍聚焦的编辑框。

## 6. 缩放滑块样式

- `QSlider#statusScaleSlider` 为水平滑块。
- 轨道高 4px，已选区域与拖动柄为蓝色系，滑块柄白色高亮。
- 右侧 `scaleValueLabel` 使用浅蓝色、等宽数字显示当前值，例如 `1.5`。

## 7. 标题栏

```text
┌──────────────────────────────────────────────┬────┐
│  （空白拖动区域，无标题文字）                │ ×  │
└──────────────────────────────────────────────┴────┘
```

- 标题栏空白处左键拖动窗口。
- 只显示关闭“×”，无最小化、最大化。
- 无“确定”“取消”“应用”按钮。
