#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "LiquidGlassStyle.h"

class FLiquidGlassDrawer;

/** Captures at its own Slate layer, then paints children normally above the glass. */
class LIQUIDGLASSUI_API SLiquidGlass : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLiquidGlass) : _GlassStyle(), _Padding(0.f) {}
        SLATE_ARGUMENT(FLiquidGlassStyle, GlassStyle)
        SLATE_ARGUMENT(FMargin, Padding)
        SLATE_DEFAULT_SLOT(FArguments, Content)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    ~SLiquidGlass();
    void SetGlassStyle(const FLiquidGlassStyle& InStyle);
    void SetContent(TSharedRef<SWidget> InContent);
    void SetContentPadding(FMargin InPadding);
protected:
    int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
private:
    FLiquidGlassStyle GlassStyle;
    TSharedPtr<FLiquidGlassDrawer, ESPMode::ThreadSafe> Drawer;
};
