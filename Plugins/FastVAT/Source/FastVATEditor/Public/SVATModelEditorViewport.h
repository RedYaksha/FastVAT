// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"
#include "Widgets/SCompoundWidget.h"


enum class EVATMeshPreviewType
{
	Invalid,
	
	StaticMesh,
	ISM,
	HISM,
	Niagara,
};

/**
 * 
 */
class FASTVATEDITOR_API SVATModelEditorViewport : public SEditorViewport, public FGCObject, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SVATModelEditorViewport)
		{}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("SVATModelEditorPreviewViewport");
	}
	// ICommonEditorViewportToolbarInfoProvider interface
	virtual TSharedRef<class SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;
	// End of ICommonEditorViewportToolbarInfoProvider interface
	
	void SetStaticMesh(TObjectPtr<UStaticMesh> InStaticMesh);
	EVATMeshPreviewType GetVATPreviewType() const { return PreviewType; }
	void SetGridSize(FVector InSize);
	void SetGridPadding(FVector InPadding);
	FVector GetGridSize() const { return GridSize; }
	FVector GetGridPadding() const { return GridPadding; }
	
protected:
	void SetVATPreviewType(EVATMeshPreviewType InType);

	bool IsPreviewModeEnabled(EVATMeshPreviewType InType) const { return PreviewType == InType; }
	
	/** SEditorViewport interface */
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual void PopulateViewportOverlays(TSharedRef<class SOverlay> Overlay) override;
	virtual EVisibility OnGetViewportContentVisibility() const override;
	virtual void BindCommands() override;
	virtual void OnFocusViewportToSelection() override;
	/** ~end SEditorViewport interface */

private:
	/** Level viewport client */
	TSharedPtr<class FEditorViewportClient> EditorViewportClient;
	
	/** Preview Scene - uses advanced preview settings */
	TSharedPtr<class FAdvancedPreviewScene> AdvancedPreviewScene;

	// preview components
	TObjectPtr<class UStaticMeshComponent> SMComponent;
	TObjectPtr<class UInstancedStaticMeshComponent> ISMComponent;
	TObjectPtr<class UHierarchicalInstancedStaticMeshComponent> HISMComponent;
	TObjectPtr<class UNiagaraComponent> NiagaraComponent;
	EVATMeshPreviewType PreviewType;

	FVector GridSize;
	FVector GridPadding;
};
