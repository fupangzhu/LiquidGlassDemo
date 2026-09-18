# Liquid Glass UI

UE 5.8 的 UMG / Slate 液态玻璃控件示例。背景实时模糊与折射，文字和按钮保持清晰。

![动态背景](docs/live-glass.gif)
![网格折射](docs/grid-refraction.gif)
![文字背景](docs/text-glass.gif)

## 启动

1. 安装 **Unreal Engine 5.8** 和支持 UE 的 **Visual Studio C++ 工具链**。
2. 下载源码，解压到英文路径。右键 `LiquidGlassDemo.uproject` 生成 Visual Studio 工程，编译 **Development Editor / Win64**。
3. 双击 `LiquidGlassDemo.uproject`，进入编辑器后点击 **Play**。

已有编译结果时，也可以双击 `Start-Demo.cmd` 直接运行示例。若有 Windows 打包版，直接双击 `LiquidGlassDemo.exe`。

拖动玻璃卡片，或在右侧调整模糊、折射和光照。默认上下亮、两侧为细灰边；**Reset all** 恢复默认效果。

要在自己的工程使用，将 `Plugins/LiquidGlassUI` 复制到工程的 `Plugins` 目录并编译，在 UMG 中添加 **Liquid Glass Panel** 即可。详见[插件用法](Plugins/LiquidGlassUI/README.md)。

当前验证平台：Windows / D3D12 / SDR。