// Fill out your copyright notice in the Description page of Project Settings.


#include "SVATModelViewportToolBar.h"
#include "SVATModelEditorViewport.h"

#include "SlateOptMacros.h"
#include "SVATEditorViewportPreviewMenu.h"
#include "VATModelEditorViewportCommands.h"

#define LOCTEXT_NAMESPACE "SVATModelViewportToolBar"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SVATModelViewportToolBar::Construct(const FArguments& InArgs, TSharedPtr<SVATModelEditorViewport> InViewport)
{
	Viewport = InViewport;
	
	SCommonEditorViewportToolbarBase::Construct(
		SCommonEditorViewportToolbarBase::FArguments()
		// .AddRealtimeButton(nullptr)
	, InViewport);
	
	UICommandList = MakeShareable(new FUICommandList);
	BindCommands();
}

void SVATModelViewportToolBar::BindCommands()
{
	const FVATModelEditorViewportCommands& Commands = FVATModelEditorViewportCommands::Get();

	// UICommandList->MapAction(Commands.SingleStaticMesh, )
}

TSharedRef<SWidget> SVATModelViewportToolBar::GenerateShowMenu() const
{
	return SNullWidget::NullWidget;
	/*
	GetInfoProvider().OnFloatingButtonClicked();
	
	TSharedRef<SEditorViewport> ViewportRef = GetInfoProvider().GetViewportWidget();
	
	FMenuBuilder ShowMenuBuilder(true, ViewportRef->GetCommandList());
	
	return ShowMenuBuilder.MakeWidget();
	*/
}

void SVATModelViewportToolBar::ExtendOptionsMenu(FMenuBuilder& OptionsMenuBuilder) const
{
	OptionsMenuBuilder.BeginSection("Preview VAT", LOCTEXT("PreviewTypesHeader", "Preview Types") );
	{
		OptionsMenuBuilder.AddMenuEntry( FVATModelEditorViewportCommands::Get().SingleStaticMesh, NAME_None, LOCTEXT("SingleStaticMeshDisplayName", "Single") );
		OptionsMenuBuilder.AddMenuEntry( FVATModelEditorViewportCommands::Get().NiagaraGrid, NAME_None, LOCTEXT("NiagaraGridDisplayName", "Niagara") );
	}
}

void SVATModelViewportToolBar::ExtendLeftAlignedToolbarSlots(TSharedPtr<SHorizontalBox> MainBoxPtr,
	TSharedPtr<SViewportToolBar> ParentToolBarPtr) const
{
	SCommonEditorViewportToolbarBase::ExtendLeftAlignedToolbarSlots(MainBoxPtr, ParentToolBarPtr);

	TSharedRef<SVATModelEditorViewport> V = Viewport.Pin().ToSharedRef();
	
	MainBoxPtr->AddSlot()
	.AutoWidth()
	[
		SNew(SVATEditorViewportPreviewMenu, V, ParentToolBarPtr.ToSharedRef())
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE