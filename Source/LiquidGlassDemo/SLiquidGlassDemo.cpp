#include "SLiquidGlassDemo.h"
#include "GlassDemoGameMode.h"
#include "SLiquidGlass.h"
#include "LiquidGlassPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Misc/App.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
const FLinearColor Muted(.62f,.70f,.83f,1);
const FLinearColor Accent(.30f,.87f,.93f,1);
TSharedRef<STextBlock> Label(const FString& Text,int32 Size=16,FLinearColor Color=FLinearColor::White,bool Bold=false)
{
    return SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",Size)).ColorAndOpacity(Color);
}
const FButtonStyle* ButtonStyle()
{
    static FButtonStyle Style=[]
    {
        FButtonStyle S=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder");
        S.SetNormal(FSlateRoundedBoxBrush(FLinearColor(1,1,1,.075f),12.f));
        S.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1,1,1,.16f),12.f));
        S.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.3f,.8f,.9f,.28f),12.f));
        return S;
    }();
    return &Style;
}
TSharedRef<SWidget> Button(const TCHAR* Text,FName WidgetTag,TFunction<FReply()> Action)
{
    return SNew(SButton).Tag(WidgetTag).ButtonStyle(ButtonStyle()).ContentPadding(FMargin(14,10)).HAlign(HAlign_Center)
        .OnClicked_Lambda(MoveTemp(Action))[Label(Text,14)];
}
}

class SGlassDemoBackdrop final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SGlassDemoBackdrop) {} SLATE_END_ARGS()
    void Construct(const FArguments&) { SetCanTick(true); SetVisibility(EVisibility::HitTestInvisible); }
    void SetAnimating(bool B) { bAnimating=B; }
    void SetBackground(int32 I) { Mode=I; Invalidate(EInvalidateWidgetReason::Paint); }
    FVector2D ComputeDesiredSize(float) const override { return FVector2D(1440,900); }
    void Tick(const FGeometry& G,double T,float Delta) override
    {
        SLeafWidget::Tick(G,T,Delta);
        if(bAnimating){Time+=FMath::Min(float(FApp::GetDeltaTime()),.1f);Invalidate(EInvalidateWidgetReason::Paint);}
    }
    int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
    {
        const FVector2D Size=G.GetLocalSize();
        const auto* White=FCoreStyle::Get().GetBrush("WhiteBrush");
        TArray<FSlateGradientStop> Stops;
        Stops.Emplace(FVector2D(0,0),FLinearColor(.012f,.028f,.085f,1));
        Stops.Emplace(FVector2D(0,Size.Y),FLinearColor(.08f,.025f,.11f,1));
        FSlateDrawElement::MakeGradient(Out,Layer,G.ToPaintGeometry(),Stops,Orient_Vertical);
        const auto Box=[&](FVector2D P,FVector2D Z,FLinearColor C,float Radius)
        {
            FSlateRoundedBoxBrush B(FLinearColor::White,Radius);
            FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(Z,FSlateLayoutTransform(P)),&B,ESlateDrawEffect::None,C);
        };
        if(Mode==0)
        {
            // Deliberately sharp graphic shapes reveal background blur and lensing.
            Box(FVector2D(-120+FMath::Sin(Time*.30f)*75,270),FVector2D(720,420),FLinearColor(.02f,.45f,.57f,1),200);
            Box(FVector2D(375,155+FMath::Cos(Time*.38f)*85),FVector2D(500,500),FLinearColor(.23f,.105f,.54f,1),250);
            Box(FVector2D(545+FMath::Sin(Time*.45f)*90,500),FVector2D(360,300),FLinearColor(.94f,.24f,.13f,1),150);
            Box(FVector2D(90+FMath::Cos(Time*.5f)*100,400),FVector2D(470,32),FLinearColor(.25f,.96f,.89f,1),16);
            Box(FVector2D(720,285+FMath::Sin(Time*.6f)*105),FVector2D(25,300),FLinearColor(1,.70f,.28f,1),12);
        }
        else if(Mode==1)
        {
            for(int32 I=0;I<42;++I)
            {
                const float X=I*26.f+FMath::Fmod(Time*16.f,26.f);
                const FLinearColor Color=I%3==0?FLinearColor(.98f,.55f,.18f,1):FLinearColor(.04f,.64f,.73f,1);
                Box(FVector2D(X,150),FVector2D(12,635),Color,0);
            }
            for(int32 I=0;I<11;++I)Box(FVector2D(35,170+I*58),FVector2D(990,2),FLinearColor(1,1,1,.5f),0);
        }
        else
        {
            Box(FVector2D(45,175),FVector2D(970,610),FLinearColor(.91f,.87f,.73f,1),36);
            for(int32 I=0;I<8;++I)
            {
                const FVector2D P(75+FMath::Sin(Time*.3f+I)*32,195+I*72);
                FSlateDrawElement::MakeText(Out,Layer+2,G.ToPaintGeometry(FVector2D(950,65),FSlateLayoutTransform(P)),
                    I%2?TEXT("TYPE  /  REFRACTION  /  0123456789"):TEXT("LIQUID GLASS  /  ABCDEFGHIJK"),
                    FCoreStyle::GetDefaultFontStyle("Bold",32),ESlateDrawEffect::None,FLinearColor(.015f,.09f,.16f,1));
            }
        }
        // Right control area stays calm and opaque for legible parameter editing.
        FSlateDrawElement::MakeBox(Out,Layer+3,G.ToPaintGeometry(FVector2D(390,900),FSlateLayoutTransform(FVector2D(1050,0))),White,ESlateDrawEffect::None,FLinearColor(.012f,.019f,.035f,.98f));
        return Layer+3;
    }
private:
    float Time=0;
    int32 Mode=0;
    bool bAnimating=true;
};

void SLiquidGlassDemo::Construct(const FArguments& Args)
{
    HUD=Args._HUD;
    Style.BlurRadius=10;Style.Refraction=13;Style.Dispersion=1.5f;
    Style.Tint=FLinearColor(.12f,.19f,.28f,.12f);
    Style.RimIntensity=.85f;
    SetTag(TEXT("Demo.Root"));
    SAssignNew(Canvas,SConstraintCanvas);
    const auto Add=[this](float X,float Y,float W,float H,TSharedRef<SWidget> Widget)
    {Canvas->AddSlot().Alignment(FVector2D::ZeroVector).Offset(FMargin(X,Y,W,H))[Widget];};
    Add(0,0,1440,900,SAssignNew(Backdrop,SGlassDemoBackdrop));
    Add(56,40,900,24,Label(TEXT("MATERIAL STUDY   /   01"),13,Accent,true));
    Add(56,75,940,65,Label(TEXT("Liquid Glass Lab"),44,FLinearColor::White,true));
    Add(57,142,925,25,Label(TEXT("Drag the glass. Change the light. Keep the content crisp."),17,Muted));
    Add(56,195,345,85,HUD->UMGPanel->TakeWidget());
    Add(421,211,590,52,Label(TEXT("Real UMG container\nThe button above is a UMG child widget."),14,Muted));

    auto Card=SNew(SVerticalBox);
    Card->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBrush")).Padding(FMargin(2,0,2,16))
        .Tag(TEXT("Demo.Drag")).OnMouseButtonDown(this,&SLiquidGlassDemo::BeginDrag)
        [SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[Label(TEXT("SLATE  /  DRAG HERE"),13,Accent,true)]
            +SHorizontalBox::Slot().AutoWidth()[Label(TEXT("+  +"),15,Muted)]]];
    Card->AddSlot().AutoHeight()[Label(TEXT("A little more liquid."),31,FLinearColor::White,true)];
    Card->AddSlot().AutoHeight().Padding(0,9,0,20)[Label(TEXT("Live background. Sharp foreground."),17,FLinearColor(.87f,.93f,1,1))];
    Card->AddSlot().AutoHeight()[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("Test Slate button"),TEXT("Demo.SlateClick"),[this]{++Clicks;return FReply::Handled();})]
        +SHorizontalBox::Slot().AutoWidth().Padding(16,0).VAlign(VAlign_Center)
            [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",16)).ColorAndOpacity(FLinearColor::White)
                .Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("%02d clicks"),Clicks));})]];
    Card->AddSlot().AutoHeight().Padding(0,18,0,5)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",13)).ColorAndOpacity(Muted)
        .Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("CHILD SLIDER                                         %d%%"),FMath::RoundToInt(Volume*100)));})];
    Card->AddSlot().AutoHeight()[SNew(SSlider).Tag(TEXT("Demo.ChildSlider")).Value_Lambda([this]{return Volume;}).OnValueChanged_Lambda([this](float V){Volume=V;})
        .SliderBarColor(FLinearColor(1,1,1,.22f)).SliderHandleColor(Accent)];
    Canvas->AddSlot().Alignment(FVector2D::ZeroVector).Offset_Lambda([this]{return FMargin(CardPosition.X,CardPosition.Y,580,290);})
        [SAssignNew(MainGlass,SLiquidGlass).GlassStyle(Style).Padding(28)[Card]];
    auto Pill=SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Label(TEXT("Glass toolbar"),17,FLinearColor::White,true)]
        +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Click +1"),TEXT("Demo.PillClick"),[this]{++Clicks;return FReply::Handled();})];
    Add(222,705,500,76,SAssignNew(PillGlass,SLiquidGlass).GlassStyle(Style).Padding(FMargin(25,12))[Pill]);
    Add(56,817,940,44,Label(TEXT("Windows / UE 5.8.2   |   Native Slate + UMG\nAll optics run on the local GPU. This is an interactive Beta sample."),13,Muted));
    Add(1081,46,330,46,Label(TEXT("Glass settings"),26,FLinearColor::White,true));
    Add(1081,96,320,730,MakeControls());
    Add(1081,846,310,24,Label(TEXT("PRESETS ARE STARTING POINTS."),11,Muted));
    ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1440).HeightOverride(900)[Canvas.ToSharedRef()]]];
    ApplyStyle();
}

TSharedRef<SWidget> SLiquidGlassDemo::Parameter(const TCHAR* Name,float FLiquidGlassStyle::*Member,float Maximum,FName WidgetTag)
{
    const float Minimum=Member==&FLiquidGlassStyle::RimWidth?.25f:(Member==&FLiquidGlassStyle::BevelWidth?1.f:0.f);
    return SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[Label(Name,14,Muted)]
            +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",14)).ColorAndOpacity(FLinearColor::White)
                .Text_Lambda([this,Member]{return FText::FromString(FString::Printf(TEXT("%.1f"),Style.*Member));})]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,7,0,16)
            [SNew(SSlider).Tag(WidgetTag).Value_Lambda([this,Member,Minimum,Maximum]{return ((Style.*Member)-Minimum)/(Maximum-Minimum);})
                .OnValueChanged_Lambda([this,Member,Minimum,Maximum](float V){Style.*Member=FMath::Lerp(Minimum,Maximum,V);ApplyStyle();})
                .SliderBarColor(FLinearColor(.10f,.17f,.25f,1)).SliderHandleColor(Accent)];
}

TSharedRef<SWidget> SLiquidGlassDemo::MakeControls()
{
    auto Controls=SNew(SVerticalBox);
    Controls->AddSlot().AutoHeight().Padding(0,0,0,16)[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("Clear"),TEXT("Demo.Clear"),[this]{Preset(0);return FReply::Handled();})]
        +SHorizontalBox::Slot().FillWidth(1).Padding(6,0)[Button(TEXT("Frosted"),TEXT("Demo.Frosted"),[this]{Preset(1);return FReply::Handled();})]
        +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("Lens"),TEXT("Demo.Lens"),[this]{Preset(2);return FReply::Handled();})]];
    auto Optics=SNew(SVerticalBox);
    const auto Add=[this](TSharedRef<SVerticalBox> Panel,const TCHAR* N,float FLiquidGlassStyle::*M,float Max,FName WidgetTag)
        {Panel->AddSlot().AutoHeight()[Parameter(N,M,Max,WidgetTag)];};
    Add(Optics,TEXT("Blur radius"),&FLiquidGlassStyle::BlurRadius,40,TEXT("Demo.Blur"));
    Add(Optics,TEXT("Refraction"),&FLiquidGlassStyle::Refraction,32,TEXT("Demo.Refraction"));
    Add(Optics,TEXT("RGB dispersion"),&FLiquidGlassStyle::Dispersion,8,TEXT("Demo.Dispersion"));
    Add(Optics,TEXT("Corner radius"),&FLiquidGlassStyle::CornerRadius,145,TEXT("Demo.Radius"));
    Add(Optics,TEXT("Bevel width"),&FLiquidGlassStyle::BevelWidth,60,TEXT("Demo.Bevel"));
    Add(Optics,TEXT("Effect opacity"),&FLiquidGlassStyle::Opacity,1,TEXT("Demo.Opacity"));
    Optics->AddSlot().AutoHeight().Padding(0,0,0,15)[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[Label(TEXT("Tint amount"),14,Muted)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,7)[SNew(SSlider).Tag(TEXT("Demo.Tint")).Value_Lambda([this]{return Style.Tint.A;})
            .OnValueChanged_Lambda([this](float V){Style.Tint.A=V;ApplyStyle();}).SliderHandleColor(Accent)]];
    auto Lighting=SNew(SVerticalBox);
    Lighting->AddSlot().AutoHeight().Padding(0,0,0,16)[Label(TEXT("Two opposing reflections\n0 / 180 deg: top + bottom   |   90 deg: sides"),12,Accent)];
    Add(Lighting,TEXT("Light angle (deg)"),&FLiquidGlassStyle::LightAngleDegrees,180,TEXT("Demo.LightAngle"));
    Add(Lighting,TEXT("Highlight strength"),&FLiquidGlassStyle::RimIntensity,1,TEXT("Demo.Rim"));
    Add(Lighting,TEXT("Highlight width"),&FLiquidGlassStyle::RimWidth,6,TEXT("Demo.RimWidth"));
    Add(Lighting,TEXT("Side darkening"),&FLiquidGlassStyle::SideShadow,1,TEXT("Demo.SideShadow"));
    Add(Lighting,TEXT("Inner shadow"),&FLiquidGlassStyle::InnerShadow,1,TEXT("Demo.Shadow"));
    Lighting->AddSlot().AutoHeight().Padding(0,4,0,0)[Button(TEXT("Restore top + bottom"),TEXT("Demo.TopBottom"),[this]
        {Style.LightAngleDegrees=0;Style.RimIntensity=.85f;Style.RimWidth=1.5f;Style.SideShadow=.30f;ApplyStyle();return FReply::Handled();})];
    auto Pages=SNew(SWidgetSwitcher)+SWidgetSwitcher::Slot()[Optics]+SWidgetSwitcher::Slot()[Lighting];
    const auto Tab=[this,Pages](const TCHAR* Name,FName WidgetTag,int32 Index)
    {
        return SNew(SButton).Tag(WidgetTag).ButtonStyle(ButtonStyle()).ContentPadding(10).HAlign(HAlign_Center)
            .OnClicked_Lambda([this,Pages,Index]{LightingTab=Index;Pages->SetActiveWidgetIndex(Index);return FReply::Handled();})
            [SNew(STextBlock).Text(FText::FromString(Name)).Font(FCoreStyle::GetDefaultFontStyle("Bold",14))
                .ColorAndOpacity_Lambda([this,Index]{return FSlateColor(LightingTab==Index?Accent:Muted);})];
    };
    Controls->AddSlot().AutoHeight().Padding(0,0,0,20)[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1)[Tab(TEXT("Optics"),TEXT("Demo.OpticsTab"),0)]
        +SHorizontalBox::Slot().FillWidth(1).Padding(8,0,0,0)[Tab(TEXT("Lighting"),TEXT("Demo.LightingTab"),1)]];
    Controls->AddSlot().FillHeight(1)[Pages];
    Controls->AddSlot().AutoHeight().Padding(0,0,0,8)[SNew(SButton).Tag(TEXT("Demo.Pause")).ButtonStyle(ButtonStyle()).ContentPadding(10).HAlign(HAlign_Center)
        .OnClicked_Lambda([this]{bAnimating=!bAnimating;Backdrop->SetAnimating(bAnimating);return FReply::Handled();})
        [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor::White)
            .Text_Lambda([this]{return FText::FromString(bAnimating?TEXT("Pause background"):TEXT("Resume background"));})]];
    Controls->AddSlot().AutoHeight().Padding(0,0,0,8)[Button(TEXT("Switch background"),TEXT("Demo.Background"),[this]{Background=(Background+1)%3;Backdrop->SetBackground(Background);return FReply::Handled();})];
    Controls->AddSlot().AutoHeight()[Button(TEXT("Reset all"),TEXT("Demo.Reset"),[this]{Preset(0);CardPosition=FVector2D(190,315);Background=0;Backdrop->SetBackground(0);bAnimating=true;Backdrop->SetAnimating(true);return FReply::Handled();})];
    return Controls;
}
void SLiquidGlassDemo::ApplyStyle()
{
    if(MainGlass)MainGlass->SetGlassStyle(Style);
    auto Pill=Style;Pill.CornerRadius=38;
    if(PillGlass)PillGlass->SetGlassStyle(Pill);
    auto UMG=Style;UMG.CornerRadius=24;
    if(HUD.IsValid()&&HUD->UMGPanel)HUD->UMGPanel->SetGlassStyle(UMG);
}
void SLiquidGlassDemo::Preset(int32 Index)
{
    Style=FLiquidGlassStyle();Style.RimIntensity=.85f;Style.Tint=FLinearColor(.12f,.19f,.28f,.12f);
    if(Index==0){Style.BlurRadius=10;Style.Refraction=13;Style.Dispersion=1.5f;}
    if(Index==1){Style.BlurRadius=32;Style.Refraction=6;Style.Dispersion=.5f;Style.Tint.A=.24f;}
    if(Index==2){Style.BlurRadius=0;Style.Refraction=26;Style.Dispersion=3.2f;Style.BevelWidth=35;Style.Tint.A=.035f;}
    ApplyStyle();
}
FReply SLiquidGlassDemo::BeginDrag(const FGeometry&,const FPointerEvent& E)
{
    if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
    bDragging=true;DragOffset=Canvas->GetCachedGeometry().AbsoluteToLocal(E.GetScreenSpacePosition())-CardPosition;
    return FReply::Handled().CaptureMouse(SharedThis(this));
}
FReply SLiquidGlassDemo::OnMouseMove(const FGeometry&,const FPointerEvent& E)
{
    if(!bDragging||!HasMouseCapture())return FReply::Unhandled();
    const auto P=Canvas->GetCachedGeometry().AbsoluteToLocal(E.GetScreenSpacePosition())-DragOffset;
    CardPosition=FVector2D(FMath::Clamp(P.X,35.,445.),FMath::Clamp(P.Y,175.,495.));
    Invalidate(EInvalidateWidgetReason::Layout);return FReply::Handled();
}
FReply SLiquidGlassDemo::OnMouseButtonUp(const FGeometry&,const FPointerEvent& E)
{
    if(E.GetEffectingButton()==EKeys::LeftMouseButton&&bDragging){bDragging=false;return FReply::Handled().ReleaseMouseCapture();}
    return FReply::Unhandled();
}
