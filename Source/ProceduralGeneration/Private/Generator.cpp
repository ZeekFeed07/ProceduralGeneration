#include "Generator.h"
#include <limits>
#include <chrono>
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Async/Async.h"

DECLARE_LOG_CATEGORY_EXTERN(MyLogCategory, Log, All);
DEFINE_LOG_CATEGORY(MyLogCategory);

static const int32 NoisePerm[512] = {
		63, 9, 212, 205, 31, 128, 72, 59, 137, 203, 195, 170, 181, 115, 165, 40, 116, 139, 175, 225, 132, 99, 222, 2, 41, 15, 197, 93, 169, 90, 228, 43, 221, 38, 206, 204, 73, 17, 97, 10, 96, 47, 32, 138, 136, 30, 219,
		78, 224, 13, 193, 88, 134, 211, 7, 112, 176, 19, 106, 83, 75, 217, 85, 0, 98, 140, 229, 80, 118, 151, 117, 251, 103, 242, 81, 238, 172, 82, 110, 4, 227, 77, 243, 46, 12, 189, 34, 188, 200, 161, 68, 76, 171, 194,
		57, 48, 247, 233, 51, 105, 5, 23, 42, 50, 216, 45, 239, 148, 249, 84, 70, 125, 108, 241, 62, 66, 64, 240, 173, 185, 250, 49, 6, 37, 26, 21, 244, 60, 223, 255, 16, 145, 27, 109, 58, 102, 142, 253, 120, 149, 160,
		124, 156, 79, 186, 135, 127, 14, 121, 22, 65, 54, 153, 91, 213, 174, 24, 252, 131, 192, 190, 202, 208, 35, 94, 231, 56, 95, 183, 163, 111, 147, 25, 67, 36, 92, 236, 71, 166, 1, 187, 100, 130, 143, 237, 178, 158,
		104, 184, 159, 177, 52, 214, 230, 119, 87, 114, 201, 179, 198, 3, 248, 182, 39, 11, 152, 196, 113, 20, 232, 69, 141, 207, 234, 53, 86, 180, 226, 74, 150, 218, 29, 133, 8, 44, 123, 28, 146, 89, 101, 154, 220, 126,
		155, 122, 210, 168, 254, 162, 129, 33, 18, 209, 61, 191, 199, 157, 245, 55, 164, 167, 215, 246, 144, 107, 235,

		63, 9, 212, 205, 31, 128, 72, 59, 137, 203, 195, 170, 181, 115, 165, 40, 116, 139, 175, 225, 132, 99, 222, 2, 41, 15, 197, 93, 169, 90, 228, 43, 221, 38, 206, 204, 73, 17, 97, 10, 96, 47, 32, 138, 136, 30, 219,
		78, 224, 13, 193, 88, 134, 211, 7, 112, 176, 19, 106, 83, 75, 217, 85, 0, 98, 140, 229, 80, 118, 151, 117, 251, 103, 242, 81, 238, 172, 82, 110, 4, 227, 77, 243, 46, 12, 189, 34, 188, 200, 161, 68, 76, 171, 194,
		57, 48, 247, 233, 51, 105, 5, 23, 42, 50, 216, 45, 239, 148, 249, 84, 70, 125, 108, 241, 62, 66, 64, 240, 173, 185, 250, 49, 6, 37, 26, 21, 244, 60, 223, 255, 16, 145, 27, 109, 58, 102, 142, 253, 120, 149, 160,
		124, 156, 79, 186, 135, 127, 14, 121, 22, 65, 54, 153, 91, 213, 174, 24, 252, 131, 192, 190, 202, 208, 35, 94, 231, 56, 95, 183, 163, 111, 147, 25, 67, 36, 92, 236, 71, 166, 1, 187, 100, 130, 143, 237, 178, 158,
		104, 184, 159, 177, 52, 214, 230, 119, 87, 114, 201, 179, 198, 3, 248, 182, 39, 11, 152, 196, 113, 20, 232, 69, 141, 207, 234, 53, 86, 180, 226, 74, 150, 218, 29, 133, 8, 44, 123, 28, 146, 89, 101, 154, 220, 126,
		155, 122, 210, 168, 254, 162, 129, 33, 18, 209, 61, 191, 199, 157, 245, 55, 164, 167, 215, 246, 144, 107, 235,
};

static const FVector2D Grad2[8] = {
	{1, 1}, {-1, 1}, {1, -1}, {-1, -1},
	{1, 0}, {-1, 0}, {0, 1}, {0, -1}
};

AGenerator::AGenerator()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("GIZMO"));
	RootComponent = DefaultSceneComponent;

	LandscapeProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralLandscape"));
	LandscapeProceduralMesh->SetupAttachment(DefaultSceneComponent);

	RefreshLandscapeSize();
}

void AGenerator::BeginPlay()
{
	Super::BeginPlay();

	RefreshLandscapeSize();
	Character = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (Character)
	{
		OldLoc = Character->GetActorLocation();
		NoiseOverlay(OldLoc);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Wrong `Character` pointer in method `AGenerator::BeginPlay`."))
	}
}

void AGenerator::RefreshLandscapeSize()
{
	Vertices.SetNum(FMath::Square(TileCount + 1));
	Normals.SetNum(FMath::Square(TileCount + 1));
	UV.SetNum(FMath::Square(TileCount + 1));
	Triangles.SetNum(FMath::Square(TileCount) * 6);

	MaxDistanceToGenerate = TileCount / 4 * TileSize;
}

void AGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshLandscapeSize();

	if (LandscapeProceduralMesh)
	{
		NoiseOverlay(Transform.GetLocation());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Wrong `LandscapeProceduralMesh` pointer in method `AGenerator::OnConstruction`."))
	}
}

void AGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CurrentLoc = Character->GetActorLocation();

	/*
	*  Перегенерировать ландшафт если расстояние от
	*  середины тайла до игрока больше чем половина тайла
	*/
	if (FVector(CurrentLoc - OldLoc).SquaredLength() >= FMath::Pow(MaxDistanceToGenerate, 2))
	{
		NoiseOverlay(CurrentLoc);
		OldLoc = CurrentLoc;
	}
}

void AGenerator::NoiseOverlay(const FVector& CenterLoc)
{
	switch (GenerationNoise)
	{
	case NoiseType::Flat:
		GenerateLocation(CenterLoc, FlatNoise);
		break;
	case NoiseType::White:
		GenerateLocation(CenterLoc, WhiteNoise);
		break;
	case NoiseType::Perlin:
		GenerateLocation(CenterLoc, PerlinNoise);
		break;
	case NoiseType::PerlinOctaves:
		GenerateLocation(CenterLoc, PerlinOctavesNoise);
		break;
	case NoiseType::Simplex:
		GenerateLocation(CenterLoc, SimplexNoise);
		break;
	case NoiseType::SimplexOctaves:
		GenerateLocation(CenterLoc, SimplexOctavesNoise);
		break;
	case NoiseType::SinCos:
		GenerateLocation(CenterLoc, SinCosNoise);
		break;
	case NoiseType::Experimental:
		GenerateLocation(CenterLoc, ExperimentalNoise);
		break;
	default:
		UE_LOG(LogTemp, Error, TEXT("Wrong type of Noise."));
		return;
	}

	LandscapeProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV,
		TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
	LandscapeProceduralMesh->SetMaterial(0, Material);
	LandscapeProceduralMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}

void AGenerator::GenerateLocation(const FVector& CenterLoc, float (* const NoiseFun)(const FVector2D&, const FNoiseInfo&))
{
	double startTime = FPlatformTime::Seconds();

	const float TileHalfSize = TileSize * 0.5;
	float RandVal = 0;
	FVector CellLoc1 = FVector::Zero();
	FVector CellLoc2 = FVector::Zero();
	FVector CellLoc3 = FVector::Zero();
	FVector CellLoc4 = FVector::Zero();

#define RANGE(i, j) ((i) * (TileCount + 1) + (j))
#define RANGE6(i, j) (6 * i) * (TileCount) + 6 * j
#define NI NoiseInfo

	FRandomStream RandomStream(NI.Seed);
	RandVal = RandomStream.RandRange(1, std::numeric_limits<int32>::max() / 20);

	for (int32 i = 0; i < TileCount; ++i)
	{
		CellLoc3.Y = CellLoc1.Y = TileHalfSize * (2 * i - TileCount) + CenterLoc.Y;
		CellLoc4.Y = CellLoc2.Y = TileHalfSize * (2 * (i + 1) - TileCount) + CenterLoc.Y;

		for (int32 j = 0; j < TileCount; ++j)
		{
			// Инициализация вершин
			CellLoc2.X = CellLoc1.X = TileHalfSize * (2 * j - TileCount) + CenterLoc.X;
			CellLoc4.X = CellLoc3.X = TileHalfSize * (2 * (j + 1) - TileCount) + CenterLoc.X;

			// Наложение шума и инициализация высоты
			CellLoc1.Z = NoiseFun(FVector2D(CellLoc1.X + RandVal, CellLoc1.Y + RandVal) / NI.Scale, NI) * NI.Amplitude;
			CellLoc2.Z = NoiseFun(FVector2D(CellLoc2.X + RandVal, CellLoc2.Y + RandVal) / NI.Scale, NI) * NI.Amplitude;
			CellLoc3.Z = NoiseFun(FVector2D(CellLoc3.X + RandVal, CellLoc3.Y + RandVal) / NI.Scale, NI) * NI.Amplitude;
			CellLoc4.Z = NoiseFun(FVector2D(CellLoc4.X + RandVal, CellLoc4.Y + RandVal) / NI.Scale, NI) * NI.Amplitude;

			Vertices[RANGE(i, j)] = CellLoc1;
			Vertices[RANGE(i + 1, j)] = CellLoc2;
			Vertices[RANGE(i, j + 1)] = CellLoc3;
			Vertices[RANGE(i + 1, j + 1)] = CellLoc4;

			// Инициализация нормалей
			Normals[RANGE(i, j)] =
				Normals[RANGE(i + 1, j)] =
				Normals[RANGE(i, j + 1)] =
				-FVector::CrossProduct(Vertices[RANGE(i + 1, j)] - Vertices[RANGE(i, j)], Vertices[RANGE(i, j + 1)] - Vertices[RANGE(i, j)]).GetSafeNormal();

			// Обход треугольников
			// Треугольник 1
			Triangles[RANGE6(i, j)] = RANGE(i, j);
			Triangles[RANGE6(i, j + 1)] = RANGE(i + 1, j);
			Triangles[RANGE6(i, j + 2)] = RANGE(i, j + 1);
			// Треугольник 2 
			Triangles[RANGE6(i, j + 3)] = RANGE(i + 1, j);
			Triangles[RANGE6(i, j + 4)] = RANGE(i + 1, j + 1);
			Triangles[RANGE6(i, j + 5)] = RANGE(i, j + 1);

			// Инициализация UV координат
			UV[RANGE(i, j)] = FVector2D(i, j) * UVScale;
			UV[RANGE(i + 1, j)] = FVector2D(i + 1, j) * UVScale;
			UV[RANGE(i, j + 1)] = FVector2D(i, j + 1) * UVScale;
			UV[RANGE(i + 1, j + 1)] = FVector2D(i + 1, j + 1) * UVScale;

		}
	}
#undef NI
#undef TRIANGLES
#undef RANGE
	double endTime = FPlatformTime::Seconds();

	double duration = endTime - startTime;
	UE_LOG(MyLogCategory, Warning, TEXT("code executed in %g seconds."), static_cast<double>(duration));

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Seconds: %g"), duration));
}


// ******************************** Функции шумов ******************************** //
inline float FlatNoise(const FVector2D& Coord, const FNoiseInfo&)
{
	return 0;
}

inline float WhiteNoise(const FVector2D& Coord, const FNoiseInfo&)
{
	return FMath::RandRange(-1.f, 1.f);
}

inline float PerlinNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo)
{
	float Xfl = FMath::FloorToFloat((float)Coord.X);
	float Yfl = FMath::FloorToFloat((float)Coord.Y);
	int32 Xi = (int32)(Xfl) & 255;
	int32 Yi = (int32)(Yfl) & 255;
	float X = (float)Coord.X - Xfl;
	float Y = (float)Coord.Y - Yfl;
	float Xm1 = X - 1.0f;
	float Ym1 = Y - 1.0f;

	const int32* P = NoisePerm;
	int32 AA = P[Xi] + Yi;
	int32 AB = AA + 1;
	int32 BA = P[Xi + 1] + Yi;
	int32 BB = BA + 1;

	float U = SmoothCurve(X);
	float V = SmoothCurve(Y);

	return FMath::Lerp(
		FMath::Lerp(PGrad2(P[AA], X, Y), PGrad2(P[BA], Xm1, Y), U),
		FMath::Lerp(PGrad2(P[AB], X, Ym1), PGrad2(P[BB], Xm1, Ym1), U),
		V);
}

inline float PerlinOctavesNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo)
{
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float noiseValue = 0.0f;

	for (int32 i = 0; i < LandscapeInfo.Octaves; i++)
	{
		noiseValue += PerlinNoise(Coord * frequency, LandscapeInfo) * amplitude;

		frequency *= LandscapeInfo.Lacunarity;
		amplitude *= LandscapeInfo.Persistence;
	}

	return noiseValue;
}

inline float SimplexNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo)
{
	// Noise contributions from the three corners
	float n0, n1, n2;

	// Skew the input space to determine which simplex cell we're in
	constexpr float F2 = 0.36602540378f; // 0.5 * (sqrt(3.0) - 1.0)
	float s = (Coord.X + Coord.Y) * F2; // Hairy factor for 2D
	int32 i = FMath::FloorToFloat(Coord.X + s);
	int32 j = FMath::FloorToFloat(Coord.Y + s);

	constexpr float G2 = 0.2113248654f;  // (3.0 - sqrt(3.0)) / 6.0
	float t = (i + j) * G2;
	float X0 = i - t; // Unskew the cell origin back to (x,y) space
	float Y0 = j - t;
	double x0 = Coord.X - X0; // The x,y distances from the cell origin   
	double y0 = Coord.Y - Y0;

	// For the 2D case, the simplex shape is an equilateral triangle.   
	// Determine which simplex we are in.   
	int32 i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords   
	if (x0 > y0) { i1 = 1; j1 = 0; } // lower triangle, XY order: (0,0)->(1,0)->(1,1)   
	else { i1 = 0; j1 = 1; }      // upper triangle, YX order: (0,0)->(0,1)->(1,1)

	// A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and    
	// a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where    
	// c = (3-sqrt(3))/6

	float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords    
	float y1 = y0 - j1 + G2;
	float x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords    
	float y2 = y0 - 1.0 + 2.0 * G2;

	// Work out the hashed gradient indices of the three simplex corners  
	const int32* NP = NoisePerm;
	int32 ii = i & 255;
	int32 jj = j & 255;
	int32 gi0 = NP[ii + NP[jj]] % 8;
	int32 gi1 = NP[ii + i1 + NP[jj + j1]] % 8;
	int32 gi2 = NP[ii + 1 + NP[jj + 1]] % 8;

	// Calculate the contribution from the three corners    
	double t0 = 0.5 - x0 * x0 - y0 * y0;
	if (t0 < 0) n0 = 0.0f;
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * FVector2D::DotProduct(Grad2[gi0], FVector2D(x0, y0));  // (x,y) of grad3 used for 2D gradient    
	}
	double t1 = 0.5 - x1 * x1 - y1 * y1;
	if (t1 < 0) n1 = 0.0f;
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * FVector2D::DotProduct(Grad2[gi1], FVector2D(x1, y1));
	}
	double t2 = 0.5 - x2 * x2 - y2 * y2;
	if (t2 < 0) n2 = 0.0f;
	else
	{
		t2 *= t2;
		n2 = t2 * t2 * FVector2D::DotProduct(Grad2[gi2], FVector2D(x2, y2));
	}
	// Add contributions from each corner to get the final noise value.    
	// The result is scaled to return values in the interval [-1,1].    
	return 70.0 * (n0 + n1 + n2);
}

float ridge(float h, float offset) {
	h = FMath::Abs(h); // create creases
	h = offset - h; // invert so creases are at top
	h = h * h; // sharpen creases
	return h;
}

inline float SimplexOctavesNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo)
{
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float noiseValue = 0.0f;

	for (int32 i = 0; i < LandscapeInfo.Octaves; i++)
	{
		noiseValue += FMath::Abs(SimplexNoise(Coord * frequency, LandscapeInfo)) * amplitude;

		frequency *= LandscapeInfo.Lacunarity;
		amplitude *= LandscapeInfo.Persistence;
	}

	return noiseValue;
	//float offset = 0.9;

	//float noiseValue = 0.0;
	//float frequency = 1.0, amplitude = 0.5;
	//float prev = 1.0;
	//for (int i = 0; i < LandscapeInfo.Octaves; i++) {
	//	float n = ridge(SimplexNoise(Coord * frequency, LandscapeInfo), offset);
	//	noiseValue += n * amplitude;
	//	noiseValue += n * amplitude * prev;  // scale by previous octave
	//	prev = n;
	//	frequency *= LandscapeInfo.Lacunarity;
	//	amplitude *= LandscapeInfo.Persistence;
	//}
	//return noiseValue;

}

FORCEINLINE float SinCosNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo)
{
	return FMath::Sin(Coord.X) + FMath::Cos(Coord.Y);
}

inline float ExperimentalNoise(const FVector2D& Coord, const FNoiseInfo& LandscapeInfo)
{

	return 0;
}
// ******************************************************************************* //

FORCEINLINE float PGrad2(int32 Hash, float X, float Y)
{
	switch (Hash & 7)
	{
	case 0: return X;
	case 1: return X + Y;
	case 2: return Y;
	case 3: return -X + Y;
	case 4: return -X;
	case 5: return -X - Y;
	case 6: return -Y;
	case 7: return X - Y;
		// can't happen
	default: return 0;
	}
}

FORCEINLINE float SmoothCurve(float X)
{
	return X * X * X * (X * (X * 6.0f - 15.0f) + 10.0f); // 6x^5-15x^4+10x^3
	//return X * X * (3.0f - 2.0f * X); // 3x^2 - 2x^3
	//return (1 - FMath::Cos(X * PI)) / 2; // (1 - cos(x*pi)) / 2
}