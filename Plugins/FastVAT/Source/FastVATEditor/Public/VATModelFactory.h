// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VATModelFactory.generated.h"

/**
 * 
 */
UCLASS()
class FASTVATEDITOR_API UVATModelFactory : public UFactory
{
	GENERATED_BODY()
public:
	UVATModelFactory();
	UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
