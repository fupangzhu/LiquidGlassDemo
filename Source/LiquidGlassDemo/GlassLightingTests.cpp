#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "SLiquidGlass.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
class FLightingPixels final : public IAutomationLatentCommand
{
public:
    FLightingPixels(FAutomationTestBase* InTest,TSharedRef<SWidget> InOverlay,TSharedRef<SWidget> InCanvas)
        :Test(InTest),Overlay(InOverlay),Canvas(InCanvas),Ready(FPlatformTime::Seconds()+1.){}
    bool Update() override
    {
        if(FPlatformTime::Seconds()<Ready)return false;
        TArray<FColor> Pixels;FIntVector Size(0,0,0);
        const bool Captured=FSlateApplication::Get().TakeScreenshot(Canvas,Pixels,Size)&&Size.X>0&&Size.Y>0;
        if(Test->TestTrue(TEXT("Lighting test captures GPU output"),Captured))
        {
            const auto Luma=[&](float X,float Y)
            {
                const int32 PX=FMath::Clamp(FMath::FloorToInt(X*Size.X/1280.f),0,Size.X-1);
                const int32 PY=FMath::Clamp(FMath::FloorToInt(Y*Size.Y/720.f),0,Size.Y-1);
                const FColor C=Pixels[PY*Size.X+PX];
                return (C.R+C.G+C.B)/3.f;
            };
            // Long edge strips average subpixel coverage; they don't depend on one lucky pixel.
            const auto Edge=[&](float X,float Y,float W,float H,int32 Side,int32 Depth=4)
            {
                double Sum=0;int32 Count=0;
                for(int32 I=0;I<30;++I)for(int32 D=1;D<=Depth;++D)
                {
                    const float T=.30f+I/29.f*.40f;
                    const float PX=Side<2?X+W*T:(Side==2?X+D:X+W-D);
                    const float PY=Side<2?(Side==0?Y+D:Y+H-D):Y+H*T;
                    Sum+=Luma(PX,PY);++Count;
                }
                return float(Sum/Count);
            };
            const float Base=Luma(640,390);
            const auto Verify=[&](float X,float Y,float W,float H,bool Vertical)
            {
                const float Top=Edge(X,Y,W,H,0),Bottom=Edge(X,Y,W,H,1);
                const float Left=Edge(X,Y,W,H,2),Right=Edge(X,Y,W,H,3);
                Test->AddInfo(FString::Printf(TEXT("Axis %s: background %.1f, top %.1f, bottom %.1f, left %.1f, right %.1f"),
                    Vertical?TEXT("vertical"):TEXT("horizontal"),Base,Top,Bottom,Left,Right));
                Test->TestTrue(TEXT("Both lit edges are clearly brighter than the backdrop"),
                    Vertical?(Top>Base+35&&Bottom>Base+35):(Left>Base+35&&Right>Base+35));
                Test->TestTrue(TEXT("Both perpendicular edges are darker than the backdrop"),
                    Vertical?(Luma(X+1,Y+H*.5f)<Base-8&&Luma(X+W-1,Y+H*.5f)<Base-8):(Luma(X+W*.5f,Y+1)<Base-8&&Luma(X+W*.5f,Y+H-1)<Base-8));
                Test->TestTrue(TEXT("Hard contour has no inward shadow at four pixels"),
                    Vertical?(FMath::Abs(Luma(X+4,Y+H*.5f)-Base)<4&&FMath::Abs(Luma(X+W-4,Y+H*.5f)-Base)<4):
                    (FMath::Abs(Luma(X+W*.5f,Y+4)-Base)<4&&FMath::Abs(Luma(X+W*.5f,Y+H-4)-Base)<4));
                Test->TestTrue(TEXT("Opposing edge brightness is balanced"),FMath::Abs(Top-Bottom)<12&&FMath::Abs(Left-Right)<12);
                Test->TestTrue(TEXT("Lighting leaves the central glass clear"),FMath::Abs(Luma(X+W*.32f,Y+H*.5f)-Base)<4);
                Test->TestTrue(TEXT("Rounded corner exterior is preserved"),FMath::Abs(Luma(X+1,Y+1)-Base)<4);
                Test->TestTrue(TEXT("White child remains crisp above the lighting"),Luma(X+W*.5f,Y+H*.5f)>245);
            };
            Verify(90,160,440,200,true);
            Verify(730,160,440,200,false);
            Verify(90,440,440,150,true); // A capsule must preserve the same light/dark relationship.
            for(int32 Side=0;Side<4;++Side)
                Test->TestTrue(TEXT("Zero intensity and zero side shadow restore the original backdrop"),
                    FMath::Abs(Edge(730,440,440,150,Side)-Base)<4);
            // Sample the top-right circular bevel: brightness fades toward the dark side.
            float Previous=255;
            for(int32 I=0;I<=4;++I)
            {
                const float Angle=FMath::DegreesToRadians(I*22.5f);
                const float Brightness=Luma(90+440-36+FMath::Sin(Angle)*33,160+36-FMath::Cos(Angle)*33);
                Test->TestTrue(TEXT("Rounded bevel transitions continuously toward the side"),Brightness<=Previous+12);
                Previous=Brightness;
            }
            TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Png);
            const FString Dir=FPaths::ProjectDir()/TEXT("Validation");
            IFileManager::Get().MakeDirectory(*Dir,true);
            Test->TestTrue(TEXT("Lighting evidence saved"),FFileHelper::SaveArrayToFile(Png,*(Dir/TEXT("Lighting.png"))));
        }
        if(GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Overlay);
        return true;
    }
private:
    FAutomationTestBase* Test;TSharedRef<SWidget> Overlay,Canvas;double Ready;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlassLightingTest,"LiquidGlassDemo.Lighting",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FGlassLightingTest::RunTest(const FString&)
{
    if(!TestNotNull(TEXT("Game viewport"),GEngine?GEngine->GameViewport.Get():nullptr))return false;
    auto Canvas=SNew(SConstraintCanvas);
    const auto* White=FCoreStyle::Get().GetBrush("WhiteBrush");
    Canvas->AddSlot().Alignment(FVector2D::ZeroVector).Offset(FMargin(0,0,1280,720))
        [SNew(SImage).Image(White).ColorAndOpacity(FLinearColor(.12f,.12f,.12f,1))];
    const TCHAR* Labels[]={TEXT("0 DEG / TOP + BOTTOM"),TEXT("90 DEG / LEFT + RIGHT"),TEXT("CAPSULE / TOP + BOTTOM"),TEXT("LIGHTING OFF / CONTROL")};
    for(int32 I=0;I<4;++I)
    {
        const float X=I%2?730.f:90.f,Y=I<2?160.f:440.f,H=I<2?200.f:150.f;
        FLiquidGlassStyle Style;
        Style.BlurRadius=0;Style.Refraction=0;Style.Dispersion=0;Style.InnerShadow=0;
        Style.Tint=FLinearColor::Transparent;Style.CornerRadius=I==2?75.f:36.f;
        Style.LightAngleDegrees=I==1?90.f:0.f;
        if(I==3){Style.RimIntensity=0;Style.SideShadow=0;}
        auto Child=SNew(SConstraintCanvas);
        Child->AddSlot().Alignment(FVector2D::ZeroVector).Offset(FMargin(212,H*.5f-8,16,16))[SNew(SImage).Image(White)];
        Canvas->AddSlot().Alignment(FVector2D::ZeroVector).Offset(FMargin(X,Y,440,H))[SNew(SLiquidGlass).GlassStyle(Style)[Child]];
        Canvas->AddSlot().Alignment(FVector2D::ZeroVector).Offset(FMargin(X,Y-42,440,28))
            [SNew(STextBlock).Text(FText::FromString(Labels[I])).Font(FCoreStyle::GetDefaultFontStyle("Bold",15))];
    }
    TSharedRef<SWidget> Overlay=SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
        [SNew(SBox).WidthOverride(1280).HeightOverride(720)[Canvas]];
    GEngine->GameViewport->AddViewportWidgetContent(Overlay,10000);
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FLightingPixels>(this,Overlay,Canvas));
    return true;
}
#endif
