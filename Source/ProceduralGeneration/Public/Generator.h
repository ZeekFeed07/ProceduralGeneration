#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C:\myProg\UE\UE_5.1\Engine\Plugins\Runtime\ProceduralMeshComponent\Source\ProceduralMeshComponent\Public\ProceduralMeshComponent.h"

#include "Generator.generated.h"

class UProceduralMeshComponent;

UENUM(BlueprintType)
enum  class  NoiseType : uint8
{
	Flat,
	White,
	Perlin,
	PerlinOctaves,
	Simplex,
	SimplexOctaves,
	SinCos,
	Experimental
};

USTRUCT(BlueprintType)
struct FNoiseInfo
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
		int32 Seed = 35416;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		float Amplitude = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		float Scale = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main", meta = (ClampMin = 1, ClampMax = 6))
		int32 Octaves = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main", meta = (ClampMin = 0, ClampMax = 10))
		float Lacunarity = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main", meta = (ClampMin = 0, ClampMax = 1))
		float Persistence = 0.15f;
};

UCLASS()
class PROCEDURALGENERATION_API AGenerator : public AActor
{
	GENERATED_BODY()

public:
	AGenerator();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transform")
		USceneComponent* DefaultSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		NoiseType GenerationNoise = NoiseType::Perlin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
		UProceduralMeshComponent* LandscapeProceduralMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh"/*, meta = (EditCondition = "GenerationNoise == NoiseType::PerlinOctaves")*/)
		FNoiseInfo NoiseInfo = FNoiseInfo();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ClampMin = 0))
		float TileSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ClampMin = 1))
		int32 TileCount = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		float UVScale = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		UMaterialInterface* Material;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plyers Info")
		FVector OldLoc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Plyers Info")
		APawn* Character;

protected:
	virtual void BeginPlay() override;

	void NoiseOverlay(const FVector& CenterLoc);
	void GenerateLocation(const FVector& CenterLoc, float (* const NoiseFun)(const FVector2D&, const FNoiseInfo&));

	float MaxDistanceToGenerate;

public:
	// Обновление рзмера ландшафта
	void RefreshLandscapeSize();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;
private:
	TArray<FVector> Vertices = TArray<FVector>();
	TArray<FVector> Normals = TArray<FVector>();
	TArray<FVector2D> UV = TArray<FVector2D>();
	TArray<int32> Triangles = TArray<int32>();
};

inline float FlatNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo);
inline float WhiteNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo);
inline float PerlinNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo);
inline float PerlinOctavesNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo);
inline float SimplexNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo);
inline float SimplexOctavesNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo);
inline float SinCosNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo);
inline float ExperimentalNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo);

FORCEINLINE float PGrad2(int32 Hash, float X, float Y);
FORCEINLINE float SmoothCurve(float X);