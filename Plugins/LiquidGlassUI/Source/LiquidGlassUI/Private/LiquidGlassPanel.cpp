#include "LiquidGlassPanel.h"
#include "SLiquidGlass.h"

TSharedRef<SWidget> ULiquidGlassPanel::RebuildWidget()
{
    return SAssignNew(Glass, SLiquidGlass).GlassStyle(GlassStyle).Padding(ContentPadding)
        [GetContent() ? GetContent()->TakeWidget() : SNullWidget::NullWidget];
}
void ULiquidGlassPanel::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Glass) { Glass->SetGlassStyle(GlassStyle); Glass->SetContentPadding(ContentPadding); }
}
void ULiquidGlassPanel::SetGlassStyle(const FLiquidGlassStyle& InStyle)
{
    GlassStyle = InStyle;
    if (Glass) Glass->SetGlassStyle(GlassStyle);
}
void ULiquidGlassPanel::SetContentPadding(FMargin InPadding)
{
    ContentPadding = InPadding;
    if (Glass) Glass->SetContentPadding(InPadding);
}
void ULiquidGlassPanel::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Glass.Reset();
}
void ULiquidGlassPanel::OnSlotAdded(UPanelSlot*)
{
    if (Glass && GetContent()) Glass->SetContent(GetContent()->TakeWidget());
}
void ULiquidGlassPanel::OnSlotRemoved(UPanelSlot*)
{
    if (Glass) Glass->SetContent(SNullWidget::NullWidget);
}
