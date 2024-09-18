// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"

#include "VATModelSettings.generated.h"

namespace VATParamNames
{
	static const FName Frame = TEXT("Frame");
	static const FName AutoPlay = TEXT("AutoPlay");
	static const FName StartFrame = TEXT("StartFrame");
	static const FName EndFrame = TEXT("EndFrame");
	static const FName SampleRate = TEXT("SampleRate");
	static const FName NumFrames = TEXT("NumFrames");
	static const FName MinBBox = TEXT("MinBBox");
	static const FName SizeBBox = TEXT("SizeBBox");
	static const FName NumBones = TEXT("NumBones");
	static const FName RowsPerFrame = TEXT("RowsPerFrame");
	static const FName BoneWeightRowsPerFrame = TEXT("BoneWeightsRowsPerFrame");
	static const FName VertexPositionTexture = TEXT("PositionTexture");
	static const FName VertexNormalTexture = TEXT("NormalTexture");
	static const FName BonePositionTexture = TEXT("BonePositionTexture");
	static const FName BoneRotationTexture = TEXT("BoneRotationTexture");
	static const FName BoneWeightsTexture = TEXT("BoneWeightsTexture");
	static const FName UseUV0 = TEXT("UseUV0");
	static const FName UseUV1 = TEXT("UseUV1");
	static const FName UseUV2 = TEXT("UseUV2");
	static const FName UseUV3 = TEXT("UseUV3");
	static const FName UseTwoInfluences = TEXT("UseTwoInfluences");
	static const FName UseFourInfluences = TEXT("UseFourInfluences");
}

UENUM()
enum class EVATModelMode : uint8
{
	Vertex,
	Bone,
};

UENUM(Blueprintable)
enum class EVATPrecision : uint8
{
	/* 8 bits */
	EightBits,
	/* 16 bits */
	SixteenBits,
};

UENUM(Blueprintable)
enum class EVATNumBoneInfluences : uint8
{
	/* Single bone influence */
	One,
	/* Blend between two influences */
	Two,
	/* Blend between four influences */
	Four,
};

USTRUCT(Blueprintable)
struct FVATAnimInfo
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category = Default, BlueprintReadOnly)
	int32 StartFrame = 0;

	UPROPERTY(VisibleAnywhere, Category = Default, BlueprintReadOnly)
	int32 EndFrame = 0;

};

USTRUCT(Blueprintable)
struct FVATAnimSequenceInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Default, BlueprintReadWrite)
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category = Default, BlueprintReadWrite)
	TObjectPtr<UAnimSequence> AnimSequence = nullptr;
	
	/* Use Custom FrameRange */
	UPROPERTY(EditAnywhere, Category = Default, BlueprintReadWrite)
	bool bUseCustomRange = false;

	/* Animation Start Frame */
	UPROPERTY(EditAnywhere, Category = Default, BlueprintReadWrite)
	int32 StartFrame = 0;

	/* Animation End Frame (Inclusive) */
	UPROPERTY(EditAnywhere, Category = Default, BlueprintReadWrite)
	int32 EndFrame = 0;

};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class FASTVAT_API UVATModelSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UVATModelSettings():
	Mode(EVATModelMode::Bone)
	{}

	UPROPERTY(EditAnywhere)
	EVATModelMode Mode;
	
	/**
	* Adds transformation to baked textures. 
	* This can be used for adding an offset to the animation.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RootTransform;

	/** 
	* Bone used for Rigid Binding. The bone needs to be part of the RawBones. 
	* Sockets and VirtualBones are not supported.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FName AttachToSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float SampleRate = 30.0f;
	
	/**
	* Number of Driver Triangles
	* Each StaticMesh Vertex will be influenced by N SkeletalMesh (Driver) Triangles.
	* Increasing the Number of Driver Triangles will increase the Mapping computation.
	* Using a single Driver Triangle will do a Rigid Binding to Closest Triangle.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StaticMesh|Mapping")
	int32 NumDriverTriangles = 10;

	/**
	* Inverse Distance Weighting
	* This exponent value will be used for computing weights for the DriverTriangles.
	* Larger number will create a more contrasted weighting, but it might 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StaticMesh|Mapping")
	float Sigma = 1.f;
	
	/**
	* Max resolution of the texture.
	* A smaller size will be used if the data fits.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture")
	int32 MaxHeight = 4096;

	/**
	* Max resolution of the texture.
	* A smaller size will be used if the data fits.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture")
	int32 MaxWidth = 4096;

	/**
	* Enforce Power Of Two on texture resolutions.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture")
	bool bEnforcePowerOfTwo = false;

	/**
	* Texture Precision
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture")
	EVATPrecision Precision = EVATPrecision::EightBits;

	/**
	* AutoPlay will use Engine Time for driving the animation.
	* This will be used by UpdateMaterialInstanceFromDataAsset and AssetActions for setting MaterialInstance static switches
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	bool bAutoPlay = true;
	
	/**
	* AnimationIndex Index of the animation to play.
	* This will internally set Start and End Frame when using AutoPlay.
	* This will be used by UpdateMaterialInstanceFromDataAsset and AssetActions for setting MaterialInstance static switches
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material", meta = (EditCondition = "bAutoPlay"))
	int32 AnimationIndex = 0;

	/**
	* Frame to play
	* When not using AutoPlay, user is responsible of setting the frame.
	* This will be used by UpdateMaterialInstanceFromDataAsset and AssetActions for setting MaterialInstance static switches
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material", meta = (EditCondition = "!bAutoPlay"))
	int32 Frame = 0;
	
	/**
	* Number of Bone Influences for deformation. More influences will produce smoother results at the cost of performance.
	* This will be used by UpdateMaterialInstanceFromDataAsset and AssetActions for setting MaterialInstance static switches
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	EVATNumBoneInfluences NumBoneInfluences = EVATNumBoneInfluences::Four;
};