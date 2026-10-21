# 设置窗口布局

依据 `dev_docs/设置界面布局.txt` 重排。原文有一处“内容纵向分为以下 2 组”的
笔误，后面实际列出了 3 个分组，代码按 3 个分组实现。

## 1. 布局树

```text
SettingsWindow (QWidget, frameless, fixed width 760, height=sizeHint)
└─ QVBoxLayout (margins 0, spacing 0)
   ├─ FramelessTitleBar (height 38)
   │  └─ QHBoxLayout
   │     ├─ addStretch
   │     └─ QPushButton "×" (44×38, objectName=closeButton)
   └─ settingsContent
      └─ QVBoxLayout (margins 20,16,20,20; spacing 16)
         ├─ sectionCard "切换与重置"
         ├─ sectionCard "武器热键"
         ├─ sectionCard "参数"
         └─ addStretch(1)
```

## 2. QGridLayout 草图

统一列宽：

```text
col0 标签 128
col1 键入框 156
col2 键入框 156
col3 容量标签 76
col4 数字框 108
col5 伸缩列
水平间距 10，垂直间距 8
```

### 组 1：切换与重置

```text
       col0        col1        col2
row0   切换        ToggleKey1  ToggleKey2
row1   重置/恢复   ResetKey
row2   白点大小    DotSize
```

### 组 2：武器热键（容量在第 1、2、4 行；容量增减在本组末尾）

```text
       col0        col1          col2          col3       col4
row0   一号武器    Weapon1Key1   Weapon1Key2   一号容量   Weapon1Capacity
row1   二号武器    Weapon2Key1   Weapon2Key2   二号容量   Weapon2Capacity
row2   刀          KnifeKey1     KnifeKey2
row3   手枪        PistolKey1    PistolKey2    手枪容量   PistolCapacity
row4   容量+1      CapacityIncKey
row5   容量-1      CapacityDecKey
```

### 组 3：参数

```text
       col0                col1                       col2~col4
row0   开镜延迟(ms)        ScopeDelay
row1   状态显示缩放比例    QSlider (colspan=4) ->     [1.0]
```

缩放滑块：

- 范围 0.5~3.0
- 步进 0.1（QSlider 内部使用 5~30 的整数刻度）
- 默认 1.0
- 右侧数值标签固定显示一位小数
- 拖动时实时发出预览信号；只有设置窗口关闭时才写入配置

## 3. 尺寸与间距

| 项目 | 尺寸 |
| --- | --- |
| 窗口宽度 | 760px |
| 标题栏高度 | 38px |
| 关闭按钮 | 44×38px |
| 内容左右边距 | 20px |
| 内容上下边距 | 16 / 20px |
| 分组间距 | 16px |
| 卡片内边距 | 14 / 12 / 14 / 14px |
| 标签列 | 128px，右对齐、垂直居中 |
| 键入框 | 156×32px |
| 容量标签 | 76px，右对齐 |
| 数字框 | 108×32px |
| 缩放数值标签 | 44px |
| 行间距 | 8px |

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
