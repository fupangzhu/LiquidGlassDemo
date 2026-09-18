#include "SLiquidGlass.h"
#include "Rendering/DrawElements.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "PixelShaderUtils.h"
#include "RHIStaticStates.h"
#include "RenderUtils.h"
#include "HAL/IConsoleManager.h"
#include "Brushes/SlateRoundedBoxBrush.h"

static TAutoConsoleVariable<int32> CVarLiquidGlassEnabled(TEXT("LiquidGlass.Enabled"), 1,
    TEXT("0: inexpensive rounded tint fallback; 1: same-frame backdrop glass."));

class FLiquidGlassBlurPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FLiquidGlassBlurPS);
    SHADER_USE_PARAMETER_STRUCT(FLiquidGlassBlurPS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, SourceSampler)
        SHADER_PARAMETER(FVector2f, InvOutputSize)
        SHADER_PARAMETER(float, BlurCenterWeight)
        SHADER_PARAMETER(uint32, BlurTapCount)
        SHADER_PARAMETER_ARRAY(FVector4f, BlurTaps, [16])
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& P)
    { return IsFeatureLevelSupported(P.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FLiquidGlassBlurPS, "/Plugin/LiquidGlassUI/Private/LiquidGlass.usf", "BlurPS", SF_Pixel);

class FLiquidGlassPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FLiquidGlassPS);
    SHADER_USE_PARAMETER_STRUCT(FLiquidGlassPS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, SourceSampler)
        SHADER_PARAMETER(FVector4f, CaptureRect)
        SHADER_PARAMETER(FVector4f, OriginAndSize)
        SHADER_PARAMETER(FVector4f, ScreenToLocal)
        SHADER_PARAMETER(FVector4f, LocalToScreen)
        SHADER_PARAMETER(FVector4f, Shape)
        SHADER_PARAMETER(FVector4f, SurfaceTint)
        SHADER_PARAMETER(FVector4f, Effects)
        SHADER_PARAMETER(FVector4f, RimLighting)
        SHADER_PARAMETER(uint32, ClipPlaneCount)
        SHADER_PARAMETER_ARRAY(FVector4f, ClipPlanes, [32])
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& P)
    { return IsFeatureLevelSupported(P.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FLiquidGlassPS, "/Plugin/LiquidGlassUI/Private/LiquidGlass.usf", "GlassPS", SF_Pixel);

struct FLiquidGlassPaintData
{
    FVector2f Origin, Size, AxisX, AxisY, InverseX, InverseY;
    FSlateRect Bounds, Clip;
    FLiquidGlassStyle Style;
    FLinearColor Tint;
    float Opacity = 1.f;
    TArray<FVector4f> ClipPlanes;
};

class FLiquidGlassDrawer final : public ICustomSlateElement
{
public:
    // Assigned exclusively in an enqueued render command; never read game-thread state here.
    FLiquidGlassPaintData Data;

    void Draw_RenderThread(FRDGBuilder& Graph, const FDrawPassInputs& Inputs) override
    {
        if (!Inputs.OutputTexture || Data.Opacity <= 0 || Data.Size.GetMin() <= 0) return;
        RDG_EVENT_SCOPE(Graph, "LiquidGlass");
        const FIntPoint Extent = Inputs.OutputTexture->Desc.Extent;
        const FVector2f Offset = Inputs.ElementsOffset;
        const FSlateRect& B = Data.Bounds;
        const FSlateRect& C = Data.Clip;
        FIntRect DrawRect(
            FMath::Max(0, FMath::FloorToInt(FMath::Max(B.Left, C.Left) + Offset.X)),
            FMath::Max(0, FMath::FloorToInt(FMath::Max(B.Top, C.Top) + Offset.Y)),
            FMath::Min(Extent.X, FMath::CeilToInt(FMath::Min(B.Right, C.Right) + Offset.X)),
            FMath::Min(Extent.Y, FMath::CeilToInt(FMath::Min(B.Bottom, C.Bottom) + Offset.Y)));
        if (DrawRect.Width() <= 0 || DrawRect.Height() <= 0) return;

        const float Scale = FMath::Max(Data.AxisX.Size(), Data.AxisY.Size());
        const float Blur = FMath::Clamp(Data.Style.BlurRadius, 0.f, 64.f) * Scale;
        const int32 Padding = FMath::CeilToInt((FMath::Clamp(Data.Style.Refraction,0.f,64.f) + FMath::Clamp(Data.Style.Dispersion,0.f,8.f)) * Scale + Blur * 2.f + 3.f);
        const FIntRect Capture(FIntPoint(FMath::Max(0, DrawRect.Min.X - Padding), FMath::Max(0, DrawRect.Min.Y - Padding)),
                              FIntPoint(FMath::Min(Extent.X, DrawRect.Max.X + Padding), FMath::Min(Extent.Y, DrawRect.Max.Y + Padding)));
        const FIntPoint CaptureSize = Capture.Size();
        auto CopyDesc = FRDGTextureDesc::Create2D(CaptureSize, Inputs.OutputTexture->Desc.Format, FClearValueBinding::None,
            TexCreate_ShaderResource | (Inputs.OutputTexture->Desc.Flags & TexCreate_SRGB));
        FRDGTextureRef Source = Graph.CreateTexture(CopyDesc, TEXT("LiquidGlass.Backdrop"));
        FRHICopyTextureInfo Copy;
        Copy.SourcePosition = FIntVector(Capture.Min.X, Capture.Min.Y, 0);
        Copy.Size = FIntVector(CaptureSize.X, CaptureSize.Y, 1);
        AddCopyTexturePass(Graph, Inputs.OutputTexture, Source, Copy);

        const auto* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        if (Blur > .01f)
        {
            const int32 Downsample = FMath::Clamp(Data.Style.BlurDownsample, 1, 4);
            const FIntPoint Small(FMath::Max(1, FMath::DivideAndRoundUp(CaptureSize.X, Downsample)),
                                  FMath::Max(1, FMath::DivideAndRoundUp(CaptureSize.Y, Downsample)));
            const auto BlurDesc = FRDGTextureDesc::Create2D(Small, PF_FloatRGBA, FClearValueBinding::None,
                TexCreate_ShaderResource | TexCreate_RenderTargetable);
            TShaderMapRef<FLiquidGlassBlurPS> BlurShader(ShaderMap);
            for (int32 Pass = Downsample > 1 ? -1 : 0; Pass < 2; ++Pass)
            {
                auto Target = Graph.CreateTexture(BlurDesc, Pass < 0 ? TEXT("LiquidGlass.Downsample") : (Pass == 0 ? TEXT("LiquidGlass.BlurX") : TEXT("LiquidGlass.BlurY")));
                auto* P = Graph.AllocParameters<FLiquidGlassBlurPS::FParameters>();
                P->SourceTexture = Source;
                P->SourceSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI();
                P->InvOutputSize = FVector2f(1.f / Small.X, 1.f / Small.Y);
                const float TexelScale=Pass==0?float(Small.X)/CaptureSize.X:float(Small.Y)/CaptureSize.Y;
                const float Sigma=FMath::Max(.35f,Blur*TexelScale/3.f);
                const int32 Radius=FMath::Clamp(FMath::CeilToInt(3.f*Sigma),1,32);
                float Total=1.f;
                for(int32 I=1;I<=Radius;++I) Total+=2.f*FMath::Exp(-.5f*FMath::Square(I/Sigma));
                P->BlurCenterWeight=Pass<0?1.f:1.f/Total;
                P->BlurTapCount=Pass<0?0:FMath::DivideAndRoundUp(Radius,2);
                for(int32 I=0;I<16;++I)
                {
                    FVector4f Tap(0,0,0,0);
                    if(I<int32(P->BlurTapCount))
                    {
                        const int32 A=I*2+1;
                        const float WA=FMath::Exp(-.5f*FMath::Square(A/Sigma));
                        const float WB=A+1<=Radius?FMath::Exp(-.5f*FMath::Square((A+1)/Sigma)):0.f;
                        const float Distance=A+WB/(WA+WB);
                        Tap=FVector4f(Pass==0?Distance/Small.X:0.f,Pass==1?Distance/Small.Y:0.f,(WA+WB)/Total,0);
                    }
                    P->BlurTaps[I]=Tap;
                }
                P->RenderTargets[0] = FRenderTargetBinding(Target, ERenderTargetLoadAction::ENoAction);
                FPixelShaderUtils::AddFullscreenPass(Graph, ShaderMap, RDG_EVENT_NAME("LiquidGlass Blur %d", Pass),
                    BlurShader, P, FIntRect(FIntPoint::ZeroValue, Small));
                Source = Target;
            }
        }

        auto* P = Graph.AllocParameters<FLiquidGlassPS::FParameters>();
        P->SourceTexture = Source;
        P->SourceSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI();
        P->CaptureRect = FVector4f(Capture.Min.X, Capture.Min.Y, 1.f / CaptureSize.X, 1.f / CaptureSize.Y);
        P->OriginAndSize = FVector4f(Data.Origin.X + Offset.X, Data.Origin.Y + Offset.Y, Data.Size.X, Data.Size.Y);
        P->ScreenToLocal = FVector4f(Data.InverseX.X, Data.InverseY.X, Data.InverseX.Y, Data.InverseY.Y);
        P->LocalToScreen = FVector4f(Data.AxisX.X, Data.AxisX.Y, Data.AxisY.X, Data.AxisY.Y);
        P->Shape = FVector4f(FMath::Clamp(Data.Style.CornerRadius, 0.f, Data.Size.GetMin() * .5f),
            FMath::Max(1.f, Data.Style.BevelWidth), FMath::Clamp(Data.Style.Refraction, 0.f, 64.f), FMath::Clamp(Data.Style.Dispersion, 0.f, 8.f));
        // SDR Slate targets contain display-referred values. HDR color conversion is not certified.
        const FColor Tint = Data.Tint.ToFColorSRGB();
        P->SurfaceTint = FVector4f(Tint.R / 255.f, Tint.G / 255.f, Tint.B / 255.f, FMath::Clamp(Data.Tint.A, 0.f, 1.f));
        P->Effects = FVector4f(FMath::Clamp(Data.Style.RimIntensity,0.f,1.f), FMath::Clamp(Data.Style.InnerShadow,0.f,1.f), Data.Opacity, 0);
        const float LightAngle = FMath::DegreesToRadians(FMath::Clamp(Data.Style.LightAngleDegrees, 0.f, 180.f));
        P->RimLighting = FVector4f(FMath::Sin(LightAngle), -FMath::Cos(LightAngle),
            FMath::Clamp(Data.Style.RimWidth, .25f, 8.f), FMath::Clamp(Data.Style.SideShadow, 0.f, 1.f));
        P->ClipPlaneCount = Data.ClipPlanes.Num();
        for (int32 I = 0; I < 32; ++I)
        {
            FVector4f Plane = I < Data.ClipPlanes.Num() ? Data.ClipPlanes[I] : FVector4f(0, 0, 0, 0);
            Plane.Z -= Plane.X * Offset.X + Plane.Y * Offset.Y;
            P->ClipPlanes[I] = Plane;
        }
        P->RenderTargets[0] = FRenderTargetBinding(Inputs.OutputTexture, ERenderTargetLoadAction::ELoad);
        TShaderMapRef<FLiquidGlassPS> Shader(ShaderMap);
        FPixelShaderUtils::AddFullscreenPass(Graph, ShaderMap, RDG_EVENT_NAME("LiquidGlass Composite"), Shader, P, DrawRect,
            TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI());
    }
};

void SLiquidGlass::Construct(const FArguments& Args)
{
    GlassStyle = Args._GlassStyle;
    Drawer = MakeShared<FLiquidGlassDrawer, ESPMode::ThreadSafe>();
    SetVisibility(EVisibility::SelfHitTestInvisible);
    ChildSlot.Padding(Args._Padding)[Args._Content.Widget];
}
SLiquidGlass::~SLiquidGlass()
{
    auto KeepAlive = MoveTemp(Drawer);
    ENQUEUE_RENDER_COMMAND(ReleaseLiquidGlass)([KeepAlive](FRHICommandListImmediate&) {});
}
void SLiquidGlass::SetGlassStyle(const FLiquidGlassStyle& InStyle)
{
    GlassStyle = InStyle;
    Invalidate(EInvalidateWidgetReason::Paint);
}
void SLiquidGlass::SetContent(TSharedRef<SWidget> InContent) { ChildSlot[InContent]; }
void SLiquidGlass::SetContentPadding(FMargin InPadding) { ChildSlot.Padding(InPadding); }

int32 SLiquidGlass::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Clip,
    FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& WidgetStyle, bool Enabled) const
{
    FLiquidGlassPaintData D;
    D.Style = GlassStyle;
    D.Size = FVector2f(G.GetLocalSize());
    D.Bounds = G.GetRenderBoundingRect();
    D.Clip = Clip;
    D.Opacity = FMath::Clamp(GlassStyle.Opacity * WidgetStyle.GetColorAndOpacityTint().A, 0.f, 1.f);
    D.Tint = GlassStyle.Tint;
    const auto T = G.GetAccumulatedRenderTransform();
    D.Origin = TransformPoint(T, FVector2f::ZeroVector);
    D.AxisX = TransformVector(T, FVector2f(1, 0));
    D.AxisY = TransformVector(T, FVector2f(0, 1));
    const float Determinant = D.AxisX.X * D.AxisY.Y - D.AxisX.Y * D.AxisY.X;
    if (FMath::Abs(Determinant) < .000001f || D.Size.GetMin() <= 0 || D.Opacity <= 0)
        return SCompoundWidget::OnPaint(Args, G, Clip, Out, Layer, WidgetStyle, Enabled);
    const auto Inv = Inverse(T);
    D.InverseX = TransformVector(Inv, FVector2f(1, 0));
    D.InverseY = TransformVector(Inv, FVector2f(0, 1));

    const auto AddZone = [&D](const FSlateClippingZone& Zone)
    {
        const FVector2f Points[] = {Zone.TopLeft, Zone.TopRight, Zone.BottomRight, Zone.BottomLeft};
        const FVector2f Center = (Zone.TopLeft + Zone.BottomRight) * .5f;
        for (int32 I = 0; I < 4; ++I)
        {
            const FVector2f A = Points[I], Edge = Points[(I + 1) % 4] - A;
            FVector2f N(-Edge.Y, Edge.X);
            if (FVector2f::DotProduct(N, Center - A) < 0) N = -N;
            D.ClipPlanes.Add(FVector4f(N.X, N.Y, -FVector2f::DotProduct(N, A), 0));
        }
    };
    AddZone(FSlateClippingZone(Clip));
    if (const auto State = Out.GetClippingState(); State.IsSet())
    {
        if (State->ScissorRect.IsSet()) AddZone(State->ScissorRect.GetValue());
        else for (const auto& Zone : State->StencilQuads) AddZone(Zone);
    }
    // Never draw outside a deep stencil clip stack. Native tint is a safe fallback.
    if (CVarLiquidGlassEnabled.GetValueOnGameThread() == 0 || D.ClipPlanes.Num() > 32 || GMaxRHIFeatureLevel < ERHIFeatureLevel::SM5)
    {
        FSlateRoundedBoxBrush Brush(FLinearColor::White, FMath::Clamp(GlassStyle.CornerRadius, 0.f, D.Size.GetMin() * .5f));
        FLinearColor Tint = GlassStyle.Tint * WidgetStyle.GetColorAndOpacityTint();
        Tint.A = FMath::Max(GlassStyle.Tint.A, .7f) * D.Opacity;
        FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), &Brush, ESlateDrawEffect::None, Tint);
    }
    else
    {
        auto RenderDrawer = Drawer;
        ENQUEUE_RENDER_COMMAND(UpdateLiquidGlass)([RenderDrawer, D = MoveTemp(D)](FRHICommandListImmediate&) mutable
        { RenderDrawer->Data = MoveTemp(D); });
        FSlateDrawElement::MakeCustom(Out, Layer, Drawer);
    }
    return SCompoundWidget::OnPaint(Args, G, Clip, Out, Layer + 1, WidgetStyle, Enabled);
}
