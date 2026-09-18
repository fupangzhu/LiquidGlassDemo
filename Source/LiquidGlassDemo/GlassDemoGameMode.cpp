#include "GlassDemoGameMode.h"
#include "SLiquidGlassDemo.h"
#include "LiquidGlassPanel.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"

AGlassDemoGameMode::AGlassDemoGameMode()
{
    HUDClass=AGlassDemoHUD::StaticClass();
    DefaultPawnClass=nullptr;
}

void AGlassDemoHUD::BeginPlay()
{
    Super::BeginPlay();
    UMGPanel=NewObject<ULiquidGlassPanel>(this);
    UMGPanel->SetContentPadding(FMargin(20));
    auto* Row=NewObject<UHorizontalBox>(UMGPanel);
    UMGPanel->AddChild(Row);
    UMGLabel=NewObject<UTextBlock>(Row);
    UMGLabel->SetText(FText::FromString(TEXT("UMG / 0 clicks")));
    UMGLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",17));
    UMGLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    auto* LabelSlot=Row->AddChildToHorizontalBox(UMGLabel);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    auto* Button=NewObject<UButton>(Row);
    FButtonStyle ButtonStyle=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder");
    ButtonStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(1,1,1,.12f),12.f));
    ButtonStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1,1,1,.22f),12.f));
    ButtonStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(1,1,1,.32f),12.f));
    Button->SetStyle(ButtonStyle);
    auto* ButtonText=NewObject<UTextBlock>(Button);
    ButtonText->SetText(FText::FromString(TEXT("Click me")));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",15));
    Button->AddChild(ButtonText);
    Row->AddChildToHorizontalBox(Button)->SetVerticalAlignment(VAlign_Center);
    Button->OnClicked.AddDynamic(this,&AGlassDemoHUD::ClickUMG);
    Button->TakeWidget()->SetTag(TEXT("Demo.UMGButton"));

    SAssignNew(Demo,SLiquidGlassDemo).HUD(this);
    GEngine->GameViewport->AddViewportWidgetContent(Demo.ToSharedRef(),10);
    auto* PC=GetOwningPlayerController();
    PC->bShowMouseCursor=true;
    FInputModeUIOnly Input;
    Input.SetWidgetToFocus(Demo);
    Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(Input);
}
void AGlassDemoHUD::ClickUMG()
{
    ++UMGClicks;
    UMGLabel->SetText(FText::FromString(FString::Printf(TEXT("UMG / %d clicks"),UMGClicks)));
}
void AGlassDemoHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    if(Demo&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Demo.ToSharedRef());
    Demo.Reset();
    Super::EndPlay(Reason);
}
