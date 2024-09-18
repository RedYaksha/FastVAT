// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VATModelSettings.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"

#include "VATModel.generated.h"

/**
 * 
 */
UCLASS()
class FASTVAT_API UVATModel : public UObject
{
	GENERATED_BODY()
public:
	UVATModel();

	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMesh> SkeletalMesh;
	
	UPROPERTY(EditAnywhere)
	FVector2D LODRange;
	
	UPROPERTY(EditAnywhere)
	EVATModelMode Mode;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<FVATAnimSequenceInfo> AnimSequences;

	
	/**
	* StaticMesh UVChannel Index for storing vertex information.
	* Make sure this index does not conflict with the Lightmap UV Index.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StaticMesh")
	int32 UVChannel = 1;

	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UVATModelSettings> Settings;

	// generated properties
	
	UPROPERTY(EditAnywhere, Category="Generated")
	TObjectPtr<UStaticMesh> StaticMesh;

	// ------------------------------------------------------
	// Texture
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generated|Texture")
	TArray< TSoftObjectPtr<UTexture2D> > VertexPositionTextures;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generated|Texture")
	TArray< TSoftObjectPtr<UTexture2D> > VertexNormalTextures;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generated|Texture")
	TArray< TSoftObjectPtr<UTexture2D> > BoneWeightTextures;
	
	/**
	* Texture for storing bone positions
	* This is only used on Bone Mode
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generated|Texture")
	TSoftObjectPtr<UTexture2D> BonePositionTexture;

	/**
	* Texture for storing bone rotations
	* This is only used on Bone Mode
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generated|Texture")
	TSoftObjectPtr<UTexture2D> BoneRotationTexture;
	
	/**
	* Texture for storing vertex/bone weighting
	* This is only used on Bone Mode
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generated|Texture")
	TSoftObjectPtr<UTexture2D> BoneWeightTexture;

	// ------------------------------------------------------
	// Info

	/* Total Number of Frames in all animations */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	int32 NumFrames = 0;

	/* Total Number of Bones */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	int32 NumBones = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	TArray<int32> VertexRowsPerFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	FVector3f VertexMinBBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	FVector3f VertexSizeBBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	TArray<int32> BoneWeightRowsPerFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	TArray<int32> BoneRowsPerFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	FVector3f BoneMinBBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	FVector3f BoneSizeBBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated|Info")
	TArray<FVATAnimInfo> Animations;
	
public:
	UStaticMesh* GetStaticMesh() const { return StaticMesh; }
	USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }
	
	// If we weren't in a plugin, we could unify this in a base class
	template<typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer)
	{
		AssetType* ReturnVal = nullptr;
		if (AssetPointer.ToSoftObjectPath().IsValid())
		{
			ReturnVal = AssetPointer.Get();
			if (!ReturnVal)
			{
				AssetType* LoadedAsset = Cast<AssetType>(AssetPointer.ToSoftObjectPath().TryLoad());
				if (ensureMsgf(LoadedAsset, TEXT("Failed to load asset pointer %s"), *AssetPointer.ToString()))
				{
					ReturnVal = LoadedAsset;
				}
			}
		}
		return ReturnVal;
	}
	
#define VATModel_Array_Texture_ASSET_ACCESSOR(ClassName, PropertyName) \
	FORCEINLINE ClassName* Get##PropertyName(int Index) const { return GetAsset(PropertyName ## s[Index]); }
	
#define VATModel_Texture_ASSET_ACCESSOR(ClassName, PropertyName) \
	FORCEINLINE ClassName* Get##PropertyName() const { return GetAsset(PropertyName); }

	VATModel_Array_Texture_ASSET_ACCESSOR(UTexture2D, VertexPositionTexture);
	VATModel_Array_Texture_ASSET_ACCESSOR(UTexture2D, VertexNormalTexture);
	VATModel_Texture_ASSET_ACCESSOR(UTexture2D, BonePositionTexture);
	VATModel_Texture_ASSET_ACCESSOR(UTexture2D, BoneRotationTexture);
	VATModel_Array_Texture_ASSET_ACCESSOR(UTexture2D, BoneWeightTexture);

	void ResetInfo();
	
};

