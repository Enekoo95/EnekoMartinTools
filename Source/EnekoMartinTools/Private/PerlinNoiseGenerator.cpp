// Fill out your copyright notice in the Description page of Project Settings.


#include "PerlinNoiseGenerator.h"

float UPerlinNoiseGenerator::GetPerlinValue(int32 X, int32 Y, float Scale, int32 Seed)
{
	// The seed offsets the sample point: same seed → same texture
	const FVector2D P(X * Scale + Seed * 31.7f, Y * Scale + Seed * 17.3f);
	// PerlinNoise2D returns [-1, 1]; remap it to [0, 1]
	return FMath::Clamp((FMath::PerlinNoise2D(P) + 1.f) * 0.5f, 0.f, 1.f);
}

UTexture2D* UPerlinNoiseGenerator::GeneratePerlinTexture(int32 Width, int32 Height, float Scale, int32 Seed)
{
	Width = FMath::Clamp(Width, 8, 4096); 
	Height = FMath::Clamp(Height, 8, 4096);

	UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Tex) { return nullptr; }

	Tex->SRGB = false;                 
	Tex->Filter = TF_Bilinear;
	Tex->AddressX = TA_Wrap;           
	Tex->AddressY = TA_Wrap;

	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	FColor* Pixels = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Y = 0; Y < Height; ++Y)
		for (int32 X = 0; X < Width; ++X)
		{
			const uint8 V = (uint8)FMath::RoundToInt(GetPerlinValue(X, Y, Scale, Seed) * 255.f);
			Pixels[Y * Width + X] = FColor(V, V, V, 255);
		}
	Mip.BulkData.Unlock();
	Tex->UpdateResource(); 
	return Tex;
}