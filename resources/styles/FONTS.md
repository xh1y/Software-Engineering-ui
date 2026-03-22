# OptiKG 字体配置指南

## 已配置的字体优先级

样式系统已配置以下字体回退链：

```css
font-family: "PingFang SC",           /* macOS - 苹方 (首选) */
             "Hiragino Sans GB",       /* macOS - 冬青黑体 (备选) */
             "Microsoft YaHei",        /* Windows - 微软雅黑 */
             "WenQuanYi Micro Hei",    /* Linux - 文泉驿微米黑 */
             "Noto Sans SC",           /* Linux - Google 思源黑体 */
             -apple-system,            /* macOS 系统字体 */
             BlinkMacSystemFont,       /* macOS Chrome */
             "Segoe UI",               /* Windows 系统字体 */
             sans-serif;
```

## 各平台最佳字体

### macOS
- **PingFang SC (苹方)** - 现代、清晰、屏幕显示优化
- 已预装，无需额外安装

### Windows  
- **Microsoft YaHei (微软雅黑)** - Windows Vista+ 默认中文字体
- 已预装，无需额外安装

### Linux
推荐安装以下字体之一：

#### 1. 思源黑体 (Noto Sans CJK / Source Han Sans) - 推荐
```bash
# Arch Linux
sudo pacman -S noto-fonts-cjk

# Ubuntu/Debian
sudo apt install fonts-noto-cjk

# Fedora
sudo dnf install google-noto-sans-cjk-fonts
```

#### 2. 文泉驿微米黑
```bash
# Arch Linux
sudo pacman -S wqy-microhei

# Ubuntu/Debian
sudo apt install ttf-wqy-microhei
```

## 字体效果预览

| 字体 | 特点 | 适用场景 |
|------|------|----------|
| 苹方 | 现代简洁、字重丰富 | macOS 首选 |
| 思源黑体 | 开源、多语言支持 | Linux 首选 |
| 微软雅黑 | 清晰易读 | Windows 默认 |

## 验证字体安装

```bash
# 检查已安装的中文字体
fc-list :lang=zh | grep -E "(PingFang|Noto|Source|Microsoft|WenQuanYi)"
```

## 字体特点

本设计选用 **PingFang SC (苹方)** 作为首选字体，因为它：

1. **现代感强** - 几何化设计，符合工业软件的科技感
2. **清晰度高** - 针对 Retina 屏幕优化
3. **字重丰富** - 支持 Light/Regular/Medium/Semibold/Bold
4. **专业感** - 被 Apple 全系产品采用，具有专业形象

如果苹方不可用，会依次回退到其他高质量黑体字体。
