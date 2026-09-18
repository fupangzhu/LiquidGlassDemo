#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GlassDemoGameMode.generated.h"

class ULiquidGlassPanel;
class UTextBlock;
class SLiquidGlassDemo;

UCLASS()
class AGlassDemoHUD : public AHUD
{
    GENERATED_BODY()
public:
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION() void ClickUMG();
    UPROPERTY(Transient) TObjectPtr<ULiquidGlassPanel> UMGPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> UMGLabel;
    TSharedPtr<SLiquidGlassDemo> Demo;
    int32 UMGClicks=0;
};

UCLASS()
class AGlassDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AGlassDemoGameMode();
};
