# 实现参数与平台映射

## Web

以下参数来自 OMNI 工作台的 `createStatusRefraction` 和 `.m5-status-tooltip`。截图只作为观察样本，数值以源码为准。

| 层 | OMNI 实现 |
| --- | --- |
| 几何 | 圆角 20 CSS px，横向内边距 16 px、纵向 12 px，最大宽 320 px，距视口至少 12 px |
| 表面 | 135° 渐变，白色约 7% → 1%，尾端淡蓝约 4%。正文为 `#24343b`，12 px，1.65 行高 |
| 倒角 | 圆角矩形 SDF，内部 12 px 以内沿法线折射，权重 `pow(clamp(1 + d / 12, 0, 1), 1.4)`，其中 `d ≤ 0` 表示内部 |
| 背景 | 用 Canvas 生成 RG 位移贴图，R 存水平、G 存垂直。中心接近中性灰。SVG 位移的 RGB scale 为 −18 / −15 / −12，分别取对应通道后合成，再接 1 px 模糊与 120% 饱和度 |
| 亮边 | 用遮罩只保留约 1 px 宽的环，左上白、右下淡蓝、左侧粉色都用径向渐变淡出 |
| 阴影 | `0 4px 14px #1b394212`。低透明度、软边，下沿脱开背景 |
| 浮起 | 内容放在 top-layer popover。锚点上方间隔 10 px。340 ms，从下方 5 px、缩放 `(.94,.90)` 进入，65% 时轻微超调，最后回正 |

SVG 的 scale 是乘数。贴图通道归一化后先减 0.5，因此参考红色通道的满幅单轴位移约 9 CSS px，绿色约 7.5 px，蓝色约 6 px。位移只作用于背后像素。改变容器尺寸时重建贴图，同尺寸缓存复用。需要兼容性回退时，先声明低强度 `blur()` 和 `saturate()`，再在验证过的目标环境启用 SVG 滤镜。无法采样背景时可略微提高表面色的不透明度。[SVG 位移计算规范](https://drafts.fxtf.org/filter-effects/#feDisplacementMapElement)

如果同屏有多个气泡实例，滤镜 ID 与缓存按实例或尺寸区分。正文、边缘高光和背景位移应能独立关闭以供对照。状态文本换行后重新测量尺寸，减少动态效果模式下免动画。

## Unreal Engine

本仓库的 [Slate 渲染入口](../../../Plugins/LiquidGlassUI/Source/LiquidGlassUI/Private/SLiquidGlass.cpp) 已复制玻璃背后的画面，[LiquidGlass.usf](../../../Plugins/LiquidGlassUI/Shaders/Private/LiquidGlass.usf) 已用圆角 SDF 做 coverage、边缘法线、折射与 RGB 分通道采样。沿用该架构即可制作小气泡。

| OMNI 特征 | 在 UE 现有实现中的落点 |
| --- | --- |
| 中央透明、边缘折射 | 先从最低的几档 BlurRadius 调起，保持中央背景细节。用 `BevelWidth` 限定边缘区域，再调 `Refraction` 和 `Dispersion` |
| RGB 轻微分离 | HLSL 中绿色是基准，红和蓝取基准两侧。可参考满幅中心约 7.5 px、通道差约 1.5 px 的量级，实际按 Slate 几何和视口 DPI 换算，再以网格验证偏移方向 |
| 左上亮、右下微蓝、左侧少量粉 | 现有 shader 的相对光向采用绝对点积，默认高光成对对称。优先用一层独立的细边遮罩复现非对称高光。需要把效果纳入 shader 时，再加入可控的局部高光位置和颜色 |
| 阴影与清晰文字 | 在玻璃后方加低 alpha 的软阴影，避免误采气泡自身阴影造成双重暗边。文字独立绘制在最上层，留出内边距和换行宽度 |
| 悬浮与锚点 | 用 UMG Render Transform 或 Slate 动画控制少量位移和缩放，枢轴靠近底部锚点。让动画控制外观，定位单独处理 |

建议在相同背景和相同文案下做两个运行中截图。先确认取样范围包括倒角位移、色散和模糊可能访问的区域，再检查 DPI、窗口缩放、裁切边界与拖动时的背景错位。纯色能验收亮边、浅底和阴影，网格才能给出折射方向、影响范围以及中央是否稳定的证据。

普通 UMG Background Blur 可用于低成本回退，提供模糊层。Retainer Box 的默认渲染目标来自它的子控件，使用它之前先确认能取得所需背景。本仓库的屏幕绘制路径另见[插件参数和平台边界](../../../Plugins/LiquidGlassUI/README.md)。

- [Epic · Background Blur Widget](https://dev.epicgames.com/documentation/unreal-engine/using-the-background-blur-widget-in-unreal-engine)
- [Epic · Using Slate postbuffers](https://dev.epicgames.com/documentation/unreal-engine/using-slate-postbuffers-in-unreal-engine)
