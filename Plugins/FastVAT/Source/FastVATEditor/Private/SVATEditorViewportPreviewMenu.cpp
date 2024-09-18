// Fill out your copyright notice in the Description page of Project Settings.


#include "SVATEditorViewportPreviewMenu.h"

#include "SEditorViewportToolBarMenu.h"
#include "SVATModelEditorViewport.h"
#include "SlateOptMacros.h"
#include "VATEditorViewportPreviewMenuContext.h"
#include "VATModelEditorViewportCommands.h"
#include "Widgets/Input/SVectorInputBox.h"

const FName SVATEditorViewportPreviewMenu::BaseMenuName("FastVAT.ViewportToolbar.Preview");

#define LOCTEXT_NAMESPACE "VATEditorViewportPreviewMenu"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SVATEditorViewportPreviewMenu::Construct(const FArguments& InArgs, TSharedRef<SVATModelEditorViewport> InViewport,
                                              TSharedRef<SViewportToolBar> InParentToolBar)
{
	Viewport = InViewport;
	MenuName = BaseMenuName;
	
	SEditorViewportToolbarMenu::Construct
	(
		SEditorViewportToolbarMenu::FArguments()
			.ParentToolBar( InParentToolBar )
			.Cursor( EMouseCursor::Default )
			.Label(this, &SVATEditorViewportPreviewMenu::GetPreviewMenuLabel)
			.LabelIcon(this, &SVATEditorViewportPreviewMenu::GetPreviewMenuLabelIcon)
			.OnGetMenuContent( this, &SVATEditorViewportPreviewMenu::GenerateViewMenuContent )
	);
	
}

FText SVATEditorViewportPreviewMenu::GetPreviewMenuLabel() const
{
	check(Viewport.IsValid())
	
	EVATMeshPreviewType PreviewType = Viewport.Pin()->GetVATPreviewType();
	switch (PreviewType) {
	case EVATMeshPreviewType::StaticMesh:
		return LOCTEXT("PreviewMenuLabelStaticMesh", "Static Mesh");
		break;
	case EVATMeshPreviewType::Niagara:
		return LOCTEXT("PreviewMenuLabelNiagara", "Niagara");
		break;
	case EVATMeshPreviewType::ISM:
		return LOCTEXT("PreviewMenuLabelISM", "ISM");
		break;
	case EVATMeshPreviewType::HISM:
		return LOCTEXT("PreviewMenuLabelHISM", "HISM");
		break;
	default:
		break;
	}
	return LOCTEXT("PreviewMenuLabelError", "Error");
}

const FSlateBrush* SVATEditorViewportPreviewMenu::GetPreviewMenuLabelIcon() const
{
	return nullptr;
}

void SVATEditorViewportPreviewMenu::FillViewMenu(UToolMenu* Menu) const
{
	const FVATModelEditorViewportCommands& BaseViewportActions = FVATModelEditorViewportCommands::Get();

	{
		// View modes
		{
			FToolMenuSection& Section = Menu->AddSection("PreviewMode", LOCTEXT("PreviewModeHeader", "Preview Mode"));
			{
				Section.AddMenuEntry(BaseViewportActions.SingleStaticMesh);
				Section.AddMenuEntry(BaseViewportActions.NiagaraGrid);
				// Section.AddMenuEntry(BaseViewportActions.ISMGrid);
				//Section.AddMenuEntry(BaseViewportActions.HISMGrid);
			}
		}
		{
			FToolMenuSection& Section = Menu->AddSection("PreviewSettings", LOCTEXT("PreviewSettingsHeader", "Preview Settings"));
			{
				// grid row/col
				Section.AddEntry(FToolMenuEntry::InitWidget("GridSize", GenerateGridSizeNumericInput(), LOCTEXT("GridSize", "Grid Size")));
				Section.AddEntry(FToolMenuEntry::InitWidget("GridPadding", GenerateGridPaddingNumericInput(), LOCTEXT("GridPadding", "Grid Padding")));
			}
		}
	}
	
}

TSharedRef<SWidget> SVATEditorViewportPreviewMenu::GenerateGridSizeNumericInput() const
{
	using SNumericIntVector3DInputBox = SNumericVectorInputBox<int, FVector, 3>;
	
	return
		SNew( SBox )
		.HAlign( HAlign_Right )
		[
			SNew( SBox )
			.Padding( FMargin(4.0f, 0.0f, 0.0f, 0.0f) )
			.WidthOverride( 100.0f )
			[
				SNew ( SBorder )
				.BorderImage(FAppStyle::Get().GetBrush("Menu.WidgetBorder"))
				.Padding(FMargin(1.0f))
				[
					SNew(SNumericIntVector3DInputBox)
					.MinVector(FVector(0))
					.AllowSpin(true)
					.OnXChanged(this, &SVATEditorViewportPreviewMenu::SetGridSize, EAxis::X)
					.OnYChanged(this, &SVATEditorViewportPreviewMenu::SetGridSize, EAxis::Y)
					.OnZChanged(this, &SVATEditorViewportPreviewMenu::SetGridSize, EAxis::Z)
					.X_Lambda([this](){ return GetGridSize().X; })
					.Y_Lambda([this](){ return GetGridSize().Y; })
					.Z_Lambda([this](){ return GetGridSize().Z; })
				]
			]
		];
}

TSharedRef<SWidget> SVATEditorViewportPreviewMenu::GenerateGridPaddingNumericInput() const
{
	return
		SNew( SBox )
		.HAlign( HAlign_Right )
		[
			SNew( SBox )
			.Padding( FMargin(4.0f, 0.0f, 0.0f, 0.0f) )
			.WidthOverride( 100.0f )
			[
				SNew ( SBorder )
				.BorderImage(FAppStyle::Get().GetBrush("Menu.WidgetBorder"))
				.Padding(FMargin(1.0f))
				[
					SNew(SNumericVectorInputBox<FVector::FReal>)
					.MinVector(FVector(0))
					.AllowSpin(true)
					.OnXChanged(this, &SVATEditorViewportPreviewMenu::SetGridPadding, EAxis::X)
					.OnYChanged(this, &SVATEditorViewportPreviewMenu::SetGridPadding, EAxis::Y)
					.OnZChanged(this, &SVATEditorViewportPreviewMenu::SetGridPadding, EAxis::Z)
					.X_Lambda([this](){ return GetGridPadding().X; })
					.Y_Lambda([this](){ return GetGridPadding().Y; })
					.Z_Lambda([this](){ return GetGridPadding().Z; })
				]
			]
		];
}

TSharedRef<SWidget> SVATEditorViewportPreviewMenu::GenerateViewMenuContent() const
{
	RegisterMenus();

	UVATEditorViewportPreviewMenuContext* ContextObject = NewObject<UVATEditorViewportPreviewMenuContext>();
	ContextObject->VATEditorViewportPreviewMenu = SharedThis(this);

	FToolMenuContext MenuContext(Viewport.Pin()->GetCommandList(), MenuExtenders, ContextObject);
	return UToolMenus::Get()->GenerateWidget(MenuName, MenuContext);
}

void SVATEditorViewportPreviewMenu::RegisterMenus() const
{
	if (!UToolMenus::Get()->IsMenuRegistered(BaseMenuName))
	{
		UToolMenu* Menu = UToolMenus::Get()->RegisterMenu(BaseMenuName);
		Menu->AddDynamicSection("BaseSection", FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
		{
			if (UVATEditorViewportPreviewMenuContext* Context = InMenu->FindContext<UVATEditorViewportPreviewMenuContext>())
			{
				Context->VATEditorViewportPreviewMenu.Pin()->FillViewMenu(InMenu);
			}
		}));
	}
}

void SVATEditorViewportPreviewMenu::OnGridSizeChanged(FVector NewSize) const
{
	check(Viewport.IsValid());
	Viewport.Pin()->SetGridSize(NewSize);
}

void SVATEditorViewportPreviewMenu::SetGridSize(int Value, EAxis::Type InAxis) const
{
	check(Viewport.IsValid());
	FVector NewSize = Viewport.Pin()->GetGridSize();
	if(InAxis == EAxis::X)
	{
		NewSize.X = Value;
	}
	else if(InAxis == EAxis::Y)
	{
		NewSize.Y = Value;
	}
	else if(InAxis == EAxis::Z)
	{
		NewSize.Z = Value;
	}

	Viewport.Pin()->SetGridSize(NewSize);
}

void SVATEditorViewportPreviewMenu::SetGridPadding(double Value, EAxis::Type InAxis) const
{
	check(Viewport.IsValid());
	FVector NewPadding = Viewport.Pin()->GetGridPadding();
	if(InAxis == EAxis::X)
	{
		NewPadding.X = Value;
	}
	else if(InAxis == EAxis::Y)
	{
		NewPadding.Y = Value;
	}
	else if(InAxis == EAxis::Z)
	{
		NewPadding.Z = Value;
	}

	Viewport.Pin()->SetGridPadding(NewPadding);
}

FVector SVATEditorViewportPreviewMenu::GetGridSize() const
{
	check(Viewport.IsValid());
	return Viewport.Pin()->GetGridSize();
}

FVector SVATEditorViewportPreviewMenu::GetGridPadding() const
{
	check(Viewport.IsValid());
	return Viewport.Pin()->GetGridPadding();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE