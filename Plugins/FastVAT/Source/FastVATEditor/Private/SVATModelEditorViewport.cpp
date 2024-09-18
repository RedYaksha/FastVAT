// Fill out your copyright notice in the Description page of Project Settings.


#include "SVATModelEditorViewport.h"

#include "EngineUtils.h"
#include "SlateOptMacros.h"
#include "SVATModelViewportToolBar.h"
#include "VATModelEditorViewportCommands.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Editor/AdvancedPreviewScene/Public/AdvancedPreviewScene.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SVATModelEditorViewport::Construct(const FArguments& InArgs)
{
	SetCanTick(true);
	AdvancedPreviewScene = MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues()));

	// SMComponent = NewObject<UStaticMeshComponent>(nullptr);
	// TODO: spawning the actual object must live in the Toolkit... as everyone else does this
	SMComponent = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	NiagaraComponent = NewObject<UNiagaraComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
	SMComponent->SetStaticMesh(Mesh);

	UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/FastVAT/NS_SimpleGridSpawn"));
	check(NiagaraSystem);

	AdvancedPreviewScene->AddComponent(SMComponent, SMComponent->GetRelativeTransform());
	AdvancedPreviewScene->AddComponent(NiagaraComponent, NiagaraComponent->GetRelativeTransform());

	AdvancedPreviewScene->SetFloorVisibility(false);
	
	
	NiagaraComponent->CastShadow = 1;
	NiagaraComponent->bCastDynamicShadow = 1;
	NiagaraComponent->SetAllowScalability(false);
	NiagaraComponent->SetAsset(NiagaraSystem);
	NiagaraComponent->SetForceSolo(true);
	NiagaraComponent->SetAgeUpdateMode(ENiagaraAgeUpdateMode::TickDeltaTime);
	NiagaraComponent->SetCanRenderWhileSeeking(false);
	NiagaraComponent->Activate(true);
	
	NiagaraComponent->SetGpuComputeDebug(true);
	
	SEditorViewport::Construct( SEditorViewport::FArguments() );

	// default to real time
	Client->SetRealtime(true);

	SetVATPreviewType(EVATMeshPreviewType::Niagara);
	SetGridPadding(FVector(200, 200, 200));
	SetGridSize(FVector(2, 2, 1));
}

void SVATModelEditorViewport::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
	const float InDeltaTime)
{
	SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if(AdvancedPreviewScene)
	{
		AdvancedPreviewScene->GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
	}
}

void SVATModelEditorViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(SMComponent);
	Collector.AddReferencedObject(NiagaraComponent);
}

TSharedRef<SEditorViewport> SVATModelEditorViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SVATModelEditorViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void SVATModelEditorViewport::OnFloatingButtonClicked()
{
	
}

TSharedRef<FEditorViewportClient> SVATModelEditorViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable( new FEditorViewportClient(nullptr, AdvancedPreviewScene.Get(), SharedThis(this)) );
	
	EditorViewportClient->EngineShowFlags.Game = 1;
	
	return EditorViewportClient.ToSharedRef();
}

void SVATModelEditorViewport::PopulateViewportOverlays(TSharedRef<SOverlay> Overlay)
{
	Overlay->AddSlot()
	.VAlign(VAlign_Top)
	[
		SNew(SVATModelViewportToolBar, SharedThis(this))
	];
}

EVisibility SVATModelEditorViewport::OnGetViewportContentVisibility() const
{
	return SEditorViewport::OnGetViewportContentVisibility();
}

void SVATModelEditorViewport::BindCommands()
{
	SEditorViewport::BindCommands();

	const FVATModelEditorViewportCommands& Commands = FVATModelEditorViewportCommands::Get();
	
	CommandList->MapAction(Commands.SingleStaticMesh,
		FExecuteAction::CreateSP(
			this,
			&SVATModelEditorViewport::SetVATPreviewType,
			EVATMeshPreviewType::StaticMesh),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SVATModelEditorViewport::IsPreviewModeEnabled, EVATMeshPreviewType::StaticMesh));
	
	CommandList->MapAction(Commands.NiagaraGrid,
		FExecuteAction::CreateSP(this, &SVATModelEditorViewport::SetVATPreviewType, EVATMeshPreviewType::Niagara),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SVATModelEditorViewport::IsPreviewModeEnabled, EVATMeshPreviewType::Niagara));

}

void SVATModelEditorViewport::OnFocusViewportToSelection()
{
	SEditorViewport::OnFocusViewportToSelection();
}

void SVATModelEditorViewport::SetStaticMesh(TObjectPtr<UStaticMesh> InStaticMesh)
{
	// TODO: depends on current preview mode
	if(SMComponent)
	{
		SMComponent->SetStaticMesh(InStaticMesh);
	}
	
	if(NiagaraComponent)
	{
		NiagaraComponent->SetVariableStaticMesh("Mesh", InStaticMesh);
		NiagaraComponent->ReinitializeSystem();
	}
}

void SVATModelEditorViewport::SetGridSize(FVector InSize)
{
	GridSize = InSize;
	
	if(NiagaraComponent)
	{
		NiagaraComponent->SetVectorParameter(TEXT("GridSize"), InSize);
		NiagaraComponent->ReinitializeSystem();
	}
}

void SVATModelEditorViewport::SetGridPadding(FVector InPadding)
{
	GridPadding = InPadding;

	if(NiagaraComponent)
	{
		NiagaraComponent->SetVectorParameter(TEXT("GridPadding"), InPadding);
		NiagaraComponent->ReinitializeSystem();
	}
}

void SVATModelEditorViewport::SetVATPreviewType(EVATMeshPreviewType InType)
{
	PreviewType = InType;
	
	if(InType == EVATMeshPreviewType::Niagara)
	{
		SMComponent->SetVisibility(false);
		NiagaraComponent->SetVisibility(true);
		NiagaraComponent->ReinitializeSystem();
	}
	else if(InType == EVATMeshPreviewType::StaticMesh)
	{
		SMComponent->SetVisibility(true);
		NiagaraComponent->SetVisibility(false);
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
