// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class FASTVATEDITOR_API SVATModelViewportToolBar : public SCommonEditorViewportToolbarBase
{
public:
	SLATE_BEGIN_ARGS(SVATModelViewportToolBar)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, TSharedPtr<class SVATModelEditorViewport> InViewport);

	void BindCommands();
	
	// SCommonEditorViewportToolbarBase interface
	virtual TSharedRef<SWidget> GenerateShowMenu() const override;
	virtual void ExtendOptionsMenu(FMenuBuilder& OptionsMenuBuilder) const override;
	/** Used for a custom realtime button as the default realtime button is for viewport realtime, not simulation realtime and overrides only allow for direct setting */
	virtual void ExtendLeftAlignedToolbarSlots(TSharedPtr<SHorizontalBox> MainBoxPtr, TSharedPtr<SViewportToolBar> ParentToolBarPtr) const override;
	virtual bool GetShowScalabilityMenu() const override
	{
		return true;
	}
	// End of SCommonEditorViewportToolbarBase
protected:
	TWeakPtr<SVATModelEditorViewport> Viewport;
	TSharedPtr<FUICommandList> UICommandList;
};
