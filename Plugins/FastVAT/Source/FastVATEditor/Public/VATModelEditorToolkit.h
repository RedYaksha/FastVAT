#pragma once
#include "CoreMinimal.h"
#include "SVATModelEditorViewport.h"
#include "VATMeshMapping.h"
#include "VATModel.h"
#include "Toolkits/AssetEditorToolkit.h"

class FVATModelEditorToolkit : public FAssetEditorToolkit
{
public:
	void InitEditor(const TArray<UObject*>& InObjects);

	
	void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;

	FName GetToolkitFName() const override;
	FText GetBaseToolkitName() const override;
	FString GetWorldCentricTabPrefix() const override;
	FLinearColor GetWorldCentricTabColorScale() const override;
	
protected:
	void BindCommands();
	void ExtendMenu();
	void ExtendToolbar();

	FString GetOutDirectoryPath();

	// automation steps
	void CreateTextures();

	// helpers
	UTexture2D* CreateTexture2DAsset(FString Path);
	FString CreateTexture2DName(FString Name, const int32 LODIndex);
	static UStaticMesh* ConvertSkeletalMeshToStaticMesh(USkeletalMesh* SkeletalMesh, const FString PackageName, const FVector2D LODRange);
	
	template <typename T>
	static T* CreateMaterialExpression(UMaterial* Material, int32 NodePosX, int32 NodePosY);


	// anim to texture
	static bool AnimationToTexture(UVATModel* InVATModel, const int32 LODIndex);
	static bool SetLightMapIndex(UStaticMesh* StaticMesh, const int32 LODIndex, const int32 LightmapIndex=1, bool bGenerateLightmapUVs=true);
	static void UpdateMaterialInstanceFromDataAsset(const UVATModel* InVATModel, const int32 LODIndex, class UMaterialInstanceConstant* MaterialInstance,
		const EMaterialParameterAssociation MaterialParameterAssociation = EMaterialParameterAssociation::LayerParameter);
	
	static bool CheckDataAsset(const UVATModel* InModel, const int32 LODIndex,
		int32& OutSocketIndex, TArray<FVATAnimSequenceInfo>& OutAnimSequences);
	
	// Returns Start, EndFrame and NumFrames in Animation
	static int32 GetAnimationFrameRange(const FVATAnimSequenceInfo& Animation, 
		int32& OutStartFrame, int32& OutEndFrame);

	// Get Vertex and Normals from Current Pose
	// The VertexDelta is returned from the RefPose
	static void GetVertexDeltasAndNormals(const USkeletalMeshComponent* SkeletalMeshComponent, const int32 LODIndex, 
		const FSourceMeshToDriverMesh& SourceMeshToDriverMesh,
		const FTransform RootTransform,
		TArray<FVector3f>& OutVertexDeltas, TArray<FVector3f>& OutVertexNormals);

	// Gets RefPose Bone Position and Rotations.
	static int32 GetRefBonePositionsAndRotations(const USkeletalMesh* SkeletalMesh, 
		TArray<FVector3f>& OutBoneRefPositions, TArray<FVector4f>& OutBoneRefRotations);

	// Gets Bone Position and Rotations for Current Pose.	
	// The BonePosition is returned relative to the RefPose
	static int32 GetBonePositionsAndRotations(const USkeletalMeshComponent* SkeletalMeshComponent, const TArray<FVector3f>& BoneRefPositions,
		TArray<FVector3f>& BonePositions, TArray<FVector4f>& BoneRotations);

	// Normalizes Deltas and Normals between [0-1] with Bounding Box
	static void NormalizeVertexData(
		const TArray<FVector3f>& Deltas, const TArray<FVector3f>& Normals,
		FVector3f& OutMinBBox, FVector3f& OutSizeBBox,
		TArray<FVector3f>& OutNormalizedDeltas, TArray<FVector3f>& OutNormalizedNormals);

	// Normalizes Positions and Rotations between [0-1] with Bounding Box
	static void NormalizeBoneData(
		const TArray<FVector3f>& Positions, const TArray<FVector4f>& Rotations,
		FVector3f& OutMinBBox, FVector3f& OutSizeBBox,
		TArray<FVector3f>& OutNormalizedPositions, TArray<FVector4f>& OutNormalizedRotations);

	/* Returns best resolution for the given data. 
	*  Returns false if data doesnt fit in the the max range */
	static bool FindBestResolution(const int32 NumFrames, const int32 NumElements,
								   int32& OutHeight, int32& OutWidth, int32& OutRowsPerFrame,
								   const int32 MaxHeight = 4096, const int32 MaxWidth = 4096, bool bEnforcePowerOfTwo = false);

	/* Sets Static Mesh FullPrecisionUVs Property*/
	static void SetFullPrecisionUVs(UStaticMesh* StaticMesh, const int32 LODIndex, bool bFullPrecision=true);

	/* Sets Static Mesh Bound Extensions */
	static void SetBoundsExtensions(UStaticMesh* StaticMesh, const FVector& MinBBox, const FVector& SizeBBox);

	/* Creates UV Coord with vertices */
	static bool CreateUVChannel(UStaticMesh* StaticMesh, const int32 LODIndex, const int32 UVChannelIndex,
		const int32 Height, const int32 Width);

protected:
	void ExecuteGenerateVAT();

private:
	TObjectPtr<UVATModel> VATModel;

	TSharedPtr<SVATModelEditorViewport> PreviewViewport;
};


