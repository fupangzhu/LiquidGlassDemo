---
name: reproduce-floating-glass
description: Reproduce the OMNI floating-glass status tooltip in Web or Unreal Slate/UMG. Use when matching a small glass tooltip or popover, tuning edge-only refraction, or diagnosing a heavy, flat, or overly blurred glass layer.
---

# 复现悬浮玻璃气泡

以 [OMNI 截图](../../docs/omni-floating-glass.png) 为视觉基准。气泡正文清晰，玻璃中央接近背景原色，变形集中在轮廓附近。亮边和阴影让气泡在近乎纯色的背景上也能成立。

1. **拆开层次。** 按背景采样、折射倒角、表面色、细亮边与阴影、前景文字的顺序合成。先在纯色背景上把轮廓和阴影调到位，再用细网格或字形背景调折射。前景文字压在玻璃层上方，保持正常字重和清晰度。
2. **把折射限制在边缘。** 用圆角矩形的带符号距离和外向法线做位移，平坦中心的位移归零。RGB 共享同一条倒角曲线，微调各通道采样位置。按容器实际尺寸计算圆角和边缘宽度。
3. **按目标环境落地。** Web 读取[实现参数与回退](references/implementation.md#web)。UE 优先沿用仓库里的 Slate 屏幕背景采样和现有 shader，读取 [UE 映射](references/implementation.md#unreal-engine)。确认采集到的是气泡后方、气泡绘制前的画面。
4. **让气泡跟随锚点。** 悬停和键盘焦点都能显示。位置优先在锚点上方，沿屏幕边缘约束。展现时从靠近锚点的底部轻微弹出。启用减少动态效果时直接显示。
5. **用三种背景验收。** 纯色检查轮廓和浮起感，网格检查只有倒角弯折，高对比文字检查折射对象和文字清晰度。再用长文本、不同 DPI、贴边锚点和重复开关检查布局及资源生命周期。

工作开始前记录原效果和对照参数。结束时交付同背景、同尺寸、同文字的截图，并说明验证过的渲染路径。遇到纯色背景上看不出折射时，切到网格测量，避免为了纯色截图加重模糊或色散。
