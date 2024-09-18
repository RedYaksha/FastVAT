// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SVATEditorViewportPreviewMenu.h"
#include "UObject/Object.h"
#include "VATEditorViewportPreviewMenuContext.generated.h"

/**
 * 
 */
UCLASS()
class FASTVATEDITOR_API UVATEditorViewportPreviewMenuContext : public UObject
{
	GENERATED_BODY()
public:
	TWeakPtr<const SVATEditorViewportPreviewMenu> VATEditorViewportPreviewMenu;
};
