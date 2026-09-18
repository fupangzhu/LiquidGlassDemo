#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "LiquidGlassStyle.h"

class AGlassDemoHUD;
class SLiquidGlass;
class SConstraintCanvas;
class SGlassDemoBackdrop;

class SLiquidGlassDemo final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLiquidGlassDemo) {} SLATE_ARGUMENT(AGlassDemoHUD*, HUD) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    bool SupportsKeyboardFocus() const override { return true; }
    FLiquidGlassStyle GetStyle() const { return Style; }
    FVector2D GetCardPosition() const { return CardPosition; }
    int32 GetClickCount() const { return Clicks; }
    bool IsAnimating() const { return bAnimating; }
    FReply OnMouseMove(const FGeometry&, const FPointerEvent&) override;
    FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
private:
    TWeakObjectPtr<AGlassDemoHUD> HUD;
    TSharedPtr<SConstraintCanvas> Canvas;
    TSharedPtr<SGlassDemoBackdrop> Backdrop;
    TSharedPtr<SLiquidGlass> MainGlass, PillGlass;
    FLiquidGlassStyle Style;
    FVector2D CardPosition=FVector2D(190,315), DragOffset;
    bool bDragging=false, bAnimating=true;
    int32 Background=0, Clicks=0, LightingTab=0;
    float Volume=.62f;
    void ApplyStyle();
    void Preset(int32 Index);
    FReply BeginDrag(const FGeometry&, const FPointerEvent&);
    TSharedRef<SWidget> Parameter(const TCHAR* Name, float FLiquidGlassStyle::*Member, float Maximum, FName WidgetTag);
    TSharedRef<SWidget> MakeControls();
};
