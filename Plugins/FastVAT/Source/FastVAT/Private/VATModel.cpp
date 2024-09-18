#include "VATModel.h"

UVATModel::UVATModel()
{
	
}

void UVATModel::ResetInfo()
{
	// Common Info.
	NumFrames = 0;
	Animations.Reset();

	// Vertex Info
	// VertexRowsPerFrame.Empty();
	VertexMinBBox = FVector3f::ZeroVector;
	VertexSizeBBox = FVector3f::ZeroVector;

	// Bone Info
	NumBones = 0;
	// BoneRowsPerFrame.Empty();
	//BoneWeightRowsPerFrame.Empty();
	BoneMinBBox = FVector3f::ZeroVector;
	BoneSizeBBox = FVector3f::ZeroVector;
}

