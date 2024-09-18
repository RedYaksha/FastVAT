// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewportToolBarMenu.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class FASTVATEDITOR_API SVATEditorViewportPreviewMenu : public SEditorViewportToolbarMenu
{
public:
	SLATE_BEGIN_ARGS(SVATEditorViewportPreviewMenu)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs, TSharedRef<class SVATModelEditorViewport> InViewport, TSharedRef<class SViewportToolBar> InParentToolBar );

private:
	FText GetPreviewMenuLabel() const;
	const FSlateBrush* GetPreviewMenuLabelIcon() const;
	void FillViewMenu(UToolMenu* Menu) const;
	TSharedRef<SWidget> GenerateGridSizeNumericInput() const;
	TSharedRef<SWidget> GenerateGridPaddingNumericInput() const;
	
protected:
	virtual TSharedRef<SWidget> GenerateViewMenuContent() const;
	virtual void RegisterMenus() const;
	void OnGridSizeChanged(FVector NewSize) const;
	void SetGridSize(int Value, EAxis::Type InAxis) const;
	void SetGridPadding(double Value, EAxis::Type InAxis) const;
	FVector GetGridSize() const;
	FVector GetGridPadding() const;
	
protected:
	/** Parent tool bar for querying other open menus */
	TWeakPtr<class SViewportToolBar> ParentToolBar;

	/** Name of tool menu */
	FName MenuName;
	
	TWeakPtr<class SVATModelEditorViewport> Viewport;
	TSharedPtr<class FExtender> MenuExtenders;

	static const FName BaseMenuName;
};


