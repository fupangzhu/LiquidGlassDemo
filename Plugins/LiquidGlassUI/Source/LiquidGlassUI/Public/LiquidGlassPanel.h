#pragma once
#include "CoreMinimal.h"
#include "Components/ContentWidget.h"
#include "LiquidGlassStyle.h"
#include "LiquidGlassPanel.generated.h"

class SLiquidGlass;

/** A single-child glass container. Add an Overlay/Box child for multiple controls. */
UCLASS(meta=(DisplayName="Liquid Glass Panel"))
class LIQUIDGLASSUI_API ULiquidGlassPanel : public UContentWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetGlassStyle, Category="Liquid Glass")
    FLiquidGlassStyle GlassStyle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetContentPadding, Category="Liquid Glass")
    FMargin ContentPadding = FMargin(16.f);

    UFUNCTION(BlueprintCallable, Category="Liquid Glass")
    void SetGlassStyle(const FLiquidGlassStyle& InStyle);

    UFUNCTION(BlueprintCallable, Category="Liquid Glass")
    void SetContentPadding(FMargin InPadding);

    void SynchronizeProperties() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
    const FText GetPaletteCategory() override { return NSLOCTEXT("LiquidGlassUI", "Palette", "Liquid Glass"); }
#endif
protected:
    TSharedRef<SWidget> RebuildWidget() override;
    void OnSlotAdded(UPanelSlot*) override;
    void OnSlotRemoved(UPanelSlot*) override;
private:
    TSharedPtr<SLiquidGlass> Glass;
};
