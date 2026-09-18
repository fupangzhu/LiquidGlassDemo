#pragma once
#include "CoreMinimal.h"
#include "LiquidGlassStyle.generated.h"

/** All distances are local Slate units; color and opacity have independent controls. */
USTRUCT(BlueprintType)
struct LIQUIDGLASSUI_API FLiquidGlassStyle
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shape", meta=(ClampMin="0"))
    float CornerRadius = 32.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0", ClampMax="64"))
    float BlurRadius = 12.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0", ClampMax="64"))
    float Refraction = 9.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0", ClampMax="8"))
    float Dispersion = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="1", ClampMax="128"))
    float BevelWidth = 18.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
    FLinearColor Tint = FLinearColor(.215f, .24f, .27f, .22f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance", meta=(ClampMin="0", ClampMax="1"))
    float RimIntensity = .85f;

    /** Axis of two opposing reflection lobes in screen space: 0 = top/bottom, 90 = sides. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting", meta=(ClampMin="0", ClampMax="180", Units="deg"))
    float LightAngleDegrees = 0.f;

    /** Width of the sharp reflected rim in local Slate units. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting", meta=(ClampMin="0.25", ClampMax="8"))
    float RimWidth = 1.5f;

    /** Darkening of edges perpendicular to the reflection axis. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting", meta=(ClampMin="0", ClampMax="1"))
    float SideShadow = .30f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance", meta=(ClampMin="0", ClampMax="1"))
    float InnerShadow = .12f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance", meta=(ClampMin="0", ClampMax="1"))
    float Opacity = 1.f;

    /** 1 = full resolution, 2 = half resolution, 4 = quarter resolution blur. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Performance", meta=(ClampMin="1", ClampMax="4"))
    int32 BlurDownsample = 2;
};
