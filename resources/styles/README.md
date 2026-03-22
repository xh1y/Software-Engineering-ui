# OptiKG 现代专业样式系统

## 🎨 设计理念

基于 **Apple HIG**、**Material Design 3**、**Vercel/Stripe** 设计语言：

- ✅ **8pt 网格系统** - 数学和谐的间距
- ✅ **60-30-10 色彩法则** - 平衡的视觉权重
- ✅ **优雅留白** - 让界面呼吸
- ✅ **柔和阴影** - 微妙的深度感
- ✅ **专业字体** - PingFang SC / 思源黑体

---

## 📁 主题列表

| 主题 | 特点 | 背景色 |
|------|------|--------|
| **Modern Pro** (默认) | 浅灰白 + 靛蓝强调 | #fafafa |
| **Modern Dark Pro** | 深灰(**非纯黑!**) + 亮靛蓝 | #171717 |
| **Warm Beige** | 米色暖调 + 橙色 | #faf8f5 |
| **Material Light** | Material Design 3 浅色主题 + 紫色主调 | #FFFBFE |
| **Material Dark** | Material Design 3 深色主题 + 浅紫色主调 | #1C1B1F |

---

## 🚀 使用方法

### 切换主题
```
菜单栏 → 视图(V) → 主题(T) → 选择主题
```

### 快捷键
- 现代专业: 点击菜单项
- 现代深色: 点击菜单项
- 温暖米色: 点击菜单项

---

## 🎯 设计系统

### 色彩
```css
/* 强调色 - 靛蓝 */
--color-primary-600: #4f46e5;

/* 背景 */
--color-gray-50:  #fafafa;  /* 页面背景 */
--color-gray-100: #f5f5f5;  /* 卡片背景 */
--color-gray-200: #e5e5e5;  /* 边框 */

/* 文字 - 避免纯黑 */
--color-gray-700: #404040;  /* 正文 */
--color-gray-800: #262626;  /* 标题 */
--color-gray-900: #171717;  /* 最深 */
```

### 间距 (8pt 网格)
```css
--space-2: 8px;
--space-3: 12px;
--space-4: 16px;
--space-5: 20px;
--space-6: 24px;
```

### 圆角
```css
--radius-md: 6px;   /* 输入框 */
--radius-lg: 8px;   /* 按钮 */
--radius-xl: 12px;  /* 卡片 */
```

### 阴影
```css
--shadow-md: 0 4px 6px -1px rgba(0,0,0,0.1);
--shadow-lg: 0 10px 15px -3px rgba(0,0,0,0.1);
```

### 字体
```css
font-family: "PingFang SC",           /* macOS 首选 */
             "Hiragino Sans GB",       /* macOS 备选 */
             "Microsoft YaHei",        /* Windows */
             "WenQuanYi Micro Hei",    /* Linux */
             "Noto Sans SC",           /* Linux 备选 */
             sans-serif;
```

详见 [FONTS.md](FONTS.md)

---

## 🧩 组件规范

### 按钮
| 类型 | ObjectName | 用途 |
|------|------------|------|
| 主要 | `extractBtn`, `primaryBtn` | 蓝色，主要操作 |
| 危险 | `clearBtn`, `deleteBtn`, `dangerBtn` | 红色，删除/清空 |
| 成功 | `successBtn` | 绿色，成功操作 |
| 次要 | `secondaryBtn` | 灰色边框，次要操作 |

### 标签
| ObjectName | 用途 |
|------------|------|
| `titleLabel` | 大标题 (18px, bold) |
| `subtitleLabel` | 小标题 (16px, semibold) |
| `descriptionLabel` | 描述文字 |
| `hintLabel` | 辅助提示 |
| `charCountLabel` | 字符计数 |

### 特殊状态文字
| ObjectName | 颜色 |
|------------|------|
| `successText` | 绿色 |
| `warningText` | 橙色 |
| `errorText` | 红色 |
| `infoText` | 蓝色 |

---

## 📝 自定义主题

### 步骤
1. 复制 `resources/styles/modern_pro.qss`
2. 修改 Design Tokens (颜色、间距、圆角)
3. 在 `resources.qrc` 中注册
4. 在 `mainwindow.cpp` 添加菜单项
5. 重新构建

### 设计检查清单
- [ ] 使用 8pt 网格
- [ ] 避免 #000000 和 #FFFFFF
- [ ] 按钮有 Hover/Pressed 状态
- [ ] 输入框有 Focus 光环
- [ ] 圆角保持一致
- [ ] 阴影柔和

---

## 💡 口诀

> "**对齐留白建骨架，少色多灰定基调；**
>  **字重拉开对比度，微小圆角配轻影。**"

---

## 🔧 故障排除

### 主题切换不生效
1. 检查资源文件路径是否正确
2. 查看调试输出：`qDebug()` 信息
3. 确保样式文件已正确嵌入资源

### 字体显示不正确
1. 检查系统是否安装了对应字体
2. 查看 [FONTS.md](FONTS.md) 安装指南
3. 使用 `fc-list :lang=zh` 检查字体

---

## 📚 参考

- [Apple HIG](https://developer.apple.com/design/human-interface-guidelines/)
- [Material Design 3](https://m3.material.io/)
- [Vercel Design](https://vercel.com/design)
