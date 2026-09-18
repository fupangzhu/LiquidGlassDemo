#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GlassDemoGameMode.h"
#include "SLiquidGlassDemo.h"
#include "LiquidGlassPanel.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"

namespace
{
TSharedPtr<SWidget> Find(const TSharedRef<SWidget>& Root,FName Tag)
{
    if(!Root->GetVisibility().IsVisible())return nullptr;
    if(Root->GetTag()==Tag)return Root;
    if(auto* Children=Root->GetChildren())for(int32 I=0;I<Children->Num();++I)
        if(auto Found=Find(Children->GetChildAt(I),Tag))return Found;
    return nullptr;
}
bool Click(const TSharedRef<SWidget>& Root,FName Tag,FVector2D At=FVector2D(.5,.5))
{
    auto W=Find(Root,Tag);if(!W||!W->IsEnabled())return false;
    const auto P=W->GetCachedGeometry().GetAbsolutePositionAtCoordinates(At);
    auto& App=FSlateApplication::Get();
    auto Path=App.LocateWindowUnderMouse(P,App.GetInteractiveTopLevelWindows());
    bool Hit=false;for(int32 I=0;I<Path.Widgets.Num();++I)if(Path.Widgets[I].Widget==W)Hit=true;
    if(!Hit)return false;
    TSet<FKey> Keys;Keys.Add(EKeys::LeftMouseButton);
    FPointerEvent Down(FSlateApplication::CursorPointerIndex,P,P,Keys,EKeys::LeftMouseButton,0,FModifierKeysState());
    FPointerEvent Up(FSlateApplication::CursorPointerIndex,P,P,TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState());
    App.RoutePointerMoveEvent(Path,Up,true);App.RoutePointerDownEvent(Path,Down);
    return App.RoutePointerUpEvent(Path,Up).IsEventHandled();
}
class FDemoCheck final : public IAutomationLatentCommand
{
public:
    FDemoCheck(FAutomationTestBase* T,AGlassDemoHUD* H):Test(T),HUD(H),Ready(FPlatformTime::Seconds()+2.){}
    bool Update() override
    {
        if(FPlatformTime::Seconds()<Ready)return false;
        if(!HUD.IsValid()||!HUD->Demo){Test->AddError(TEXT("Example HUD missing"));return true;}
        auto Demo=HUD->Demo.ToSharedRef();
        if(Phase==0)
        {
            // Automated runs may start behind the user's active window. Permit Slate
            // capture during this input sequence, then restore the normal app policy.
            auto& TestApp=FSlateApplication::Get();
            const bool PreviousBackgroundInput=TestApp.GetHandleDeviceInputWhenApplicationNotActive();
            TestApp.SetHandleDeviceInputWhenApplicationNotActive(true);
            ON_SCOPE_EXIT { TestApp.SetHandleDeviceInputWhenApplicationNotActive(PreviousBackgroundInput); };
            Test->TestTrue(TEXT("Slate button receives real hit-tested pointer input"),Click(Demo,TEXT("Demo.SlateClick")));
            Test->TestEqual(TEXT("Slate counter updates"),Demo->GetClickCount(),1);
            Test->TestTrue(TEXT("UMG child button receives pointer input"),Click(Demo,TEXT("Demo.UMGButton")));
            Test->TestEqual(TEXT("UMG counter updates"),HUD->UMGClicks,1);
            Test->TestTrue(TEXT("Frosted preset clickable"),Click(Demo,TEXT("Demo.Frosted")));
            Test->TestEqual(TEXT("Frosted updates Slate"),Demo->GetStyle().BlurRadius,32.f);
            Test->TestEqual(TEXT("Frosted updates UMG"),HUD->UMGPanel->GlassStyle.BlurRadius,32.f);
            Test->TestTrue(TEXT("Lens preset clickable"),Click(Demo,TEXT("Demo.Lens")));
            Test->TestEqual(TEXT("Lens disables blur"),Demo->GetStyle().BlurRadius,0.f);
            Test->TestTrue(TEXT("Pause clickable"),Click(Demo,TEXT("Demo.Pause")));
            Test->TestFalse(TEXT("Animation pauses"),Demo->IsAnimating());
            Test->TestTrue(TEXT("Background selector clickable"),Click(Demo,TEXT("Demo.Background")));
            Test->TestTrue(TEXT("Blur slider receives pointer input"),Click(Demo,TEXT("Demo.Blur"),FVector2D(.8,.5)));
            Test->TestTrue(TEXT("Blur slider changes the optical parameter"),Demo->GetStyle().BlurRadius>28.f);
            Test->TestEqual(TEXT("Slider synchronizes UMG"),HUD->UMGPanel->GlassStyle.BlurRadius,Demo->GetStyle().BlurRadius);
            if(auto Handle=Find(Demo,TEXT("Demo.Drag")))
            {
                auto& App=FSlateApplication::Get();
                const auto P=Handle->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(.5,.5));
                const auto Q=P+FVector2D(95,20);
                const FVector2D Before=Demo->GetCardPosition();
                auto Path=App.LocateWindowUnderMouse(P,App.GetInteractiveTopLevelWindows());
                TSet<FKey> Keys;Keys.Add(EKeys::LeftMouseButton);
                FPointerEvent D(FSlateApplication::CursorPointerIndex,P,P,Keys,EKeys::LeftMouseButton,0,FModifierKeysState());
                FPointerEvent M(FSlateApplication::CursorPointerIndex,Q,P,Keys,EKeys::Invalid,0,FModifierKeysState());
                FPointerEvent U(FSlateApplication::CursorPointerIndex,Q,Q,TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState());
                App.RoutePointerMoveEvent(Path,D,true);App.RoutePointerDownEvent(Path,D);
                Test->TestTrue(TEXT("Drag handle captures pointer"),Demo->HasMouseCapture());
                App.RoutePointerMoveEvent(Path,M,false);App.RoutePointerUpEvent(Path,U);
                Test->TestTrue(TEXT("Dragging moves the glass card"),Demo->GetCardPosition().X>Before.X+50);
            }
            else Test->AddError(TEXT("Drag handle missing"));
            Test->TestTrue(TEXT("Reset clickable and within window"),Click(Demo,TEXT("Demo.Reset")));
            Test->TestEqual(TEXT("Reset restores default blur"),Demo->GetStyle().BlurRadius,10.f);
            Test->TestTrue(TEXT("Reset resumes animation"),Demo->IsAnimating());
            Test->TestTrue(TEXT("Lighting tab receives input"),Click(Demo,TEXT("Demo.LightingTab")));
            Ready=FPlatformTime::Seconds()+.3;Phase=1;return false;
        }
        if(Phase==1)
        {
            auto& App=FSlateApplication::Get();
            const bool Previous=App.GetHandleDeviceInputWhenApplicationNotActive();
            App.SetHandleDeviceInputWhenApplicationNotActive(true);
            ON_SCOPE_EXIT { App.SetHandleDeviceInputWhenApplicationNotActive(Previous); };
            Test->TestTrue(TEXT("Light angle slider receives pointer input"),Click(Demo,TEXT("Demo.LightAngle"),FVector2D(.5,.5)));
            Test->TestTrue(TEXT("Light axis rotates to horizontal"),FMath::IsNearlyEqual(Demo->GetStyle().LightAngleDegrees,90.f,2.f));
            Test->TestEqual(TEXT("UMG receives light direction"),HUD->UMGPanel->GlassStyle.LightAngleDegrees,Demo->GetStyle().LightAngleDegrees);
            Test->TestTrue(TEXT("Rim width slider receives pointer input"),Click(Demo,TEXT("Demo.RimWidth"),FVector2D(.6,.5)));
            Test->TestTrue(TEXT("Highlight width changes"),Demo->GetStyle().RimWidth>3.f);
            Test->TestTrue(TEXT("Side shadow slider receives pointer input"),Click(Demo,TEXT("Demo.SideShadow"),FVector2D(.6,.5)));
            Test->TestTrue(TEXT("Perpendicular edge darkening changes"),Demo->GetStyle().SideShadow>.5f);
            Test->TestTrue(TEXT("Restore top bottom button clickable"),Click(Demo,TEXT("Demo.TopBottom")));
            Test->TestEqual(TEXT("Default axis restored"),Demo->GetStyle().LightAngleDegrees,0.f);
            Test->TestEqual(TEXT("Default side shadow restored"),HUD->UMGPanel->GlassStyle.SideShadow,.30f);
            Ready=FPlatformTime::Seconds()+.3;Phase=2;return false;
        }
        TArray<FColor> Pixels;FIntVector Size(0,0,0);
        if(Test->TestTrue(TEXT("Rendered sample screenshot captured"),FSlateApplication::Get().TakeScreenshot(Demo,Pixels,Size)))
        {
            TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Png);
            const FString Dir=FPaths::ProjectDir()/TEXT("Validation");
            IFileManager::Get().MakeDirectory(*Dir,true);
            Test->TestTrue(TEXT("Screenshot saved"),FFileHelper::SaveArrayToFile(Png,*(Dir/TEXT("Demo.png"))));
        }
        return true;
    }
private:
    FAutomationTestBase* Test;TWeakObjectPtr<AGlassDemoHUD> HUD;double Ready;int32 Phase=0;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlassDemoTest,"LiquidGlassDemo.Interactive",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FGlassDemoTest::RunTest(const FString&)
{
    for(const auto& Context:GEngine->GetWorldContexts())
    {
        auto* W=Context.World();
        if(W&&W->IsGameWorld()&&W->GetFirstPlayerController())
            if(auto* H=Cast<AGlassDemoHUD>(W->GetFirstPlayerController()->GetHUD()))
            {FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FDemoCheck>(this,H));return true;}
    }
    AddError(TEXT("Run this test with the example game viewport open"));return false;
}
// Optional media capture: outputs actual rendered frames, separate from regression tests.
namespace {
class FGlassMediaCapture final : public IAutomationLatentCommand {
public:
    FGlassMediaCapture(FAutomationTestBase* T,AGlassDemoHUD* H):Test(T),HUD(H),Ready(FPlatformTime::Seconds()+2){}
    bool Update() override {
        if(FPlatformTime::Seconds()<Ready)return false;
        if(!HUD.IsValid()||!HUD->Demo){Test->AddError(TEXT("Missing demo"));return true;}
        auto Demo=HUD->Demo.ToSharedRef();
        auto& App=FSlateApplication::Get();
        const bool Previous=App.GetHandleDeviceInputWhenApplicationNotActive();
        App.SetHandleDeviceInputWhenApplicationNotActive(true);
        ON_SCOPE_EXIT {App.SetHandleDeviceInputWhenApplicationNotActive(Previous);};
        if(Frame==-1){
            if(Clip==0)Click(Demo,TEXT("Demo.Reset"));
            else Click(Demo,TEXT("Demo.Background"));
            Frame=0;Ready=FPlatformTime::Seconds()+.3;return false;
        }
        TArray<FColor> Pixels;FIntVector Size(0,0,0);
        if(!App.TakeScreenshot(Demo,Pixels,Size)){Test->AddError(TEXT("Capture failed"));return true;}
        TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Png);
        const FString Dir=FPaths::ProjectDir()/TEXT("Validation/Media");
        IFileManager::Get().MakeDirectory(*Dir,true);
        if(!FFileHelper::SaveArrayToFile(Png,*(Dir/FString::Printf(TEXT("clip%d_%03d.png"),Clip,Frame)))){Test->AddError(TEXT("Save failed"));return true;}
        ++Frame;Ready=FPlatformTime::Seconds()+.1;
        if(Frame>=36){++Clip;Frame=-1;}
        return Clip>=3;
    }
private: FAutomationTestBase* Test;TWeakObjectPtr<AGlassDemoHUD> HUD;double Ready;int32 Clip=0,Frame=-1;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlassMediaTest,"LiquidGlassMedia.Record",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FGlassMediaTest::RunTest(const FString&) {
    for(const auto& Context:GEngine->GetWorldContexts()) {
        auto* W=Context.World();
        if(W&&W->IsGameWorld()&&W->GetFirstPlayerController())
            if(auto* H=Cast<AGlassDemoHUD>(W->GetFirstPlayerController()->GetHUD()))
            {FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FGlassMediaCapture>(this,H));return true;}
    }
    AddError(TEXT("Open the sample game viewport"));return false;
}
#endif
