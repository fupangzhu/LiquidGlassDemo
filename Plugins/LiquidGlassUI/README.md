# Liquid Glass UI — UE 5.8

支持 Slate 与 UMG 的液态玻璃容器，支持背景模糊、折射、色散与可调方向高光。

## 使用

1. 将 `LiquidGlassUI` 文件夹放入 C++ 工程的 `Plugins` 目录，启用插件并完整编译、重启编辑器。全局 Shader 模块必须在 `PostConfigInit` 加载，不支持用 Live Coding 首次安装。
2. UMG Palette → **Liquid Glass → Liquid Glass Panel**。把 Overlay、Horizontal Box 或按钮等放到容器内部；多个子控件通过一个布局容器组合。
3. Details 面板设置 `GlassStyle` 与 `ContentPadding`；运行时调用蓝图 `SetGlassStyle` / `SetContentPadding`。
4. 在本示例中，`SLiquidGlassDemo` 演示 Slate 用法，`GlassDemoGameMode.cpp` 演示真实 UMG 容器与子按钮。

Slate 示例（使用方 Build.cs 需要依赖 `LiquidGlassUI`）：

```cpp
#include "SLiquidGlass.h"

FLiquidGlassStyle Style;
Style.CornerRadius = 28.f;
Style.BlurRadius = 12.f;
Style.Refraction = 9.f;
Style.Dispersion = 1.2f;

SNew(SLiquidGlass).GlassStyle(Style).Padding(16.f)
[
    SNew(STextBlock).Text(FText::FromString(TEXT("Clear text")))
];
```

## 参数

| 参数 | 含义 |
|---|---|
| CornerRadius | 圆角，自动限制到短边的一半 |
| BlurRadius | 背景高斯模糊的近似支持半径；0 关闭模糊 |
| Refraction | 玻璃倒角附近的最大背景偏移量；0 关闭折射 |
| Dispersion | 红蓝采样分离程度；0 关闭色散 |
| BevelWidth | 边缘折射影响范围 |
| Tint | 染色，Alpha 表示染色比例 |
| RimIntensity | 两道对向高光强度，默认 0.85 |
| LightAngleDegrees | 屏幕空间光照轴；0 / 180 度时上下亮、左右暗，90 度时左右亮、上下暗 |
| RimWidth | 亮边宽度（Slate 单位），默认 1.5，范围 0.25–8 |
| SideShadow | 细灰轮廓强度，默认 0.30；侧边硬边及底部细线，不向内扩散 |
| InnerShadow | 玻璃内侧阴影 |
| Opacity | 整体效果强度，与父控件透明度相乘 |
| BlurDownsample | 模糊降采样 1–4；默认 2 |

距离使用控件本地 Slate 单位，随布局 / DPI 比例缩放。圆角外不绘制玻璃。内容仍由 Slate 正常绘制与命中测试，圆角遮罩仅裁玻璃表面，不自动裁内部子控件；使用 ContentPadding 安排内容。

## 渲染路径

`当前 Slate 层已绘制的背景 → 带采样边距的局部 GPU 复制 → 降采样、水平/垂直高斯模糊 → 圆角 SDF 折射 / RGB 色散 / tint / 高光 / 内阴影合成 → 子控件`

- 使用 UE 5.8 `ICustomSlateElement::Draw_RenderThread` 和 Render Dependency Graph，不修改引擎，不使用 SceneCapture2D，不使用上一帧整屏快照。
- 普通运行无 GPU → CPU 像素回读；截图回读仅用于自动化验收。
- 同一窗口中后绘制的玻璃可以采样已绘制的底层 UI / 玻璃。需由 UI 层级明确控制前后关系。
- 每个控件复制自身可见包围框加采样边距；模糊仅处理局部区域。跳过零尺寸、不可见或透明度为零的玻璃。
- 游戏线程到渲染线程通过排队命令传递值，Shader 参数不读取 UObject；资源由 RDG 管理。
- 使用实际 RenderTransform 反变换计算玻璃坐标，处理圆角、缩放与旋转；用裁剪半平面处理父级剪裁。超出可支持的裁剪栈深度时回退原生染色圆角。
- 控制台 `LiquidGlass.Enabled 0` 切换低成本染色圆角；`LiquidGlass.Enabled 1` 恢复。

## 验证与边界

本轮目标为 Windows、UE 5.8.2、D3D12 SM6、SDR 的屏幕 UI。其他 UE 版本、Vulkan、移动端、HDR、VR 双眼、世界空间 WidgetComponent、Retainer Box 离屏路径未验收；离屏绘制只能采样自己的输出目标，不能承诺采到其外部背景。

性能随屏幕覆盖面积、玻璃层数、模糊半径和降采样变化。截图测试帧率不代表 GPU 性能基准。后续应通过 Unreal Insights / GPU profiler 在目标设备上测量 `LiquidGlass` RDG 事件。

示例验收入口：工程根目录的 `Build-Sample.ps1 -Action Test`；打包版加 `-Packaged`。报告在工程 `Validation` 目录。

## 0.2.0 方向高光

采用双向环境反射近似，让上下两条亮边同时出现，圆角连续过渡到两侧暗部。默认光照轴为 0 度；旋转该参数可改变亮边方向。轴位于屏幕空间，控件旋转不会带着光源一起转动。这是风格近似，并非 Apple 系统材质源码。
