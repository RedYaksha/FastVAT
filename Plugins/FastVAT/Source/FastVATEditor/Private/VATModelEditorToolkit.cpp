#include "VATModelEditorToolkit.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "EditorAssetLibrary.h"
#include "IContentBrowserSingleton.h"
#include "MeshDescription.h"
#include "MeshUtilities.h"
#include "RawMesh.h"
#include "SVATModelEditorViewport.h"
#include "VATMeshMapping.h"
#include "VATModelEditorCommands.h"
#include "VATUtils.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/MaterialEditor/Public/MaterialEditingLibrary.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Factories/TextureFactory.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "Materials/MaterialAttributeDefinitionMap.h"
#include "Materials/MaterialExpressionBlendMaterialAttributes.h"
#include "Materials/MaterialExpressionExecEnd.h"
#include "Materials/MaterialExpressionGetMaterialAttributes.h"
#include "Materials/MaterialExpressionMakeMaterialAttributes.h"
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Rendering/NaniteResources.h"

#define LOCTEXT_NAMESPACE "VATModelEditor"

void FVATModelEditorToolkit::InitEditor(const TArray<UObject*>& InObjects)
{
	VATModel = Cast<UVATModel>(InObjects[0]);
	
	BindCommands();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("VATModelEditorLayout")
	->AddArea
	(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewSplitter()
			->SetSizeCoefficient(0.6f)
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.2f)
				->AddTab("VATModelDetailsTab", ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.8f)
				->AddTab("VATModelPreviewTab", ETabState::OpenedTab)
			)
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.4f)
			->AddTab("OutputLog", ETabState::OpenedTab)
		)
	);
	FAssetEditorToolkit::InitAssetEditor(EToolkitMode::Standalone, {}, "VATModelEditor", Layout, true, true, InObjects);

	ExtendMenu();
	ExtendToolbar();
	RegenerateMenusAndToolbars();
	
}

void FVATModelEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("VAT Model Editor"));

	TabManager->RegisterTabSpawner("VATModelPreviewTab", FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
	{
		SAssignNew(PreviewViewport, SVATModelEditorViewport);
		
		if(PreviewViewport)
		{
			PreviewViewport->SetStaticMesh(VATModel->StaticMesh);
		}
		
		return SNew(SDockTab)
		[
			PreviewViewport.ToSharedRef()
		];
	}))
	.SetDisplayName(INVTEXT("Preview"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	

	// TODO: customize property editor,
	// simple: only skeletal and animations
	// detailed: see textures, intermediate assets

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObjects(TArray<UObject*>{ VATModel });
	InTabManager->RegisterTabSpawner("VATModelDetailsTab", FOnSpawnTab::CreateLambda([=](const FSpawnTabArgs&)
	{
		return SNew(SDockTab)
		[
			DetailsView
		];
	}))
	.SetDisplayName(INVTEXT("Details"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FVATModelEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner("VATModelDetailsTab");
}

FName FVATModelEditorToolkit::GetToolkitFName() const
{
	return "VATModelEditor";
}

FText FVATModelEditorToolkit::GetBaseToolkitName() const
{
	return INVTEXT("VAT Model Editor");
}

FString FVATModelEditorToolkit::GetWorldCentricTabPrefix() const
{
	return "VAT";
}

FLinearColor FVATModelEditorToolkit::GetWorldCentricTabColorScale() const
{
	return {};
}

void FVATModelEditorToolkit::BindCommands()
{
	const FVATModelEditorCommands& Commands = FVATModelEditorCommands::Get();

	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();

	UICommandList->MapAction(Commands.GenerateVertexAnimationTextures,
		FExecuteAction::CreateSP(this, &FVATModelEditorToolkit::ExecuteGenerateVAT),
		FCanExecuteAction());
}

void FVATModelEditorToolkit::ExtendMenu()
{
}

void FVATModelEditorToolkit::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("Command");
			{
				ToolbarBuilder.AddToolBarButton(FVATModelEditorCommands::Get().GenerateVertexAnimationTextures);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		/*ViewportPtr->GetCommandList()*/ GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar )
		);

	AddToolbarExtender(ToolbarExtender);

	// custom extenders...
 	// IPaper2DEditorModule* Paper2DEditorModule = &FModuleManager::LoadModuleChecked<IPaper2DEditorModule>("Paper2DEditor");
 	// AddToolbarExtender(Paper2DEditorModule->GetFlipbookEditorToolBarExtensibilityManager()->GetAllExtenders());
}

FString FVATModelEditorToolkit::GetOutDirectoryPath()
{
	const FString PackageName = VATModel->GetOutermost()->GetName();
	const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
	return FPaths::Combine(PackagePath, VATModel.GetName() + "_GeneratedVAT");
}

void FVATModelEditorToolkit::CreateTextures()
{
	const FString Directory = GetOutDirectoryPath();

	// Mode: Vertex | Textures: Position, Normal
	// e.g. TX_VAT_<AssetName>_VertexPosition

	// Mode: Bone | Textures: Position, Rotation, Weight
	// e.g. TX_VAT_<AssetName>_BonePosition

	int NumLODs = (int) VATModel->LODRange.Y - (int) VATModel->LODRange.X + 1; // range is inclusive 

	VATModel->VertexPositionTextures.Empty();
	VATModel->VertexNormalTextures.Empty();
	VATModel->BoneWeightTextures.Empty();

	VATModel->BonePositionTexture = CreateTexture2DAsset(FPaths::Combine(Directory, CreateTexture2DName("BonePosition", -1)));
	VATModel->BoneRotationTexture = CreateTexture2DAsset(FPaths::Combine(Directory, CreateTexture2DName("BoneRotation", -1)));
	
	for(int i = 0; i < NumLODs; i++)
	{
		if(VATModel->Mode == EVATModelMode::Vertex)
		{
			VATModel->VertexPositionTextures.Add(CreateTexture2DAsset(FPaths::Combine(Directory, CreateTexture2DName("VertexPosition", i))) );
			VATModel->VertexNormalTextures.Add(CreateTexture2DAsset(FPaths::Combine(Directory, CreateTexture2DName("VertexNormal", i))) );
		}
		else if(VATModel->Mode == EVATModelMode::Bone)
		{
			VATModel->BoneWeightTextures.Add( CreateTexture2DAsset(FPaths::Combine(Directory, CreateTexture2DName("BoneWeight", i))) );
		}
	}
}

UTexture2D* FVATModelEditorToolkit::CreateTexture2DAsset(FString Path)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	
	UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
	
	FString PackageName;
	FString Name;
	AssetToolsModule.Get().CreateUniqueAssetName(Path, "", /*out*/ PackageName, /*out*/ Name);
	
	UObject* NewAsset = TextureFactory->CreateTexture2D(CreatePackage(*PackageName), FName(Name), RF_Standalone | RF_Public);

	UE_LOG(LogTemp, Log, TEXT("Creating %s"), *PackageName);
	
	if( NewAsset )
	{
		// package needs saving
		bool bSuccess = NewAsset->MarkPackageDirty();

		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(NewAsset);
	}

	return Cast<UTexture2D>(NewAsset);
}

FString FVATModelEditorToolkit::CreateTexture2DName(FString Name, const int32 LODIndex )
{
	if(LODIndex < 0)
	{
		return FString::Printf(TEXT("TX_VAT_%s_%s"), *VATModel.GetName(), *Name);
	}
	
	return FString::Printf(TEXT("TX_VAT_LOD_%d_%s_%s"), LODIndex, *VATModel.GetName(), *Name);
}

UStaticMesh* FVATModelEditorToolkit::ConvertSkeletalMeshToStaticMesh(USkeletalMesh* SkeletalMesh,
	const FString PackageName, const FVector2D LODRange)
{
	check(SkeletalMesh);

	if (PackageName.IsEmpty() || !FPackageName::IsValidObjectPath(PackageName))
	{
		return nullptr;
	}

	if(LODRange.X > LODRange.Y)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid LOD Range. X < Y. %s"), *LODRange.ToString());
		return nullptr;
	}

	for(int i = LODRange.X; i <= LODRange.Y; i++)
	{
		if (!SkeletalMesh->IsValidLODIndex(i))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid LODIndex: %i"), i);
			return nullptr;
		}
	}

	// Create Temp Actor
	check(GEditor);
	UWorld* World = GEditor->GetEditorWorldContext().World();
	check(World);
	AActor* Actor = World->SpawnActor<AActor>();
	check(Actor);

	// Create Temp SkeletalMesh Component
	USkeletalMeshComponent* MeshComponent = NewObject<USkeletalMeshComponent>(Actor);
	MeshComponent->RegisterComponent();
	MeshComponent->SetSkeletalMesh(SkeletalMesh);
	TArray<UMeshComponent*> MeshComponents = { MeshComponent };

	UStaticMesh* OutStaticMesh = nullptr;
	bool bGeneratedCorrectly = true;

	// Create New StaticMesh
	if (!FPackageName::DoesPackageExist(PackageName))
	{
		IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
		OutStaticMesh = MeshUtilities.ConvertMeshesToStaticMesh(MeshComponents, FTransform::Identity, PackageName);
	}
	// Update Existing StaticMesh
	else
	{
		// Load Existing Mesh
		OutStaticMesh = LoadObject<UStaticMesh>(nullptr, *PackageName);
	}

	if (OutStaticMesh)
	{
		// Create Temp Package.
		// because 
		UPackage* TransientPackage = GetTransientPackage();

		// Create Temp Mesh.
		IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
		UStaticMesh* TempMesh = MeshUtilities.ConvertMeshesToStaticMesh(MeshComponents, FTransform::Identity, TransientPackage->GetPathName());

		// make sure transactional flag is on
		TempMesh->SetFlags(RF_Transactional);

		// Copy a range of LODs
		if (LODRange.X != LODRange.Y)
		{
			const int32 NumSourceModels = TempMesh->GetNumSourceModels();
			OutStaticMesh->SetNumSourceModels((LODRange.Y - LODRange.X) + 1);

			if(LODRange.Y >= NumSourceModels)
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid Source Model Index: %i"), (int) LODRange.Y);
				bGeneratedCorrectly = false;
			}
			else if(LODRange.X < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid Source Model Index: %i"), (int) LODRange.X);
				bGeneratedCorrectly = false;
			}
			else
			{
				for (int32 Index = LODRange.X; Index <= LODRange.Y; ++Index)
				{
					UE_LOG(LogTemp, Log, TEXT("Saving Raw Mesh LOD: %d"), Index);
					
					// Get RawMesh
					FRawMesh RawMesh;
					TempMesh->GetSourceModel(Index).LoadRawMesh(RawMesh);

					// Set RawMesh
					OutStaticMesh->GetSourceModel(Index - LODRange.X).SaveRawMesh(RawMesh);
				}
			}
		}

		// Copy Single LOD
		else
		{
			const int LODIndex = LODRange.X;
			if (LODIndex >= TempMesh->GetNumSourceModels())
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid Source Model Index: %i"), LODIndex);
				bGeneratedCorrectly = false;
			}
			else
			{
				OutStaticMesh->SetNumSourceModels(1);

				// Get RawMesh
				FRawMesh RawMesh;
				TempMesh->GetSourceModel(LODIndex).LoadRawMesh(RawMesh);

				// Set RawMesh
				OutStaticMesh->GetSourceModel(0).SaveRawMesh(RawMesh);
			}
		}
			
		// Copy Materials
		const TArray<FStaticMaterial>& Materials = TempMesh->GetStaticMaterials();
		OutStaticMesh->SetStaticMaterials(Materials);

		// Done
		TArray<FText> OutErrors;
		OutStaticMesh->Build(true, &OutErrors);
		OutStaticMesh->MarkPackageDirty();
	}

	// Destroy Temp Component and Actor
	MeshComponent->UnregisterComponent();
	MeshComponent->DestroyComponent();
	Actor->Destroy();

	UE_LOG(LogTemp, Log, TEXT("NumLODs %d"), OutStaticMesh->GetNumLODs());
	
	check(OutStaticMesh->GetNumLODs() == (LODRange.Y - LODRange.X) + 1);

	return bGeneratedCorrectly ? OutStaticMesh : nullptr;
	
}

bool FVATModelEditorToolkit::AnimationToTexture(UVATModel* Model, const int32 LODIndex)
{
	if(!Model)
	{
		return false;
	}
	
	// Runs some checks for the assets in DataAsset
	int32 SocketIndex = INDEX_NONE;
	TArray<FVATAnimSequenceInfo> AnimSequences;
	if (!CheckDataAsset(Model, LODIndex, SocketIndex, AnimSequences))
	{
		return false;
	}

	// Reset DataAsset Info Values
	Model->ResetInfo();

	// ---------------------------------------------------------------------------		
	// Get Mapping between Static and Skeletal Meshes
	// Since they might not have same number of points.
	//
	FSourceMeshToDriverMesh Mapping;
	{
		FScopedSlowTask ProgressBar(1.f, LOCTEXT("ProcessingMapping", "Processing StaticMesh -> SkeletalMesh Mapping ..."), true /*Enabled*/);
		ProgressBar.MakeDialog(false /*bShowCancelButton*/, false /*bAllowInPIE*/);

		Mapping.Update(Model->GetStaticMesh(), LODIndex,
			Model->GetSkeletalMesh(), LODIndex, Model->Settings->NumDriverTriangles, Model->Settings->Sigma);
	}

	// Get Number of Source Vertices (StaticMesh)
	const int32 NumVertices = Mapping.GetNumSourceVertices();

	UE_LOG(LogTemp, Log, TEXT("LOD: %d Num Vertices: %d"), LODIndex, NumVertices);
	
	if (!NumVertices)
	{
		return false;
	}

	// ---------------------------------------------------------------------------
	// Get Reference Skeleton Transforms
	//
	TArray<FVector3f> BoneRefPositions;
	TArray<FVector4f> BoneRefRotations;
	TArray<FVector3f> BonePositions;
	TArray<FVector4f> BoneRotations;
	
	if (Model->Mode == EVATModelMode::Bone)
	{
		// Gets Ref Bone Position and Rotations.
		Model->NumBones = GetRefBonePositionsAndRotations(Model->GetSkeletalMesh(),
			BoneRefPositions, BoneRefRotations);

		// Add RefPose 
		// Note: this is added in the first frame of the Bone Position and Rotation Textures
		BonePositions.Append(BoneRefPositions);
		BoneRotations.Append(BoneRefRotations);
	}

	// --------------------------------------------------------------------------

	// Create Temp Actor
	check(GEditor);
	UWorld* World = GEditor->GetEditorWorldContext().World();
	check(World);

	AActor* Actor = World->SpawnActor<AActor>();
	check(Actor);

	// Create Temp SkeletalMesh Component
	USkeletalMeshComponent* SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(Actor);
	check(SkeletalMeshComponent);
	SkeletalMeshComponent->SetSkeletalMesh(Model->GetSkeletalMesh());

	// depends on first lod x range
	UE_LOG(LogTemp, Log, TEXT("Forcing LOD Skeleton: %f"), LODIndex + Model->LODRange.X);
	SkeletalMeshComponent->SetForcedLOD(LODIndex + Model->LODRange.X);
	
	SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SkeletalMeshComponent->SetUpdateAnimationInEditor(true);
	SkeletalMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	SkeletalMeshComponent->RegisterComponent();

	// ---------------------------------------------------------------------------
	// Get Vertex Data (for all frames)
	//		
	TArray<FVector3f> VertexDeltas;
	TArray<FVector3f> VertexNormals;
	
	// Get Animation Frames Data
	//
	for (int32 AnimSequenceIndex = 0; AnimSequenceIndex < AnimSequences.Num(); AnimSequenceIndex++)
	{
		const FVATAnimSequenceInfo& AnimSequenceInfo = AnimSequences[AnimSequenceIndex];

		// Set Animation
		UAnimSequence* AnimSequence = AnimSequenceInfo.AnimSequence;
		SkeletalMeshComponent->SetAnimation(AnimSequence);

		// Get Number of Frames
		int32 AnimStartFrame;
		int32 AnimEndFrame;
		const int32 AnimNumFrames = GetAnimationFrameRange(AnimSequenceInfo, AnimStartFrame, AnimEndFrame);
		const float AnimStartTime = AnimSequence->GetTimeAtFrame(AnimStartFrame);

		int32 SampleIndex = 0;
		const float SampleInterval = 1.f / Model->Settings->SampleRate;

		// Progress Bar
		FFormatNamedArguments Args;
		Args.Add(TEXT("AnimSequenceIndex"), AnimSequenceIndex+1);
		Args.Add(TEXT("NumAnimSequences"), AnimSequences.Num());
		Args.Add(TEXT("AnimSequence"), FText::FromString(*AnimSequence->GetFName().ToString()));
		FScopedSlowTask AnimProgressBar(AnimNumFrames, FText::Format(LOCTEXT("ProcessingAnimSequence", "Processing AnimSequence: {AnimSequence} [{AnimSequenceIndex}/{NumAnimSequences}]"), Args), true /*Enabled*/);
		AnimProgressBar.MakeDialog(false /*bShowCancelButton*/, false /*bAllowInPIE*/);

		while (SampleIndex < AnimNumFrames)
		{
			AnimProgressBar.EnterProgressFrame();

			const float Time = AnimStartTime + ((float)SampleIndex * SampleInterval);
			SampleIndex++;

			// Go To Time
			SkeletalMeshComponent->SetPosition(Time);
			// Update SkelMesh Animation.
			SkeletalMeshComponent->TickAnimation(0.f, false /*bNeedsValidRootMotion*/);
			SkeletalMeshComponent->RefreshBoneTransforms(nullptr /*TickFunction*/);
			
			// ---------------------------------------------------------------------------
			// Store Vertex Deltas & Normals.
			//
			if (Model->Mode == EVATModelMode::Vertex)
			{
				TArray<FVector3f> VertexFrameDeltas;
				TArray<FVector3f> VertexFrameNormals;

				GetVertexDeltasAndNormals(SkeletalMeshComponent, LODIndex,
					Mapping, Model->Settings->RootTransform,
					VertexFrameDeltas, VertexFrameNormals);
					
				VertexDeltas.Append(VertexFrameDeltas);
				VertexNormals.Append(VertexFrameNormals);
			}

			// ---------------------------------------------------------------------------
			// Store Bone Positions & Rotations
			//
			else if (Model->Mode == EVATModelMode::Bone)
			{
				TArray<FVector3f> BoneFramePositions;
				TArray<FVector4f> BoneFrameRotations;

				GetBonePositionsAndRotations(SkeletalMeshComponent, BoneRefPositions,
					BoneFramePositions, BoneFrameRotations);

				BonePositions.Append(BoneFramePositions);
				BoneRotations.Append(BoneFrameRotations);

			}
		} // End Frame

		// Store Anim Info Data
		FVATAnimInfo AnimInfo;
		AnimInfo.StartFrame = Model->NumFrames;
		AnimInfo.EndFrame = Model->NumFrames + AnimNumFrames - 1;
		Model->Animations.Add(AnimInfo);

		// Accumulate Frames
		Model->NumFrames += AnimNumFrames;

	} // End Anim
		
	// Destroy Temp Component & Actor
	SkeletalMeshComponent->UnregisterComponent();
	SkeletalMeshComponent->DestroyComponent();
	Actor->Destroy();
	
	// ---------------------------------------------------------------------------

	if (Model->Mode == EVATModelMode::Vertex)
	{
		// Find Best Resolution for Vertex Data
		int32 Height, Width;
		if (!FindBestResolution(Model->NumFrames, NumVertices, 
								Height, Width, Model->VertexRowsPerFrame[LODIndex], 
								Model->Settings->MaxHeight, Model->Settings->MaxWidth, Model->Settings->bEnforcePowerOfTwo))
		{
			UE_LOG(LogTemp, Warning, TEXT("Vertex Animation data cannot be fit in a %ix%i texture."), Model->Settings->MaxHeight, Model->Settings->MaxWidth);
			return false;
		}

		// Normalize Vertex Data
		TArray<FVector3f> NormalizedVertexDeltas;
		TArray<FVector3f> NormalizedVertexNormals;
		NormalizeVertexData(
			VertexDeltas, VertexNormals,
			Model->VertexMinBBox, Model->VertexSizeBBox,
			NormalizedVertexDeltas, NormalizedVertexNormals);

		// Write Textures
		if (Model->Settings->Precision == EVATPrecision::SixteenBits)
		{
			FVATUtils::WriteVectorsToTexture<FVector3f, FHighPrecision>(NormalizedVertexDeltas, Model->NumFrames, Model->VertexRowsPerFrame[LODIndex], Height, Width, Model->GetVertexPositionTexture(LODIndex));
			FVATUtils::WriteVectorsToTexture<FVector3f, FHighPrecision>(NormalizedVertexNormals, Model->NumFrames, Model->VertexRowsPerFrame[LODIndex], Height, Width, Model->GetVertexNormalTexture(LODIndex));
		}
		else
		{
			FVATUtils::WriteVectorsToTexture<FVector3f, FLowPrecision>(NormalizedVertexDeltas, Model->NumFrames, Model->VertexRowsPerFrame[LODIndex], Height, Width, Model->GetVertexPositionTexture(LODIndex));
			FVATUtils::WriteVectorsToTexture<FVector3f, FLowPrecision>(NormalizedVertexNormals, Model->NumFrames, Model->VertexRowsPerFrame[LODIndex], Height, Width, Model->GetVertexNormalTexture(LODIndex));
		}		

		// Add Vertex UVChannel
		CreateUVChannel(Model->GetStaticMesh(), LODIndex, Model->UVChannel, Height, Width);

		// Update Bounds
		SetBoundsExtensions(Model->GetStaticMesh(), (FVector)Model->VertexMinBBox, (FVector)Model->VertexSizeBBox);

		// Done with StaticMesh
		Model->GetStaticMesh()->PostEditChange();
	}

	// ---------------------------------------------------------------------------
	
	if (Model->Mode == EVATModelMode::Bone)
	{
		// Find Best Resolution for Bone Data
		int32 Height, Width;

		// Write Bone Position and Rotation Textures
		{
			// Note we are adding +1 frame for the ref pose
			if (!FindBestResolution(Model->NumFrames + 1, Model->NumBones,
				Height, Width, Model->BoneRowsPerFrame[LODIndex],
				Model->Settings->MaxHeight, Model->Settings->MaxWidth, Model->Settings->bEnforcePowerOfTwo))
			{
				UE_LOG(LogTemp, Warning, TEXT("Bone Animation data cannot be fit in a %ix%i texture."), Model->Settings->MaxHeight, Model->Settings->MaxWidth);
				return false;
			}

			// Normalize Bone Data
			TArray<FVector3f> NormalizedBonePositions;
			TArray<FVector4f> NormalizedBoneRotations;
			NormalizeBoneData(
				BonePositions, BoneRotations,
				Model->BoneMinBBox, Model->BoneSizeBBox,
				NormalizedBonePositions, NormalizedBoneRotations);


			// Write Textures
			if (Model->Settings->Precision == EVATPrecision::SixteenBits)
			{
				FVATUtils::WriteVectorsToTexture<FVector3f, FHighPrecision>(NormalizedBonePositions, Model->NumFrames + 1, Model->BoneRowsPerFrame[LODIndex], Height, Width, Model->GetBonePositionTexture());
				FVATUtils::WriteVectorsToTexture<FVector4f, FHighPrecision>(NormalizedBoneRotations, Model->NumFrames + 1, Model->BoneRowsPerFrame[LODIndex], Height, Width, Model->GetBoneRotationTexture());
			}
			else
			{
				FVATUtils::WriteVectorsToTexture<FVector3f, FLowPrecision>(NormalizedBonePositions, Model->NumFrames + 1, Model->BoneRowsPerFrame[LODIndex], Height, Width, Model->GetBonePositionTexture());
				FVATUtils::WriteVectorsToTexture<FVector4f, FLowPrecision>(NormalizedBoneRotations, Model->NumFrames + 1, Model->BoneRowsPerFrame[LODIndex], Height, Width, Model->GetBoneRotationTexture());
			}

			// Update Bounds
			SetBoundsExtensions(Model->GetStaticMesh(), (FVector)Model->BoneMinBBox, (FVector)Model->BoneSizeBBox);
		}

		// ---------------------------------------------------------------------------
		
		// Write Weights Texture
		{
			// Find Best Resolution for Bone Weights Texture
			if (!FindBestResolution(2, NumVertices,
				Height, Width, Model->BoneWeightRowsPerFrame[LODIndex],
				Model->Settings->MaxHeight, Model->Settings->MaxWidth, Model->Settings->bEnforcePowerOfTwo))
			{
				UE_LOG(LogTemp, Warning, TEXT("Weights Data cannot be fit in a %ix%i texture."), Model->Settings->MaxHeight, Model->Settings->MaxWidth);
				return false;
			}

			TArray<TVertexSkinWeight<4>> SkinWeights;

			// Reduce BoneWeights to 4 Influences.
			if (SocketIndex == INDEX_NONE)
			{
				// Project SkinWeights from SkeletalMesh to StaticMesh
				TArray<VertexSkinWeightMax> StaticMeshSkinWeights;
				Mapping.ProjectSkinWeights(StaticMeshSkinWeights);

				// Reduce Weights to 4 highest influences.
				FVATSkeletalMeshUtilities::ReduceSkinWeights(StaticMeshSkinWeights, SkinWeights);
			}
			// If Valid Socket, set all influences to same index.
			else
			{
				// Set all indices and weights to same SocketIndex
				SkinWeights.SetNumUninitialized(NumVertices);
				for (TVertexSkinWeight<4>& SkinWeight : SkinWeights)
				{
					SkinWeight.BoneWeights = TStaticArray<uint8, 4>(InPlace, 255);
					SkinWeight.MeshBoneIndices = TStaticArray<uint16, 4>(InPlace, SocketIndex);
				}
			}

			UE_LOG(LogTemp, Log, TEXT("SkinWeightsNum: %d"), SkinWeights.Num());

			// Write Bone Weights Texture
			if (Model->Settings->Precision == EVATPrecision::SixteenBits)
			{
				FVATUtils::WriteSkinWeightsToTexture<FHighPrecision>(SkinWeights, Model->NumBones,
					Model->BoneWeightRowsPerFrame[LODIndex], Height, Width, Model->GetBoneWeightTexture(LODIndex));
			}
			else
			{
				FVATUtils::WriteSkinWeightsToTexture<FLowPrecision>(SkinWeights, Model->NumBones,
					Model->BoneWeightRowsPerFrame[LODIndex], Height, Width, Model->GetBoneWeightTexture(LODIndex));
			}

			// Add Vertex UVChannel
			CreateUVChannel(Model->GetStaticMesh(), LODIndex, Model->UVChannel, Height, Width);
		}

		// Done with StaticMesh
		Model->GetStaticMesh()->PostEditChange();
	}

	// ---------------------------------------------------------------------------
	// Mark Packages dirty
	//
	Model->MarkPackageDirty();
	
	// All good here !
	return true;
}

bool FVATModelEditorToolkit::SetLightMapIndex(UStaticMesh* StaticMesh, const int32 LODIndex, const int32 LightmapIndex,
	bool bGenerateLightmapUVs)
{
	check(StaticMesh);

	if (!StaticMesh->IsSourceModelValid(LODIndex))
	{
		return false;
	}

	for (int32 Index=0; Index < LightmapIndex; Index++)
	{
		if (LightmapIndex > StaticMesh->GetNumUVChannels(LODIndex))
		{
			StaticMesh->AddUVChannel(LODIndex);
		}
	}

	// Set Build Settings
	FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel(LODIndex);
	SourceModel.BuildSettings.bGenerateLightmapUVs = bGenerateLightmapUVs;
	SourceModel.BuildSettings.DstLightmapIndex = LightmapIndex;
	StaticMesh->SetLightMapCoordinateIndex(LightmapIndex);

	// Build Mesh
	StaticMesh->Build(false);
	StaticMesh->PostEditChange();
	StaticMesh->MarkPackageDirty();

	return true;
	
}

void FVATModelEditorToolkit::UpdateMaterialInstanceFromDataAsset(const UVATModel* Model, const int32 LODIndex,
	UMaterialInstanceConstant* MaterialInstance, const EMaterialParameterAssociation MaterialParameterAssociation)
{
	check(Model);
	check(MaterialInstance);
	
	// Set UVChannel
	switch (Model->UVChannel)
	{
		case 0:
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV0, true, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV1, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV2, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV3, false, MaterialParameterAssociation);
			break;
		case 1:
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV0, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV1, true, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV2, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV3, false, MaterialParameterAssociation);
			break;
		case 2:
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV0, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV1, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV2, true, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV3, false, MaterialParameterAssociation);
			break;
		case 3:
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV0, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV1, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV2, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV3, true, MaterialParameterAssociation);
			break;
		default:
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV0, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV1, true, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV2, false, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseUV3, false, MaterialParameterAssociation);
			break;
	}

	// Update Vertex Params
	if (Model->Mode == EVATModelMode::Vertex)
	{
		UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MaterialInstance, VATParamNames::MinBBox, FLinearColor(Model->VertexMinBBox), MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MaterialInstance, VATParamNames::SizeBBox, FLinearColor(Model->VertexSizeBBox), MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::RowsPerFrame, Model->VertexRowsPerFrame[LODIndex], MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, VATParamNames::VertexPositionTexture, Model->GetVertexPositionTexture(LODIndex), MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, VATParamNames::VertexNormalTexture, Model->GetVertexNormalTexture(LODIndex), MaterialParameterAssociation);
	}

	// Update Bone Params
	else if (Model->Mode == EVATModelMode::Bone)
	{
		UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::NumBones, Model->NumBones, MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MaterialInstance, VATParamNames::MinBBox, FLinearColor(Model->BoneMinBBox), MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MaterialInstance, VATParamNames::SizeBBox, FLinearColor(Model->BoneSizeBBox), MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::RowsPerFrame, Model->BoneRowsPerFrame[LODIndex], MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::BoneWeightRowsPerFrame, Model->BoneWeightRowsPerFrame[LODIndex], MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, VATParamNames::BonePositionTexture, Model->GetBonePositionTexture(), MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, VATParamNames::BoneRotationTexture, Model->GetBoneRotationTexture(), MaterialParameterAssociation);
		UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, VATParamNames::BoneWeightsTexture, Model->GetBoneWeightTexture(LODIndex), MaterialParameterAssociation);

		// Num Influences
		switch (Model->Settings->NumBoneInfluences)
		{
			case EVATNumBoneInfluences::One:
				UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseTwoInfluences, false, MaterialParameterAssociation);
				UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseFourInfluences, false, MaterialParameterAssociation);
				break;
			case EVATNumBoneInfluences::Two:
				UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseTwoInfluences, true, MaterialParameterAssociation);
				UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseFourInfluences, false, MaterialParameterAssociation);
				break;
			case EVATNumBoneInfluences::Four:
				UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseTwoInfluences, false, MaterialParameterAssociation);
				UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::UseFourInfluences, true, MaterialParameterAssociation);
				break;
		}
	}

	// AutoPlay
	UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(MaterialInstance, VATParamNames::AutoPlay, Model->Settings->bAutoPlay, MaterialParameterAssociation);
	if (Model->Settings->bAutoPlay)
	{
		if (Model->Animations.IsValidIndex(Model->Settings->AnimationIndex))
		{
			UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::StartFrame, Model->Animations[Model->Settings->AnimationIndex].StartFrame, MaterialParameterAssociation);
			UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::EndFrame, Model->Animations[Model->Settings->AnimationIndex].EndFrame, MaterialParameterAssociation);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid AnimationIndex: %i"), Model->Settings->AnimationIndex);
		}
	}
	else
	{
		if (Model->Settings->Frame >= 0 && Model->Settings->Frame < Model->NumFrames)
		{
			UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::Frame, Model->Settings->Frame, MaterialParameterAssociation);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Frame out of range: %i"), Model->Settings->Frame);
		}
	}
	
	// NumFrames
	UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::NumFrames, Model->NumFrames, MaterialParameterAssociation);

	// SampleRate
	UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, VATParamNames::SampleRate, Model->Settings->SampleRate, MaterialParameterAssociation);

	// Update Material
	UMaterialEditingLibrary::UpdateMaterialInstance(MaterialInstance);

	// Rebuild Material
	UMaterialEditingLibrary::RebuildMaterialInstanceEditors(MaterialInstance->GetMaterial());

	// Set Preview Mesh
	if (Model->GetStaticMesh())
	{
		MaterialInstance->PreviewMesh = Model->GetStaticMesh();
	}

	MaterialInstance->MarkPackageDirty();
	
}

bool FVATModelEditorToolkit::CheckDataAsset(const UVATModel* Model, const int32 LODIndex, int32& OutSocketIndex,
                                            TArray<FVATAnimSequenceInfo>& OutAnimSequences)
{
	// Check StaticMesh
	if (!Model->GetStaticMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid StaticMesh"));
		return false;
	}

	// Check SkeletalMesh
	if (!Model->GetSkeletalMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid SkeletalMesh"));
		return false;
	}

	// Check Skeleton
	if (!Model->GetSkeletalMesh()->GetSkeleton())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid SkeletalMesh. No valid Skeleton found"));
		return false;
	}

	// Check Socket.
	/*
	OutSocketIndex = INDEX_NONE;
	if (Model->AttachToSocket.IsValid() && !Model->AttachToSocket.IsNone())
	{
		// Get Bone Names (no virtual)
		TArray<FName> BoneNames;
		GetBoneNames(Model->GetSkeletalMesh(), BoneNames);

		// Check if Socket is in BoneNames
		if (!BoneNames.Find(Model->AttachToSocket, OutSocketIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Socket: %s not found in Raw Bone List"), *Model->AttachToSocket.ToString());
			return false;
		}
		else
		{
			// TODO: SocketIndex can only be < TNumericLimits<uint16>::Max()
		}
	}
	*/

	// Check if UVChannel is being used by the Lightmap UV
	Model->GetStaticMesh()->Build();
	const FStaticMeshSourceModel& SourceModel = Model->GetStaticMesh()->GetSourceModel(LODIndex);
	
	if (SourceModel.BuildSettings.bGenerateLightmapUVs &&
		SourceModel.BuildSettings.DstLightmapIndex == Model->UVChannel)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid StaticMesh UVChannel: %i. Already used by LightMap"), Model->UVChannel);
		return false;
	}

	// Check if NumBones > 256
	const int32 NumBones = FVATSkeletalMeshUtilities::GetNumBones(Model->GetSkeletalMesh());
	if (Model->Settings->Precision == EVATPrecision::EightBits &&
		NumBones > 256)
	{
		UE_LOG(LogTemp, Warning, TEXT("Too many Bones: %i. There is a maximum of 256 bones for 8bit Precision"), NumBones);
		return false;
	}
	
	// Check Animations
	OutAnimSequences.Reset();
	for (const FVATAnimSequenceInfo& AnimSequenceInfo : Model->AnimSequences)
	{
		const UAnimSequence* AnimSequence = AnimSequenceInfo.AnimSequence;

		if (AnimSequenceInfo.bEnabled && AnimSequence)
		{
			// Make sure SkeletalMesh is compatible with AnimSequence
			if (!Model->GetSkeletalMesh()->GetSkeleton()->IsCompatibleForEditor(AnimSequence->GetSkeleton()))
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid AnimSequence: %s for given SkeletalMesh: %s"), *AnimSequence->GetFName().ToString(), *Model->GetSkeletalMesh()->GetFName().ToString());
				return false;
			}

			// Check Frame Range
			if (AnimSequenceInfo.bUseCustomRange &&
				(AnimSequenceInfo.StartFrame < 0 ||
					AnimSequenceInfo.EndFrame > AnimSequence->GetNumberOfSampledKeys() - 1 ||
					AnimSequenceInfo.EndFrame - AnimSequenceInfo.StartFrame < 0))
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid CustomRange for AnimSequence: %s"), *AnimSequence->GetName());
				return false;
			}

			// Store Valid AnimSequenceInfo
			OutAnimSequences.Add(AnimSequenceInfo);
		}
	}

	if (!OutAnimSequences.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid AnimSequences found"));
		return false;
	}

	// All Good !
	return true;
	
}

int32 FVATModelEditorToolkit::GetAnimationFrameRange(const FVATAnimSequenceInfo& Animation,
	int32& OutStartFrame, int32& OutEndFrame)
{
	if (!Animation.AnimSequence)
	{
		return INDEX_NONE;
	}

	// Get Range from AnimSequence
	if (!Animation.bUseCustomRange)
	{
		OutStartFrame = 0;
		OutEndFrame = Animation.AnimSequence->GetNumberOfSampledKeys() - 1; // AnimSequence->GetNumberOfFrames();
	}
	// Get Custom Range
	else
	{
		OutStartFrame = Animation.StartFrame;
		OutEndFrame = Animation.EndFrame;
	}

	// Return Number of Frames
	return OutEndFrame - OutStartFrame + 1;
}

void FVATModelEditorToolkit::GetVertexDeltasAndNormals(const USkeletalMeshComponent* SkeletalMeshComponent,
	const int32 LODIndex, const FSourceMeshToDriverMesh& SourceMeshToDriverMesh, const FTransform RootTransform,
	TArray<FVector3f>& OutVertexDeltas, TArray<FVector3f>& OutVertexNormals)
{
	OutVertexDeltas.Reset();
	OutVertexNormals.Reset();
		
	// Get Deformed vertices at current frame
	TArray<FVector3f> SkinnedVertices;
	FVATSkeletalMeshUtilities::GetSkinnedVertices(SkeletalMeshComponent, LODIndex, SkinnedVertices);
	
	// Get Source Vertices (StaticMesh)
	TArray<FVector3f> SourceVertices;
	const int32 NumVertices = SourceMeshToDriverMesh.GetSourceVertices(SourceVertices);

	// Deform Source Vertices with DriverMesh (SkeletalMesh
	TArray<FVector3f> DeformedVertices;
	TArray<FVector3f> DeformedNormals;
	SourceMeshToDriverMesh.DeformVerticesAndNormals(SkinnedVertices, DeformedVertices, DeformedNormals);

	// Allocate
	check(DeformedVertices.Num() == NumVertices && DeformedNormals.Num() == NumVertices);
	OutVertexDeltas.SetNumUninitialized(NumVertices);
	OutVertexNormals.SetNumUninitialized(NumVertices);

	// Transform Vertices and Normals with RootTransform
	for (int32 VertexIndex = 0; VertexIndex < NumVertices; VertexIndex++)
	{
		const FVector3f& SourceVertex   = SourceVertices[VertexIndex];
		const FVector3f& DeformedVertex = DeformedVertices[VertexIndex];
		const FVector3f& DeformedNormal = DeformedNormals[VertexIndex];
	
		// Transform Position and Delta with RootTransform
		const FVector3f TransformedVertexDelta = ((FVector3f)RootTransform.TransformPosition((FVector)DeformedVertex)) - SourceVertex;
		const FVector3f TransformedVertexNormal = (FVector3f)RootTransform.TransformVector((FVector)DeformedNormal);
		
		OutVertexDeltas[VertexIndex] = TransformedVertexDelta;
		OutVertexNormals[VertexIndex] = TransformedVertexNormal;
	}
}

int32 FVATModelEditorToolkit::GetRefBonePositionsAndRotations(const USkeletalMesh* SkeletalMesh,
	TArray<FVector3f>& OutBoneRefPositions, TArray<FVector4f>& OutBoneRefRotations)
{
	check(SkeletalMesh);

	OutBoneRefPositions.Reset();
	OutBoneRefRotations.Reset();

	// Get Number of RawBones (no virtual)
	const int32 NumBones = FVATSkeletalMeshUtilities::GetNumBones(SkeletalMesh);
	
	// Get Raw Ref Bone (no virtual)
	TArray<FTransform> RefBoneTransforms;
	FVATSkeletalMeshUtilities::GetRefBoneTransforms(SkeletalMesh, RefBoneTransforms);
	FVATSkeletalMeshUtilities::DecomposeTransformations(RefBoneTransforms, OutBoneRefPositions, OutBoneRefRotations);

	return NumBones;
}

int32 FVATModelEditorToolkit::GetBonePositionsAndRotations(const USkeletalMeshComponent* SkeletalMeshComponent,
	const TArray<FVector3f>& BoneRefPositions, TArray<FVector3f>& BonePositions, TArray<FVector4f>& BoneRotations)
{
	check(SkeletalMeshComponent);

	BonePositions.Reset();
	BoneRotations.Reset();

	// Get Relative Transforms
	// Note: Size is of Raw bones in SkeletalMesh. These are the original/raw bones of the asset, without Virtual Bones.
	TArray<FMatrix44f> RefToLocals;
	SkeletalMeshComponent->CacheRefToLocalMatrices(RefToLocals);
	const int32 NumBones = RefToLocals.Num();

	// check size
	check(NumBones == BoneRefPositions.Num());

	// Get Component Space Transforms
	// Note returns all transforms, including VirtualBones
	const TArray<FTransform>& CompSpaceTransforms = SkeletalMeshComponent->GetComponentSpaceTransforms();
	check(CompSpaceTransforms.Num() >= RefToLocals.Num());

	// Allocate
	BonePositions.SetNumUninitialized(NumBones);
	BoneRotations.SetNumUninitialized(NumBones);

	for (int32 BoneIndex = 0; BoneIndex < NumBones; BoneIndex++)
	{
		// Decompose Transformation (ComponentSpace)
		const FTransform& CompSpaceTransform = CompSpaceTransforms[BoneIndex];
		FVector3f BonePosition;
		FVector4f BoneRotation;
		FVATSkeletalMeshUtilities::DecomposeTransformation(CompSpaceTransform, BonePosition, BoneRotation);

		// Position Delta (from RefPose)
		const FVector3f Delta = BonePosition - BoneRefPositions[BoneIndex];

		// Decompose Transformation (Relative to RefPose)
		FVector3f BoneRelativePosition;
		FVector4f BoneRelativeRotation;
		const FMatrix RefToLocalMatrix(RefToLocals[BoneIndex]);
		const FTransform RelativeTransform(RefToLocalMatrix);
		FVATSkeletalMeshUtilities::DecomposeTransformation(RelativeTransform, BoneRelativePosition, BoneRelativeRotation);

		BonePositions[BoneIndex] = Delta;
		BoneRotations[BoneIndex] = BoneRelativeRotation;
	}

	return NumBones;
}

void FVATModelEditorToolkit::NormalizeVertexData(const TArray<FVector3f>& Deltas, const TArray<FVector3f>& Normals,
	FVector3f& OutMinBBox, FVector3f& OutSizeBBox, TArray<FVector3f>& OutNormalizedDeltas,
	TArray<FVector3f>& OutNormalizedNormals)
{
	check(Deltas.Num() == Normals.Num());

	// ---------------------------------------------------------------------------
	// Compute Bounding Box
	//
	OutMinBBox = { TNumericLimits<float>::Max(), TNumericLimits<float>::Max(), TNumericLimits<float>::Max() };
	FVector3f MaxBBox = { TNumericLimits<float>::Min(), TNumericLimits<float>::Min(), TNumericLimits<float>::Min() };
	
	for (const FVector3f& Delta: Deltas)
	{
		// Find Min/Max BoundingBox
		OutMinBBox.X = FMath::Min(Delta.X, OutMinBBox.X);
		OutMinBBox.Y = FMath::Min(Delta.Y, OutMinBBox.Y);
		OutMinBBox.Z = FMath::Min(Delta.Z, OutMinBBox.Z);

		MaxBBox.X = FMath::Max(Delta.X, MaxBBox.X);
		MaxBBox.Y = FMath::Max(Delta.Y, MaxBBox.Y);
		MaxBBox.Z = FMath::Max(Delta.Z, MaxBBox.Z);
	}

	OutSizeBBox = MaxBBox - OutMinBBox;

	// ---------------------------------------------------------------------------
	// Normalize Vertex Position Deltas
	// Basically we want all deltas to be between [0, 1]
	
	// Compute Normalization Factor per-axis.
	const FVector3f NormFactor = {
		1.f / static_cast<float>(OutSizeBBox.X),
		1.f / static_cast<float>(OutSizeBBox.Y),
		1.f / static_cast<float>(OutSizeBBox.Z) };

	OutNormalizedDeltas.SetNumUninitialized(Deltas.Num());
	for (int32 Index = 0; Index < Deltas.Num(); ++Index)
	{
		OutNormalizedDeltas[Index] = (Deltas[Index] - OutMinBBox) * NormFactor;
	}

	// ---------------------------------------------------------------------------
	// Normalize Vertex Normals
	// And move them to [0, 1]
	
	OutNormalizedNormals.SetNumUninitialized(Normals.Num());
	for (int32 Index = 0; Index < Normals.Num(); ++Index)
	{
		OutNormalizedNormals[Index] = (Normals[Index].GetSafeNormal() + FVector3f::OneVector) * 0.5f;
	}

}

void FVATModelEditorToolkit::NormalizeBoneData(const TArray<FVector3f>& Positions, const TArray<FVector4f>& Rotations,
	FVector3f& OutMinBBox, FVector3f& OutSizeBBox, TArray<FVector3f>& OutNormalizedPositions,
	TArray<FVector4f>& OutNormalizedRotations)
{
	check(Positions.Num() == Rotations.Num());

	// ---------------------------------------------------------------------------
	// Compute Position Bounding Box
	//
	OutMinBBox = { TNumericLimits<float>::Max(), TNumericLimits<float>::Max(), TNumericLimits<float>::Max() };
	FVector3f MaxBBox = { TNumericLimits<float>::Min(), TNumericLimits<float>::Min(), TNumericLimits<float>::Min() };

	for (const FVector3f& Position : Positions)
	{
		// Find Min/Max BoundingBox
		OutMinBBox.X = FMath::Min(Position.X, OutMinBBox.X);
		OutMinBBox.Y = FMath::Min(Position.Y, OutMinBBox.Y);
		OutMinBBox.Z = FMath::Min(Position.Z, OutMinBBox.Z);

		MaxBBox.X = FMath::Max(Position.X, MaxBBox.X);
		MaxBBox.Y = FMath::Max(Position.Y, MaxBBox.Y);
		MaxBBox.Z = FMath::Max(Position.Z, MaxBBox.Z);
	}

	OutSizeBBox = MaxBBox - OutMinBBox;

	// ---------------------------------------------------------------------------
	// Normalize Bone Position.
	// Basically we want all positions to be between [0, 1]

	// Compute Normalization Factor per-axis.
	const FVector3f NormFactor = {
		1.f / static_cast<float>(OutSizeBBox.X),
		1.f / static_cast<float>(OutSizeBBox.Y),
		1.f / static_cast<float>(OutSizeBBox.Z) };

	OutNormalizedPositions.SetNumUninitialized(Positions.Num());
	for (int32 Index = 0; Index < Positions.Num(); ++Index)
	{
		OutNormalizedPositions[Index] = (Positions[Index] - OutMinBBox) * NormFactor;
	}

	// ---------------------------------------------------------------------------
	// Normalize Rotations
	// And move them to [0, 1]
	OutNormalizedRotations.SetNumUninitialized(Rotations.Num());
	for (int32 Index = 0; Index < Rotations.Num(); ++Index)
	{
		const FVector4f Axis = Rotations[Index];
		const float Angle = Rotations[Index].W; // Angle are returned in radians and they go from [0-pi*2]

		OutNormalizedRotations[Index] = (Axis.GetSafeNormal() + FVector3f::OneVector) * 0.5f;
		OutNormalizedRotations[Index].W = Angle / (PI * 2.f);
	}
}

bool FVATModelEditorToolkit::FindBestResolution(const int32 NumFrames, const int32 NumElements, int32& OutHeight,
	int32& OutWidth, int32& OutRowsPerFrame, const int32 MaxHeight, const int32 MaxWidth, bool bEnforcePowerOfTwo)
{
	if (bEnforcePowerOfTwo)
	{
		OutWidth = 2;
		while (OutWidth < NumElements && OutWidth < MaxWidth)
		{
			OutWidth *= 2;
		}
		OutRowsPerFrame = FMath::CeilToInt(NumElements / (float)OutWidth);

		const int32 TargetHeight = NumFrames * OutRowsPerFrame;
		OutHeight = 2;
		while (OutHeight < TargetHeight)
		{
			OutHeight *= 2;
		}
	}
	else
	{
		OutRowsPerFrame = FMath::CeilToInt(NumElements / (float)MaxWidth);
		OutWidth = FMath::CeilToInt(NumElements / (float)OutRowsPerFrame);
		OutHeight = NumFrames * OutRowsPerFrame;
	}

	const bool bValidResolution = OutWidth <= MaxWidth && OutHeight <= MaxHeight;
	return bValidResolution;
}

void FVATModelEditorToolkit::SetFullPrecisionUVs(UStaticMesh* StaticMesh, const int32 LODIndex, bool bFullPrecision)
{
	check(StaticMesh);

	if (StaticMesh->IsSourceModelValid(LODIndex))
	{
		FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel(LODIndex);
		SourceModel.BuildSettings.bUseFullPrecisionUVs = bFullPrecision;
	}
}

void FVATModelEditorToolkit::SetBoundsExtensions(UStaticMesh* StaticMesh, const FVector& MinBBox,
	const FVector& SizeBBox)
{
	check(StaticMesh);

	// Calculate MaxBBox
	const FVector MaxBBox = SizeBBox + MinBBox;

	// Reset current extension bounds
	const FVector PositiveBoundsExtension = StaticMesh->GetPositiveBoundsExtension();
	const FVector NegativeBoundsExtension = StaticMesh->GetNegativeBoundsExtension();
		
	// Get current BoundingBox including extensions
	FBox BoundingBox = StaticMesh->GetBoundingBox();
		
	// Remove extensions from BoundingBox
	BoundingBox.Max -= PositiveBoundsExtension;
	BoundingBox.Min += NegativeBoundsExtension;
		
	// Calculate New BoundingBox
	FVector NewMaxBBox(
		FMath::Max(BoundingBox.Max.X, MaxBBox.X),
		FMath::Max(BoundingBox.Max.Y, MaxBBox.Y),
		FMath::Max(BoundingBox.Max.Z, MaxBBox.Z)
	);
		
	FVector NewMinBBox(
		FMath::Min(BoundingBox.Min.X, MinBBox.X),
		FMath::Min(BoundingBox.Min.Y, MinBBox.Y),
		FMath::Min(BoundingBox.Min.Z, MinBBox.Z)
	);

	// Calculate New Extensions
	FVector NewPositiveBoundsExtension = NewMaxBBox - BoundingBox.Max;
	FVector NewNegativeBoundsExtension = BoundingBox.Min - NewMinBBox;
				
	// Update StaticMesh
	StaticMesh->SetPositiveBoundsExtension(NewPositiveBoundsExtension);
	StaticMesh->SetNegativeBoundsExtension(NewNegativeBoundsExtension);
	StaticMesh->CalculateExtendedBounds();
}

bool FVATModelEditorToolkit::CreateUVChannel(UStaticMesh* StaticMesh, const int32 LODIndex, const int32 UVChannelIndex,
	const int32 Height, const int32 Width)
{
	check(StaticMesh);

	if (!StaticMesh->IsSourceModelValid(LODIndex))
	{
		return false;
	}

	// ----------------------------------------------------------------------------
	// Get Mesh Description.
	// This is needed for Inserting UVChannel
	FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(LODIndex);
	check(MeshDescription);

	// Add New UVChannel.
	if (UVChannelIndex == StaticMesh->GetNumUVChannels(LODIndex))
	{
		if (!StaticMesh->InsertUVChannel(LODIndex, UVChannelIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Unable to Add UVChannel"));
			
			return false;
		}
	}
	else if (UVChannelIndex > StaticMesh->GetNumUVChannels(LODIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("UVChannel: %i Out of Range. Number of existing UVChannels: %i"), UVChannelIndex, StaticMesh->GetNumUVChannels(LODIndex));
		return false;
	}

	// -----------------------------------------------------------------------------

	TMap<FVertexInstanceID, FVector2D> TexCoords;

	for (const FVertexInstanceID VertexInstanceID : MeshDescription->VertexInstances().GetElementIDs())
	{
		const FVertexID VertexID = MeshDescription->GetVertexInstanceVertex(VertexInstanceID);
		const int32 VertexIndex = VertexID.GetValue();

		// Instead
		// 
		float U = (0.5f / (float)Width) + (VertexIndex % Width) / (float)Width;
		float V = (0.5f / (float)Height) + (VertexIndex / Width) / (float)Height;
		
		TexCoords.Add(VertexInstanceID, FVector2D(U, V));
		StaticMesh->HasValidNaniteData();
		FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
		TPimplPtr<Nanite::FResources>& NaniteData = RenderData->NaniteResourcesPtr;
		NaniteData->NumClusters;
		// NaniteData->
	}

	// Set Full Precision UVs
	SetFullPrecisionUVs(StaticMesh, LODIndex, true);

	// Set UVs
	if (StaticMesh->SetUVChannel(LODIndex, UVChannelIndex, TexCoords))
	{
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to Set UVChannel: %i. TexCoords: %i"), UVChannelIndex, TexCoords.Num());
		return false;
	};

	return false;
}

void FVATModelEditorToolkit::ExecuteGenerateVAT()
{
	// TODO: start slow task, or should this be async
	
	// include timer "VAT generated in X seconds"
	UE_LOG(LogTemp, Log, TEXT("Executing VAT Generation"))

	
	// make intermediate directory if it doesn't exist
	const FString OutDirectoryPath = GetOutDirectoryPath();

	UE_LOG(LogTemp, Log, TEXT("Directory Path: %s"), *OutDirectoryPath);

	// remove the generated files directory
	if(UEditorAssetLibrary::DoesDirectoryExist(OutDirectoryPath))
	{
		UEditorAssetLibrary::DeleteDirectory(OutDirectoryPath);
	}

	check(!UEditorAssetLibrary::DoesDirectoryExist(OutDirectoryPath))
	
	UEditorAssetLibrary::MakeDirectory(OutDirectoryPath);
	

	CreateTextures();

	// TODO: choose which SKM LODs to use, and support > 1 LOD
	
	// create static mesh from source skeletal mesh
	FString SMPath = FPaths::Combine(OutDirectoryPath, "SM_VAT_" + VATModel.GetName());

	
	UStaticMesh* NewStaticMesh = ConvertSkeletalMeshToStaticMesh(VATModel->SkeletalMesh, SMPath, VATModel->LODRange);
	

	// Setup LOD Material Slots
	int NumLODs = NewStaticMesh->GetNumLODs();
	int OriginalNumMatSlots = VATModel->SkeletalMesh->GetNumMaterials();
	TArray<FStaticMaterial> StaticMaterialsOG = NewStaticMesh->GetStaticMaterials();
	check(StaticMaterialsOG.Num() == OriginalNumMatSlots)

	for(int i = 0; i < NumLODs - 1; i++)
	{
		for(auto M : StaticMaterialsOG)
		{
			NewStaticMesh->AddMaterial(M.MaterialInterface);
		}
	}

	FStaticMeshLODResourcesArray& LODResources = NewStaticMesh->GetRenderData()->LODResources;
	for(int i = 0; i < LODResources.Num(); i++)
	{
		FStaticMeshLODResources& LODModel = LODResources[i];
		
		for (int j = 0; j < LODModel.Sections.Num(); j++)
		{
			FMeshSectionInfo Info = NewStaticMesh->GetSectionInfoMap().Get(i, j);
			Info.MaterialIndex = LODModel.Sections.Num() * i + j;
			NewStaticMesh->GetSectionInfoMap().Set(i, j, Info);
		}
	}

	// create/modify materials from source base material and material instances
	TArray<FStaticMaterial> StaticMaterials = NewStaticMesh->GetStaticMaterials();
	
	// Names of all UMaterials for the static mesh (not instances) because
	// these need to be converted to material instances
	TSet<FString> BaseUMaterialNames;
	
	TQueue<UMaterialInterface*> StaticMaterialsQueue;
	for(auto M : StaticMaterials)
	{
		if(UMaterial* MaterialInterface = Cast<UMaterial>(M.MaterialInterface))
		{
			BaseUMaterialNames.Add(MaterialInterface->GetName());
		}
		
		StaticMaterialsQueue.Enqueue(M.MaterialInterface);
	}

	// original name -> material interface
	TMap<FString, UMaterialInterface*> MaterialsCreated;
	
	// these are the material instances for LOD 0 of the static mesh.
	// Will duplicate these Material Instances per LOD
	TMap<FString, UMaterialInstanceConstant*> SourceMatInstances;

	// All materials that are UMaterial* (not instances), must be converted to instances first.
	

	int MaxIter = 500;
	int CurIter = 0;

	while(!StaticMaterialsQueue.IsEmpty())
	{
		CurIter++;
		if(CurIter > MaxIter)
		{
			UE_LOG(LogTemp, Log, TEXT("Exceeded max iter."))
			break;
		}

		
		UMaterialInterface* M;
		StaticMaterialsQueue.Dequeue(M);


		// TODO: handle, duplicate asset names...
		if(MaterialsCreated.Contains(M->GetFullName()))
		{
			continue;
		}
		
		// m can either be a material interface or instance
		// ... and if it's instances, we'd need to copy the whole tree
		UE_LOG(LogTemp, Log, TEXT("Material Class Name: %s"), *M->GetClass()->GetName())
		const FString MatClassName = M->GetClass()->GetName();
		// ClassName = "Material" or "MaterialInstanceConstant"
		if(MatClassName == "Material")
		{
			FString Source = M->GetFullName();
			FString CopyDest = FPaths::Combine(OutDirectoryPath, "M_VAT_" + M->GetName() + "_" + VATModel.GetName() + "_" + FString::FromInt(CurIter));
			UMaterial* CopiedMat = Cast<UMaterial>(UEditorAssetLibrary::DuplicateAsset(Source, CopyDest));
			
			if(CopiedMat)
			{
				// add the proper nodes to the material
				auto* GetMatAttrs = CreateMaterialExpression<UMaterialExpressionGetMaterialAttributes>(CopiedMat, 500, 0);
				
				// /Script/Engine.MaterialFunction'/Engine/Functions/Engine_MaterialFunctions02/Utility/BlendAngleCorrectedNormals.BlendAngleCorrectedNormals'
				auto* BlendAngleCorrectedNormals = CreateMaterialExpression<UMaterialExpressionMaterialFunctionCall>(CopiedMat, 550, 0);
				check(BlendAngleCorrectedNormals);
				
				UMaterialFunctionInterface* MatFunc = LoadObject<UMaterialFunctionInterface>(nullptr,
					TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/BlendAngleCorrectedNormals.BlendAngleCorrectedNormals"));
				check(MatFunc);
				
				BlendAngleCorrectedNormals->SetMaterialFunction(MatFunc);

				
				auto* SetMatAttrs = CreateMaterialExpression<UMaterialExpressionSetMaterialAttributes>(CopiedMat, 600, 0);
				check(SetMatAttrs);
				
				auto* MatAttrLayers = CreateMaterialExpression<UMaterialExpressionMaterialAttributeLayers>(CopiedMat, 650, 0);
				check(MatAttrLayers);

				// /AnimToTexture/Materials/ML_BoneAnimation.ML_BoneAnimation
				// /AnimToTexture/Materials/ML_VertexAnimation.ML_VertexAnimation
				FString MaterialLayerPath;
				if(VATModel->Mode == EVATModelMode::Bone)
				{
					MaterialLayerPath = "/AnimToTexture/Materials/ML_BoneAnimation.ML_BoneAnimation";
				}
				else
				{
					MaterialLayerPath = "/AnimToTexture/Materials/ML_VertexAnimation.ML_VertexAnimation";
				}
				
				UMaterialFunctionInterface* Layer = LoadObject<UMaterialFunctionMaterialLayer>(nullptr, *MaterialLayerPath);
				
				MatAttrLayers->DefaultLayers.Layers[0] = Layer;
				MatAttrLayers->DefaultLayers.UnlinkLayerFromParent(0);
				
				auto* GetMatAttrs2 = CreateMaterialExpression<UMaterialExpressionGetMaterialAttributes>(CopiedMat, 700, 0);
				check(GetMatAttrs2);

				FExpressionInput* BaseNormalInput = BlendAngleCorrectedNormals->GetInput(0);
				FExpressionInput* AdditionalNormalInput = BlendAngleCorrectedNormals->GetInput(1);

				FString AttributeName;
				
				GetMatAttrs->AttributeGetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_Normal));
				
				AttributeName = FMaterialAttributeDefinitionMap::GetDisplayNameForMaterial(GetMatAttrs->AttributeGetTypes.Last(), CopiedMat).ToString();
				GetMatAttrs->Outputs.Add(FExpressionOutput(*AttributeName, 0, 0, 0, 0, 0));
				
				GetMatAttrs->AttributeGetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_WorldPositionOffset));
				
				AttributeName = FMaterialAttributeDefinitionMap::GetDisplayNameForMaterial(GetMatAttrs->AttributeGetTypes.Last(), CopiedMat).ToString();
				GetMatAttrs->Outputs.Add(FExpressionOutput(*AttributeName, 0, 0, 0, 0, 0));

				GetMatAttrs->ConnectExpression(AdditionalNormalInput, 1);

				MatAttrLayers->ConnectExpression(GetMatAttrs2->GetInput(0), 0);
				
				GetMatAttrs2->AttributeGetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_Normal));
				AttributeName = FMaterialAttributeDefinitionMap::GetDisplayNameForMaterial(GetMatAttrs2->AttributeGetTypes.Last(), CopiedMat).ToString();
				GetMatAttrs2->Outputs.Add(FExpressionOutput(*AttributeName, 0, 0, 0, 0, 0));
				
				GetMatAttrs2->AttributeGetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_WorldPositionOffset));
				AttributeName = FMaterialAttributeDefinitionMap::GetDisplayNameForMaterial(GetMatAttrs2->AttributeGetTypes.Last(), CopiedMat).ToString();
				GetMatAttrs2->Outputs.Add(FExpressionOutput(*AttributeName, 0, 0, 0, 0, 0));

				GetMatAttrs2->ConnectExpression(BaseNormalInput, 1);

				SetMatAttrs->AttributeSetTypes.Add(FMaterialAttributeDefinitionMap::GetID(MP_Normal));
				SetMatAttrs->Inputs.Add(FExpressionInput());
				SetMatAttrs->Inputs.Last().InputName = FName(*FMaterialAttributeDefinitionMap::GetDisplayNameForMaterial(SetMatAttrs->AttributeSetTypes.Last(), CopiedMat).ToString());
				
				SetMatAttrs->AttributeSetTypes.Add(FMaterialAttributeDefinitionMap::GetID(MP_WorldPositionOffset));
				SetMatAttrs->Inputs.Add(FExpressionInput());
				SetMatAttrs->Inputs.Last().InputName = FName(*FMaterialAttributeDefinitionMap::GetDisplayNameForMaterial(SetMatAttrs->AttributeSetTypes.Last(), CopiedMat).ToString());

				GetMatAttrs->ConnectExpression(SetMatAttrs->GetInput(0), 0);
				BlendAngleCorrectedNormals->ConnectExpression(SetMatAttrs->GetInput(1), 0);
				GetMatAttrs2->ConnectExpression(SetMatAttrs->GetInput(2), 2);


				// if bUseMaterialAttributes is false, all inputs of the main material node must be
				// redirected to a make material attributes then fed into getmaterialattributes
				if( ! CopiedMat->bUseMaterialAttributes )
				{
					auto* MakeMatAttrs = CreateMaterialExpression<UMaterialExpressionMakeMaterialAttributes>(CopiedMat, 600, 0);

					// connect make mat to first get mat attrs
					MakeMatAttrs->ConnectExpression(GetMatAttrs->GetInput(0), 0);

#define ConnectMaterialPropertyExpressionToMakeMatAttrDifferentName(Mat, MakeMatAttrsVar, MPName, MatAttrName) \
	{\
	FExpressionInput* Input = Mat->GetExpressionInputForProperty(MP_ ## MPName); \
	if(Input->Expression)\
	{\
		Input->Expression->ConnectExpression(&MakeMatAttrsVar->MatAttrName, Input->OutputIndex);\
	}\
	}\
	
#define ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(Mat, MakeMatAttrsVar, UVIndex) \
	{\
	FExpressionInput* Input = Mat->GetExpressionInputForProperty(MP_ ## CustomizedUVs ## UVIndex); \
	if(Input->Expression)\
	{\
		Input->Expression->ConnectExpression(&MakeMatAttrsVar->CustomizedUVs[UVIndex], Input->OutputIndex);\
	}\
	}\
	
#define ConnectMaterialPropertyExpressionToMakeMatAttr(Mat, MakeMatAttrsVar, Name) \
	{\
	FExpressionInput* Input = Mat->GetExpressionInputForProperty(MP_ ## Name); \
	if(Input->Expression)\
	{\
		Input->Expression->ConnectExpression(&MakeMatAttrsVar->Name, Input->OutputIndex);\
	}\
	}\
	
					ConnectMaterialPropertyExpressionToMakeMatAttrDifferentName(CopiedMat, MakeMatAttrs, CustomData0, ClearCoat)
					ConnectMaterialPropertyExpressionToMakeMatAttrDifferentName(CopiedMat, MakeMatAttrs, CustomData1, ClearCoatRoughness)

					// there's max of 8 (last index = 7) amount of customized UVs
					ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(CopiedMat, MakeMatAttrs, 0)
					ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(CopiedMat, MakeMatAttrs, 1)
					ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(CopiedMat, MakeMatAttrs, 2)
					ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(CopiedMat, MakeMatAttrs, 3)
					ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(CopiedMat, MakeMatAttrs, 4)
					ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(CopiedMat, MakeMatAttrs, 5)
					ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(CopiedMat, MakeMatAttrs, 6)
					ConnectMaterialPropertyExpressionToMakeMatAttrCustomUV(CopiedMat, MakeMatAttrs, 7)

					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, BaseColor)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Metallic)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Specular)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Roughness)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Anisotropy)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, EmissiveColor)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Opacity)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, OpacityMask)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Normal)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Tangent)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, WorldPositionOffset)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, SubsurfaceColor)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, AmbientOcclusion)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Refraction)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, PixelDepthOffset)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, ShadingModel)
					ConnectMaterialPropertyExpressionToMakeMatAttr(CopiedMat, MakeMatAttrs, Displacement)
				}
				
				CopiedMat->bUseMaterialAttributes = true;
				CopiedMat->bUsedWithInstancedStaticMeshes = true;
				CopiedMat->bUsedWithNanite = true;
				CopiedMat->BlendMode = BLEND_Opaque;
				CopiedMat->TwoSided = false;
				CopiedMat->bTangentSpaceNormal = false;
				
				FExpressionInput* MaterialAttributesInput = CopiedMat->GetExpressionInputForProperty(MP_MaterialAttributes);
				if (MaterialAttributesInput)
				{
					UMaterialExpression* PreviousFinalExpression = MaterialAttributesInput->Expression;
					if(PreviousFinalExpression)
					{
						PreviousFinalExpression->ConnectExpression(GetMatAttrs->GetInput(0), 0);
						SetMatAttrs->ConnectExpression(MaterialAttributesInput, 0);
					}
					else
					{
						UMaterialEditorOnlyData* EditorOnlyData = CopiedMat->GetEditorOnlyData();
						SetMatAttrs->ConnectExpression(&EditorOnlyData->MaterialAttributes, 0);
					}
				}
				else
				{
					UMaterialEditorOnlyData* EditorOnlyData = CopiedMat->GetEditorOnlyData();
					SetMatAttrs->ConnectExpression(&EditorOnlyData->MaterialAttributes, 0);
				}

				UMaterialEditingLibrary::LayoutMaterialExpressions(CopiedMat);
				UMaterialEditingLibrary::RecompileMaterial(CopiedMat);

				// is this material we're copying a base material of the static mesh?
				if(BaseUMaterialNames.Contains(M->GetName()))
				{
					TArray<UMaterialInterface*> Mats = {CopiedMat};
					
					UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
					Factory->InitialParent = CopiedMat;
					
					FString CopyName = "MI_VAT_" + M->GetName() + "_" + VATModel.GetName() + "_" + FString::FromInt(CurIter);
					UObject* ConvertedMIObj = IAssetTools::Get().CreateAsset(CopyName, OutDirectoryPath,
						UMaterialInstanceConstant::StaticClass(), Factory);

					/*
					IAssetTools::Get().CreateAssetsFrom<UMaterialInterface>(
						Mats, UMaterialInstanceConstant::StaticClass(), TEXT(""), [](UMaterialInterface* SourceObject)
						{
							UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
							Factory->InitialParent = SourceObject;
							return Factory;
						}
					);
					*/
					
					UMaterialInstanceConstant* ConvertedMI = Cast<UMaterialInstanceConstant>(ConvertedMIObj);
					check(ConvertedMI);
					
					MaterialsCreated.Add(M->GetFullName(), ConvertedMI);
					MaterialsCreated.Add(CopiedMat->GetFullName(), CopiedMat);
					SourceMatInstances.Add(M->GetFullName(), ConvertedMI);
				}
				else
				{
					MaterialsCreated.Add(M->GetFullName(), CopiedMat);
				}
			}
		}
		else if(MatClassName == "MaterialInstanceConstant")
		{
			// add it back to the queue, and try to re-parent it 
			FString Source = M->GetFullName();

			// is the parent made already?
			UMaterialInstanceConstant* SrcMI = Cast<UMaterialInstanceConstant>(M);
			check(SrcMI);

			FString ParentName;
			bool bHasParent = SrcMI->Parent != nullptr;
			if(bHasParent)
			{
				ParentName = SrcMI->Parent.GetFullName();
			}

			// instances can be parents to other instances... so we need a queue
			if(bHasParent && MaterialsCreated.Contains(ParentName))
			{
				FString CopyDest = FPaths::Combine(OutDirectoryPath, "MI_VAT_" + M->GetName() + "_" + VATModel.GetName() + "_" + FString::FromInt(CurIter));
				UMaterialInstanceConstant* CopiedMatInst = Cast<UMaterialInstanceConstant>(UEditorAssetLibrary::DuplicateAsset(Source, CopyDest));
				
				if(SrcMI->Parent != nullptr)
				{
					CopiedMatInst->SetParentEditorOnly(MaterialsCreated[ParentName]);
				}
				
				// copy all parameters
				// TODO: for some reason, textures aren't being copied (needed for face/clothes/etc.), but everything else is...
				CopiedMatInst->CopyMaterialUniformParametersEditorOnly(SrcMI);
				TArray<FTextureParameterValue> Textures = SrcMI->TextureParameterValues;
				for(FTextureParameterValue V : Textures)
				{
					UE_LOG(LogTemp, Log, TEXT("%s"), *V.ParameterInfo.Name.ToString())
					CopiedMatInst->SetTextureParameterValueEditorOnly(V.ParameterInfo, V.ParameterValue);
				}
				
				MaterialsCreated.Add(M->GetFullName(), CopiedMatInst);
				SourceMatInstances.Add(M->GetFullName(), SrcMI);
			}
			else
			{
				// requeue
				UE_LOG(LogTemp, Log, TEXT("Parent not found: %s"), *ParentName);
				
				// add to the parent to the queue, just in case
				StaticMaterialsQueue.Enqueue(SrcMI->Parent);
				StaticMaterialsQueue.Enqueue(M);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Material Class not handled: %s"), *MatClassName);
		}
	}

	TArray< TMap<FString, UMaterialInstanceConstant*> > MaterialInstancesCreatedByLOD;
	
	// duplicate the material instances per LOD
	for(int i = 1; i < NumLODs; i++)
	{
		TArray<UMaterialInstanceConstant*> SourceMatInstancesObjs;
		SourceMatInstances.GenerateValueArray(SourceMatInstancesObjs);

		TMap<FString, UMaterialInstanceConstant*> MICreated;
		
		TQueue<FString> MINameQueue;
		
		for(auto MI : SourceMatInstances)
		{
			MINameQueue.Enqueue(MI.Key);
		}

		MaxIter = 100;
		CurIter = 0;

		while(!MINameQueue.IsEmpty())
		{
			CurIter++;
			if(CurIter > MaxIter) {
				UE_LOG(LogTemp, Log, TEXT("Exceeded maximum iterations"))
				break;
			}

			FString CurName;
			MINameQueue.Dequeue(CurName);

			check(SourceMatInstances.Contains(CurName))
			UMaterialInstanceConstant* CurMI = SourceMatInstances[CurName];
			
			FString Source = CurMI->GetFullName();

			bool bParentIsBaseMaterial = Cast<UMaterial>(CurMI->Parent) != nullptr;
			
			FString ParentName = "";
			if(CurMI->Parent != nullptr)
			{
				ParentName = CurMI->Parent.GetFullName();
			}
			
			
			if(bParentIsBaseMaterial || MICreated.Contains(ParentName))
			{
				FString OutAssetName = FString::Printf(TEXT("MI_VAT_LOD_%d_%s_%s"), i, *CurMI->GetName(), *VATModel.GetName());
				FString CopyDest = FPaths::Combine(OutDirectoryPath, OutAssetName);
				
				UE_LOG(LogTemp, Log, TEXT("%s %s"), *ParentName, *OutAssetName);
				
				UMaterialInstanceConstant* CopiedMatInst = Cast<UMaterialInstanceConstant>(UEditorAssetLibrary::DuplicateAsset(Source, CopyDest));

				if(bParentIsBaseMaterial)
				{
					check(MaterialsCreated.Contains(ParentName))
					
					CopiedMatInst->SetParentEditorOnly(MaterialsCreated[ParentName]);
				}
				else // parent is MI
				{
					CopiedMatInst->SetParentEditorOnly(MICreated[ParentName]);
				}

				MICreated.Add(CurName, CopiedMatInst);
			}
			else
			{
				// requeue, shouldn't need to add parent back to queue
				MINameQueue.Enqueue(CurName);
			}
		}

		MaterialInstancesCreatedByLOD.Add(MICreated);
	}

	// assign materials to static mesh
	for(int i = 0; i < OriginalNumMatSlots; i++)
	{
		FString MatName = StaticMaterials[i].MaterialInterface.GetFullName();
		if(MaterialsCreated.Contains(MatName))
		{
			NewStaticMesh->SetMaterial(i, MaterialsCreated[MatName]);
			for(int LODIndex = 1; LODIndex < NumLODs; LODIndex++)
			{
				check(MaterialInstancesCreatedByLOD.IsValidIndex(LODIndex - 1));

				TMap<FString, UMaterialInstanceConstant*> MaterialInstancesCreatedCurLOD = MaterialInstancesCreatedByLOD[LODIndex - 1];
				check(MaterialInstancesCreatedCurLOD.Contains(MatName));
				
				UMaterialInstanceConstant* MI = MaterialInstancesCreatedByLOD[LODIndex - 1][MatName];
				
				
				NewStaticMesh->SetMaterial(LODIndex * OriginalNumMatSlots + i, MI);
			}
		}
	}

	// update VATModel
	VATModel->StaticMesh = NewStaticMesh;


	//
	VATModel->BoneRowsPerFrame.AddDefaulted(NumLODs);
	VATModel->BoneWeightRowsPerFrame.AddDefaulted(NumLODs);
	VATModel->VertexRowsPerFrame.AddDefaulted(NumLODs);
	
	// perform the AnimToTexture automation (fill data for the textures)
	// per lod
	for(int i = 0; i < NumLODs; i++)
	{
		SetLightMapIndex(NewStaticMesh, i, 2, true);
		AnimationToTexture(VATModel, i);
	}

	// set material parameters

	// LOD 0
	for(auto M : MaterialsCreated)
	{
		UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(M.Value);
		if(!MI)
		{
			continue;
		}

		UpdateMaterialInstanceFromDataAsset(VATModel, 0, MI, EMaterialParameterAssociation::LayerParameter);
	}

	// LOD >= 1 
	for(int LODIndex = 1; LODIndex < NumLODs; LODIndex++)
	{
		auto MIMap = MaterialInstancesCreatedByLOD[LODIndex - 1];
		for(auto Tuple : MIMap)
		{
			UpdateMaterialInstanceFromDataAsset(VATModel, LODIndex, Tuple.Value, EMaterialParameterAssociation::LayerParameter);
		}
	}
	
	// done

	// update the viewport to show final static mesh
	if(PreviewViewport)
	{
		PreviewViewport->SetStaticMesh(NewStaticMesh);
	}
}

template <typename T>
T* FVATModelEditorToolkit::CreateMaterialExpression(UMaterial* Material, int32 NodePosX, int32 NodePosY)
{
	UMaterialExpression* Expression = Cast<T>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, T::StaticClass(), NodePosX, NodePosY)
	);

	return Cast<T>(Expression);
}

#undef LOCTEXT_NAMESPACE