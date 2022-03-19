#include "PBDEditorFunctionLibrary.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"

static TArray<FVector> LoadPolyModel(const FString& InFilePath)
{
	TArray<FVector> Points = {};
	{
		TArray<FString> FileContentLines;
		if (FFileHelper::LoadFileToStringArray(FileContentLines, *InFilePath))
		{
			for (const FString& Line : FileContentLines)
			{
				FString PatternString(TEXT(R"( ?+[0-9]*: +([^\s]+) +([^\s]+) +([^\s]+))"));
				FRegexPattern Pattern(PatternString);
				FRegexMatcher Matcher(Pattern, Line);

				if (Matcher.FindNext())
				{
					FVector Position;

					Position.X = FCString::Atof(*Matcher.GetCaptureGroup(1));
					Position.Z = FCString::Atof(*Matcher.GetCaptureGroup(2));
					Position.Y = FCString::Atof(*Matcher.GetCaptureGroup(3));

					Points.Add(Position);
				}
			}
		}

		FVector CenterOfMass = FVector::ZeroVector;
		for (const FVector& Position : Points)
		{
			CenterOfMass += Position;
		}
		const float NumPoints = (float)Points.Num();
		CenterOfMass /= NumPoints;

		for (FVector& Position : Points)
		{
			Position -= CenterOfMass;
		}
	}
	return Points;
}

static void CreateTexture(const FString& PackagePath, const FString& FileName, int32 TextureWidth, int32 TextureHeight, int32 PixelSize, ETextureSourceFormat eTextureFormat, uint8* PixelData)
{
	// Create Package
	UPackage* Package = CreatePackage(*(PackagePath + FileName));

	// Create the Texture
	FName TextureName = FName(*FileName);
	UTexture2D* Texture = NewObject<UTexture2D>(Package, TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);

	// compression and mip settings
	Texture->CompressionSettings = TextureCompressionSettings::TC_HDR;
	Texture->LODGroup = TEXTUREGROUP_Pixels2D;
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->SRGB = false;
	Texture->AddressX = TextureAddress::TA_Clamp;
	Texture->AddressY = TextureAddress::TA_Clamp;

	// Updating Texture & mark it as unsaved
	Texture->AddToRoot();
	Texture->Source.Init(TextureWidth, TextureHeight, 1, 1, eTextureFormat, PixelData);	// make sure the data is saved on disk

	Texture->UpdateResource();
	Texture->PostEditChange();

	Package->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(Texture);
	Texture->MarkPackageDirty();
	Package->SetDirtyFlag(true);

	UE_LOG(LogTemp, Log, TEXT("Texture created: %s"), *FileName);
}

bool UPBDEditorFunctionLibrary::GeneratePBDShapeMatchingTextures(
	const FString& ParticlesFilePath,
	const FString& PackagePath,
	const FString& BaseFileName,
	float DistanceThreshold)
{
	TArray<FVector> ParticlePositions = LoadPolyModel(ParticlesFilePath);
	const int32 NumParticles = ParticlePositions.Num();
	if (NumParticles > 0)
	{
		// unreal doesn't support R32/R16 so create RGBAFloat textures :(
		const int32 PixelSize = sizeof(FFloat16) * 4;
		
		struct FShapeConstraint
		{
			TArray<int32> ParticleIndices = {};
		};

		struct FParticleConstraintList
		{
			TArray<int32> Constraints = {};
		};

		TArray<FShapeConstraint> ShapeConstraints;
		ShapeConstraints.SetNum(NumParticles); // one shape matching constraint per particle
		int32 MaxParticlesPerConstraint = 0;

		TArray<FParticleConstraintList> ParticleConstraintList;
		ParticleConstraintList.SetNum(NumParticles);
		int32 MaxConstraintsPerParticles = 0;

		// generate constraint data
		{
			for (int32 CurrentParticleIndex = 0; CurrentParticleIndex < NumParticles; ++CurrentParticleIndex)
			{
				const int32 CurrentShapeConstraintIndex = CurrentParticleIndex;

				// add current particle to constraint
				ShapeConstraints[CurrentParticleIndex].ParticleIndices.Add(CurrentParticleIndex);

				// add current constraint to current particle
				ParticleConstraintList[CurrentParticleIndex].Constraints.Add(CurrentShapeConstraintIndex);

				for (int32 OtherParticleIndex = 0; OtherParticleIndex < NumParticles; ++OtherParticleIndex)
				{
					if (CurrentParticleIndex != OtherParticleIndex)
					{
						const FVector& P0 = ParticlePositions[CurrentParticleIndex];
						const FVector& P1 = ParticlePositions[OtherParticleIndex];

						const float Distance = FVector::Distance(P0, P1);
						if (Distance <= DistanceThreshold)
						{
							// add other particle to constraint 
							ShapeConstraints[CurrentParticleIndex].ParticleIndices.Add(OtherParticleIndex);

							// add current cosntraint to other particle
							ParticleConstraintList[OtherParticleIndex].Constraints.Add(CurrentShapeConstraintIndex);

							MaxConstraintsPerParticles = FMath::Max(MaxConstraintsPerParticles, ParticleConstraintList[OtherParticleIndex].Constraints.Num());
						}
					}
				}

				MaxParticlesPerConstraint = FMath::Max(MaxParticlesPerConstraint, ShapeConstraints[CurrentParticleIndex].ParticleIndices.Num());
			}
		}

		if (MaxParticlesPerConstraint <= 0 || MaxConstraintsPerParticles <= 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[PBD] Unable to create constraint info, check \"Distance Threshold\""));

			return false;
		}

		// create position texture
		{
			TArray<FFloat16> ParticlePositionData;
			ParticlePositionData.SetNum(NumParticles * 4);
			for (int32 Index = 0; Index < NumParticles; ++Index)
			{
				ParticlePositionData[Index * 4 + 0] = ParticlePositions[Index].X;
				ParticlePositionData[Index * 4 + 1] = ParticlePositions[Index].Y;
				ParticlePositionData[Index * 4 + 2] = ParticlePositions[Index].Z;
				ParticlePositionData[Index * 4 + 3] = 1.f;
			}
			CreateTexture(PackagePath, BaseFileName + "_Positions", NumParticles, 1, PixelSize, ETextureSourceFormat::TSF_RGBA16F, (uint8*)ParticlePositionData.GetData());
		}

		// create shape constraint texture
		{
			const size_t TextureWidth = NumParticles;
			const size_t TextureHeight = MaxParticlesPerConstraint;
			TArray<FFloat16> TextureData;
			TextureData.SetNum(TextureWidth * TextureHeight * 4);
			for (int32 Index = 0; Index < (TextureWidth * TextureHeight); ++Index)
			{
				TextureData[Index * 4 + 0] = NumParticles;
				TextureData[Index * 4 + 1] = NumParticles;
				TextureData[Index * 4 + 2] = NumParticles;
				TextureData[Index * 4 + 3] = 1.f;
			}

			for (int32 i = 0; i < ShapeConstraints.Num(); ++i)
			{
				const FShapeConstraint& Constraint = ShapeConstraints[i];

				for (int32 j = 0; j < Constraint.ParticleIndices.Num(); ++j)
				{
					TextureData[(j * TextureWidth + i) * 4] = Constraint.ParticleIndices[j];
				}
			}

			CreateTexture(PackagePath, BaseFileName + "_ShapeConstraints", TextureWidth, TextureHeight, PixelSize, ETextureSourceFormat::TSF_RGBA16F, (uint8*)TextureData.GetData());
		}

		// create particle constraint list texture
		{
			const size_t TextureWidth = NumParticles;
			const size_t TextureHeight = MaxConstraintsPerParticles;
			TArray<FFloat16> TextureData;
			TextureData.SetNum(TextureWidth * TextureHeight * 4);
			for (int32 Index = 0; Index < (TextureWidth * TextureHeight); ++Index)
			{
				TextureData[Index * 4 + 0] = NumParticles;
				TextureData[Index * 4 + 1] = NumParticles;
				TextureData[Index * 4 + 2] = NumParticles;
				TextureData[Index * 4 + 3] = 1.f;
			}

			for (int32 i = 0; i < NumParticles; ++i)
			{
				for (int32 j = 0; j < ParticleConstraintList[i].Constraints.Num(); ++j)
				{
					TextureData[(j * TextureWidth + i) * 4] = ParticleConstraintList[i].Constraints[j];
				}
			}

			CreateTexture(PackagePath, BaseFileName + "_ConstraintList", TextureWidth, TextureHeight, PixelSize, ETextureSourceFormat::TSF_RGBA16F, (uint8*)TextureData.GetData());
		}

		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("[PBD] File(%s) contains no particle data"), *ParticlesFilePath);
	return false;
}