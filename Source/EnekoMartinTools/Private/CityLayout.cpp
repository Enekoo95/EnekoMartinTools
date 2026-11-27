#include "CityLayout.h"

namespace
{
	constexpr float HeightNoiseCell = 500.f;
	constexpr int32 MaxLotsPerAxis = 6;

	float Perlin01(const FCityLayoutParams& P, float X, float Y, float Scale)
	{
		const float Offset = (P.Seed % 1000) * 17.31f;
		const FVector2D Q((X + 0.37f) * Scale + Offset, (Y + 0.37f) * Scale + Offset * 0.73f);
		return FMath::Clamp((FMath::PerlinNoise2D(Q) + 1.f) * 0.5f, 0.f, 1.f);
	}

	float NoiseNorm(const FCityLayoutParams& P, float X, float Y, float Scale)
	{
		return FMath::Clamp((Perlin01(P, X, Y, Scale) - 0.25f) / 0.5f, 0.f, 1.f);
	}

	float BlockExtent(const FCityLayoutParams& P, int32 Index, int32 Axis)
	{
		float N;
		if (P.bUsePerlinNoise)
		{
			N = NoiseNorm(P, Index, Axis * 37.f, P.BlockSizeNoiseScale);
		}
		else
		{
			const int32 Hash = static_cast<int32>(HashCombine(GetTypeHash(P.Seed), GetTypeHash(Index * 2 + Axis)));
			N = FRandomStream(Hash).FRand();
		}
		return FMath::Lerp(P.MinBlockSize, P.MaxBlockSize, N);
	}

	bool IsPark(const FCityLayoutParams& P, int32 X, int32 Y)
	{
		if (P.bUsePerlinNoise)
		{
			if (P.ParkThreshold >= 1.f)
			{
				return true;
			}
			return NoiseNorm(P, X, Y, P.ParkNoiseScale) < P.ParkThreshold;
		}
		const int32 Hash = static_cast<int32>(HashCombine(GetTypeHash(P.Seed), GetTypeHash(X * 1000 + Y)));
		return FRandomStream(Hash).FRand() < P.ParkChance;
	}

	FCityLayoutParams Sanitized(FCityLayoutParams P)
	{
		P.BlocksX = FMath::Clamp(P.BlocksX, 1, 8);
		P.BlocksY = FMath::Clamp(P.BlocksY, 1, 8);
		P.StreetWidth = FMath::Max(P.StreetWidth, 200.f);
		P.TargetLotSize = FMath::Max(P.TargetLotSize, 200.f);
		P.MinBlockSize = FMath::Max(P.MinBlockSize, 200.f);
		P.MaxBlockSize = FMath::Max(P.MaxBlockSize, P.MinBlockSize);
		P.MinFootprint = FMath::Clamp(P.MinFootprint, 0.2f, 0.9f);
		P.MaxFootprint = FMath::Clamp(P.MaxFootprint, P.MinFootprint, 0.9f);
		P.MinBuildingHeight = FMath::Max(P.MinBuildingHeight, 100.f);
		P.MaxBuildingHeight = FMath::Max(P.MaxBuildingHeight, P.MinBuildingHeight);
		P.SurfaceHeight = FMath::Max(P.SurfaceHeight, 0.f);
		P.BlockSizeNoiseScale = FMath::Clamp(P.BlockSizeNoiseScale, 0.01f, 2.f);
		P.ParkNoiseScale = FMath::Clamp(P.ParkNoiseScale, 0.01f, 1.f);
		P.HeightNoiseScale = FMath::Clamp(P.HeightNoiseScale, 0.01f, 1.f);
		P.ParkThreshold = FMath::Clamp(P.ParkThreshold, 0.f, 1.f);
		P.ParkChance = FMath::Clamp(P.ParkChance, 0.f, 1.f);
		return P;
	}
}

FCityLayout FCityLayoutBuilder::Build(const FCityLayoutParams& InParams)
{
	const FCityLayoutParams P = Sanitized(InParams);
	FCityLayout Out;

	TArray<float> ColSize, RowSize, ColStart, RowStart;
	ColSize.SetNum(P.BlocksX);
	ColStart.SetNum(P.BlocksX);
	RowSize.SetNum(P.BlocksY);
	RowStart.SetNum(P.BlocksY);

	float SumX = 0.f, SumY = 0.f;
	for (int32 X = 0; X < P.BlocksX; ++X) { ColSize[X] = BlockExtent(P, X, 0); SumX += ColSize[X]; }
	for (int32 Y = 0; Y < P.BlocksY; ++Y) { RowSize[Y] = BlockExtent(P, Y, 1); SumY += RowSize[Y]; }

	Out.TotalSize.X = SumX + P.StreetWidth * (P.BlocksX + 1);
	Out.TotalSize.Y = SumY + P.StreetWidth * (P.BlocksY + 1);

	float Pos = -Out.TotalSize.X * 0.5f + P.StreetWidth;
	for (int32 X = 0; X < P.BlocksX; ++X) { ColStart[X] = Pos; Pos += ColSize[X] + P.StreetWidth; }
	Pos = -Out.TotalSize.Y * 0.5f + P.StreetWidth;
	for (int32 Y = 0; Y < P.BlocksY; ++Y) { RowStart[Y] = Pos; Pos += RowSize[Y] + P.StreetWidth; }

	// ---- Calles: verticales completas; horizontales solo entre manzanas (sin solapes en los cruces) ----
	Pos = -Out.TotalSize.X * 0.5f;
	for (int32 I = 0; I <= P.BlocksX; ++I)
	{
		FCityRect& R = Out.Roads.AddDefaulted_GetRef();
		R.Center = FVector2D(Pos + P.StreetWidth * 0.5f, 0.f);
		R.Size = FVector2D(P.StreetWidth, Out.TotalSize.Y);
		Pos += P.StreetWidth;
		if (I < P.BlocksX) { Pos += ColSize[I]; }
	}
	Pos = -Out.TotalSize.Y * 0.5f;
	for (int32 J = 0; J <= P.BlocksY; ++J)
	{
		const float CenterY = Pos + P.StreetWidth * 0.5f;
		for (int32 X = 0; X < P.BlocksX; ++X)
		{
			FCityRect& R = Out.Roads.AddDefaulted_GetRef();
			R.Center = FVector2D(ColStart[X] + ColSize[X] * 0.5f, CenterY);
			R.Size = FVector2D(ColSize[X], P.StreetWidth);
		}
		Pos += P.StreetWidth;
		if (J < P.BlocksY) { Pos += RowSize[J]; }
	}

	// ---- Manzanas y parcelas ----
	for (int32 BX = 0; BX < P.BlocksX; ++BX)
	{
		for (int32 BY = 0; BY < P.BlocksY; ++BY)
		{
			FCityBlock& Block = Out.Blocks.AddDefaulted_GetRef();
			Block.X = BX;
			Block.Y = BY;
			Block.Center = FVector2D(ColStart[BX] + ColSize[BX] * 0.5f, RowStart[BY] + RowSize[BY] * 0.5f);
			Block.Size = FVector2D(ColSize[BX], RowSize[BY]);
			Block.bPark = IsPark(P, BX, BY);
			if (Block.bPark)
			{
				continue;
			}

			const FVector2D Center = Block.Center;
			const FVector2D Size = Block.Size;
			const int32 LotsX = FMath::Clamp(FMath::RoundToInt(Size.X / P.TargetLotSize), 1, MaxLotsPerAxis);
			const int32 LotsY = FMath::Clamp(FMath::RoundToInt(Size.Y / P.TargetLotSize), 1, MaxLotsPerAxis);
			const float LotW = Size.X / LotsX;
			const float LotD = Size.Y / LotsY;
			const uint32 BlockHash = HashCombine(GetTypeHash(P.Seed), GetTypeHash(BX * 100 + BY));

			for (int32 I = 0; I < LotsX; ++I)
			{
				for (int32 J = 0; J < LotsY; ++J)
				{
					const float OffsetX = (I + 0.5f) * LotW - Size.X * 0.5f;
					const float OffsetY = (J + 0.5f) * LotD - Size.Y * 0.5f;

					float Yaw;
					bool bFaceX;
					if (FMath::Abs(OffsetX) / Size.X >= FMath::Abs(OffsetY) / Size.Y)
					{
						Yaw = (OffsetX >= 0.f) ? 0.f : 180.f;
						bFaceX = true;
					}
					else
					{
						Yaw = (OffsetY >= 0.f) ? 90.f : -90.f;
						bFaceX = false;
					}

					FRandomStream LotRng(static_cast<int32>(HashCombine(BlockHash, GetTypeHash(I * 10 + J))));
					const float DepthLot = bFaceX ? LotW : LotD;
					const float WidthLot = bFaceX ? LotD : LotW;

					FCityLot& Lot = Out.Lots.AddDefaulted_GetRef();
					Lot.Depth = DepthLot * LotRng.FRandRange(P.MinFootprint, P.MaxFootprint);
					Lot.Width = WidthLot * LotRng.FRandRange(P.MinFootprint, P.MaxFootprint);

					float HeightAlpha;
					if (P.bUsePerlinNoise)
					{
						HeightAlpha = NoiseNorm(P, (Center.X + OffsetX) / HeightNoiseCell,
							(Center.Y + OffsetY) / HeightNoiseCell, P.HeightNoiseScale);
					}
					else
					{
						HeightAlpha = LotRng.FRand();
					}
					Lot.Height = FMath::Lerp(P.MinBuildingHeight, P.MaxBuildingHeight, HeightAlpha);
					Lot.Yaw = Yaw;
					Lot.Location = FVector(Center.X + OffsetX, Center.Y + OffsetY, P.SurfaceHeight);
					Lot.BlockX = BX;
					Lot.BlockY = BY;
				}
			}
		}
	}

	return Out;
}
