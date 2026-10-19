# 设置窗口布局

依据工作区中的布局要求实现。原文写“内容纵向分为以下 5 组”，但只列出了组 1~组 4；代码按实际列出的四组实现，未臆造第五组。

## 1. 布局树

```text
SettingsWindow (QWidget, frameless, fixed width 720, height=sizeHint)
└─ QVBoxLayout (margins 0, spacing 0)
   ├─ FramelessTitleBar (height 38)
   │  └─ QHBoxLayout (margins 0)
   │     ├─ addStretch
   │     └─ QPushButton "×" (44×38, objectName=closeButton)
   └─ settingsContent (QWidget)
      └─ QVBoxLayout (margins 20,16,20,20; spacing 16)
         ├─ QFrame sectionCard "切换与重置"
         │  └─ QVBoxLayout (margins 14,12,14,14; spacing 10)
         │     ├─ QLabel "切换与重置" (sectionTitle)
         │     ├─ QGridLayout
         │     └─ ...
         ├─ QFrame sectionCard "武器热键"
         ├─ QFrame sectionCard "参数"
         ├─ QFrame sectionCard "容量增减热键"
         └─ addStretch(1)
```

## 2. QGridLayout 草图

统一列宽：

```text
col0 标签 104
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
row0         切换        ToggleKey1  ToggleKey2
row1         重置/恢复   ResetKey
```

### 组 2：武器热键（容量分别在第 1、2、4 行）

```text
       col0        col1          col2          col3       col4
row0   一号武器    Weapon1Key1   Weapon1Key2   一号容量   Weapon1Capacity
row1   二号武器    Weapon2Key1   Weapon2Key2   二号容量   Weapon2Capacity
row2   刀          KnifeKey1     KnifeKey2
row3   手枪        PistolKey1    PistolKey2    手枪容量   PistolCapacity
```

### 组 3：参数

```text
       col0             col1
row0   白点大小         DotSize
row1   开镜延迟(ms)     ScopeDelay
```

### 组 4：容量增减热键

```text
       col0        col1
row0   容量+1      CapacityIncKey
row1   容量-1      CapacityDecKey
```

## 3. 尺寸与间距

| 项目 | 尺寸 |
| --- | --- |
| 窗口宽度 | 720px |
| 标题栏高度 | 38px |
| 关闭按钮 | 44×38px |
| 内容左右边距 | 20px |
| 内容上下边距 | 16 / 20px |
| 分组间距 | 16px |
| 卡片内边距 | 14 / 12 / 14 / 14px |
| 标签列 | 104px，右对齐、垂直居中 |
| 键入框 | 156×32px（符合 140~180px 建议） |
| 容量标签 | 76px，右对齐 |
| 数字框 | 108×32px（符合 100~140px 建议） |
| 行间距 | 8px |

窗口高度由 `sizeHint()` 计算，默认在**主屏可用区域**居中。拖动只由标题栏空白区域发起；按钮不参与拖动。

## 4. 键入框状态与样式

### 正常 / 空绑定

- 只读 `QLineEdit`，`Qt::NoFocus`，鼠标指针为手型。
- 有绑定：显示 `KeyBinding::displayName()`，例如 `B`、`小键盘 5`、`鼠标中键`、`滚轮上`。
- 空绑定：显示“未绑定”，样式颜色 `#74859a`。
- 原生右键上下文菜单禁用。

### 捕获态

- 鼠标左键点击进入捕获态。
- 文本显示“请按键…”，边框切换为 2px 蓝色高亮，背景变深蓝。
- `capturing=true` 动态属性触发 QSS 切换。
- 捕获到合法单键后立即退出捕获，写入新显示值。
- Esc 取消，恢复原值。
- 右键清空当前绑定并退出捕获。
- 捕获态截图样式中：

```css
QLineEdit#bindingEdit[capturing="true"] {
    background-color: #0b2a3f;
    border: 2px solid #52b8ff;
    color: #e9f7ff;
}
```

## 5. 数字输入框焦点/失焦

### 三个容量框（`CapacityEdit`，0~9）

- 聚焦：全选当前数字。
- 数字键 0~9：立即写入该数字并 `clearFocus()`。
- 其他按键：吞掉，不修改，保持焦点继续等待。
- Tab / Shift+Tab：正常焦点链。
- Enter：失焦。
- 不做失焦修正；全选未输入直接失焦时原值保留。
- 宽度 108，内容长度 1。

### 白点大小 / 开镜延迟（`ParameterEdit`，非负整数）

- 聚焦：全选；记录进入焦点时的原值。
- 输入中只拒绝非数字字符，不做范围检查、不夹取、不修正。
- Tab / Shift+Tab：走正常焦点链，`focusOutEvent` 提交。
- Enter：提交并失焦。
- 点击其他可聚焦控件：失焦提交。
- 关闭窗口：即使尚未失焦也强制提交。
- 空、解析失败、负数：恢复进入焦点时的原值。
- 合法：接受该非负整数并规范化显示。

## 6. 标题栏

```text
┌──────────────────────────────────────────────┬────┐
│  （空白拖动区域，无标题文字）                │ ×  │
└──────────────────────────────────────────────┴────┘
```

- 仅一个关闭按钮，无最小化/最大化。
- 无“确定”“取消”“应用”按钮。
- 左键按住标题栏空白区，记录 `globalPosition - frameGeometry.topLeft()`，移动时调用 `window()->move()`。
- 标题栏高度 38px，关闭按钮悬停为红底白字。
