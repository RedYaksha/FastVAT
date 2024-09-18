// Fill out your copyright notice in the Description page of Project Settings.


#include "VATModelFactory.h"

#include "VATModel.h"

UVATModelFactory::UVATModelFactory()
{
	SupportedClass = UVATModel::StaticClass();
	bCreateNew = true;
}

UObject* UVATModelFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UVATModel>(InParent, InClass, InName, Flags, Context);
}
