// smol-v - tests code - public domain - https://github.com/aras-p/smol-v
// authored on 2016 by Aras Pranckevicius
// no warranty implied; use at your own risk

#define _CRT_SECURE_NO_WARNINGS // yes MSVC, I want to use fopen
#include "../source/filteression.h"
#include "external/lz4/lz4.h"
#include "external/lz4/lz4hc.h"
#include "external/miniz/miniz.h"
#include "external/zstd/zstd.h"
#include <stdio.h>
#include <map>
#include <string>
#include <vector>
#include <regex>


typedef std::vector<uint8_t> ByteArray;


static void ReadFile(const char* fileName, ByteArray& output)
{
	FILE* f = fopen(fileName, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		size_t pos = output.size();
		output.resize(pos + size);
		fread(output.data() + pos, size, 1, f);
		fclose(f);
	}
}

static size_t CompressLZ4(const void* data, size_t size)
{
    if (size == 0)
        return 0;
    int bufferSize = LZ4_compressBound((int)size);
    char* buffer = new char[bufferSize];
    size_t resSize = LZ4_compress_default ((const char*)data, buffer, (int)size, bufferSize);
    delete[] buffer;
    return resSize;
}

static size_t CompressLZ4HC(const void* data, size_t size, int level = 0)
{
	if (size == 0)
		return 0;
	int bufferSize = LZ4_compressBound((int)size);
	char* buffer = new char[bufferSize];
	size_t resSize = LZ4_compress_HC ((const char*)data, buffer, (int)size, bufferSize, level);
	delete[] buffer;
	return resSize;
}

static size_t CompressZstd(const void* data, size_t size, int level = 0)
{
	if (size == 0)
		return 0;
	size_t bufferSize = ZSTD_compressBound(size);
	char* buffer = new char[bufferSize];
	size_t resSize = ZSTD_compress(buffer, bufferSize, data, size, level);
	delete[] buffer;
	return resSize;
}

static size_t CompressMiniz(const void* data, size_t size, int level = MZ_DEFAULT_LEVEL)
{
	if (size == 0)
		return 0;
	size_t bufferSize = mz_compressBound(size);
	unsigned char* buffer = new unsigned char[bufferSize];
	size_t resSize = bufferSize;
	int res = mz_compress2(buffer, &resSize, (const unsigned char*)data, size, level);
	delete[] buffer;
	if (res != MZ_OK)
		return -1;
	return resSize;
}


int main()
{
	#define TEST_BC1 1
    #define TEST_BC3 1

	// files we're testing on
	const char* kFiles[] =
	{
		#if TEST_BC1
		"dxt1/1k_HouseDiffuse-1024x1024.bin",
        "dxt1/ArchesDiffuse-1024x1024.bin",
        "dxt1/ColorGrid-128x128.bin",
        "dxt1/ContrastEnhanced3D16-256x16.bin",
        "dxt1/galacticcenter_greatobs_big-2048x1024.bin",
        "dxt1/HellephantDiffuse-1024x1024.bin",
        "dxt1/mainsquare_shadows_bake1-1024x1024.bin",
        "dxt1/Marble Tile NM-512x512.bin",
        "dxt1/Marble Tile-512x512.bin",
        //"dxt1/NoiseEffectGrain-256x256.bin",
        //"dxt1/NoiseEffectScratch-512x512.bin",
		#endif // #if TEST_BC1

        #if TEST_BC3
        //"dxt5/2K_bomb-2048x2048.bin",
        //"dxt5/ArchesAO-1024x1024.bin",
        //"dxt5/ArchesNormals-1024x1024.bin",
        "dxt5/Big Checkmark-64x64.bin",
        "dxt5/Explosion-512x512.bin",
        //"dxt5/fire-512x256.bin",
        //"dxt5/HellephantEmissive-1024x1024.bin",
        "dxt5/HellephantNormals-1024x1024.bin",
        "dxt5/HellephantOcclusion-1024x1024.bin",
        //"dxt5/icon-2048x2048.bin",
        "dxt5/SpaceCute-Girl1-256x256.bin",
        "dxt5/tile_grate_tech_dff-1024x512.bin",
        "dxt5/tile_paint_orange_dff-1024x1024.bin",
        "dxt5/vehicle_rcFlyer_dff-1024x1024.bin",
        "dxt5/wall_deco1_diff_layers-512x512.bin",
        #endif // #if TEST_BC3
	};

    typedef std::pair<size_t, size_t> SizesRawFiltered;
    typedef std::map<std::string, SizesRawFiltered> CompressorSizeMap;
    typedef std::map<std::string, CompressorSizeMap> FormatSizeMap;
    FormatSizeMap sizes;
    std::map<std::string, size_t> totalSize;

	// go over all test files
    std::regex namePattern("(.*)/.*-(\\d+)x(\\d+)\\.bin");
	int errorCount = 0;
	for (size_t i = 0; i < sizeof(kFiles)/sizeof(kFiles[0]); ++i)
	{
		// Read
		printf("Reading %s\n", kFiles[i]);
		ByteArray textureData;
		ReadFile((std::string("tests/bits/") + kFiles[i]).c_str(), textureData);
		if (textureData.empty())
		{
			printf("ERROR: failed to read %s\n", kFiles[i]);
			++errorCount;
			break;
		}
        std::cmatch match;
        if (!std::regex_match(kFiles[i], match, namePattern))
        {
            printf("ERROR: filename does not match expected pattern\n");
            ++errorCount;
            break;
        }
        std::string formatStr = match.str(1);
        std::string widthStr = match.str(2);
        std::string heightStr = match.str(3);
        filteression::TextureFormat format = filteression::kTextureFormatCount;
        if (formatStr == "dxt1")
            format = filteression::kTextureFormatBC1;
        else if (formatStr == "dxt5")
            format = filteression::kTextureFormatBC3;
        else
        {
            printf("ERROR: unknown format %s\n", formatStr.c_str());
            ++errorCount;
            break;
        }
        int width = atoi(widthStr.c_str());
        int height = atoi(heightStr.c_str());

        // Encode
        ByteArray encoded;
        encoded.resize(textureData.size(), 0xCD);
        if (!filteression::Encode(textureData.data(), textureData.size(), width, height, format, encoded.data()))
        {
            printf("ERROR: failed to encode\n");
            ++errorCount;
            break;
        }

        // Decode
        ByteArray decoded;
        decoded.resize(textureData.size(), 0xCD);
        if (!filteression::Decode(encoded.data(), encoded.size(), width, height, format, decoded.data()))
        {
            printf("ERROR: failed to decode\n");
            ++errorCount;
            break;
        }

		// Check that it decoded 100% the same
		if (textureData != decoded)
		{
			printf("ERROR: did not encode+decode properly (bug?) %s\n", kFiles[i]);
			const uint8_t* texPtr = textureData.data();
			const uint8_t* texPtrEnd = texPtr + textureData.size();
			const uint8_t* texDecodedPtr = decoded.data();
			const uint8_t* texDecodedEnd = texDecodedPtr + decoded.size();
			int idx = 0;
            int printedCount = 0;
			while (texPtr < texPtrEnd && texDecodedPtr < texDecodedEnd && printedCount < 20)
			{
				if (*texPtr != *texDecodedPtr)
				{
					printf("  byte #%04i %02x -> %02x\n", idx, *texPtr, *texPtrEnd);
                    ++printedCount;
				}
				++texPtr;
				++texDecodedPtr;
				++idx;
			}			
			++errorCount;
			break;
		}

        totalSize[formatStr] += textureData.size();
        CompressorSizeMap& sizeMap = sizes[formatStr];
        sizeMap["1   zlib"].first += CompressMiniz(textureData.data(), textureData.size());
        sizeMap["2    lz4"].first += CompressLZ4(textureData.data(), textureData.size());
        sizeMap["3  lz4hc"].first += CompressLZ4HC(textureData.data(), textureData.size());
        sizeMap["4   zstd"].first += CompressZstd(textureData.data(), textureData.size());
        sizeMap["5 zstd20"].first += CompressZstd(textureData.data(), textureData.size(), 20);

        sizeMap["1   zlib"].second += CompressMiniz(encoded.data(), encoded.size());
        sizeMap["2    lz4"].second += CompressLZ4(encoded.data(), encoded.size());
        sizeMap["3  lz4hc"].second += CompressLZ4HC(encoded.data(), encoded.size());
        sizeMap["4   zstd"].second += CompressZstd(encoded.data(), encoded.size());
        sizeMap["5 zstd20"].second += CompressZstd(encoded.data(), encoded.size(), 20);
	}
	
	if (errorCount != 0)
	{
		printf("Got ERRORS: %i\n", errorCount);
		return 1;
	}
	
	// Compress various ways (as a whole blob) and print sizes
    for (FormatSizeMap::const_iterator fit = sizes.begin(), fitEnd = sizes.end(); fit != fitEnd; ++fit)
    {
        size_t size = totalSize[fit->first];
        printf("\n%s: original size %6.1fKB\n", fit->first.c_str(), size/1024.0f);
        const CompressorSizeMap& sizeMap = sizes[fit->first];
        for (CompressorSizeMap::const_iterator it = sizeMap.begin(), itEnd = sizeMap.end(); it != itEnd; ++it)
        {
            printf("%-13s %6.1fKB %5.1f%%  %6.1fKB %5.1f%%\n",
                   it->first.c_str(),
                   it->second.first/1024.0f, (float)it->second.first/(float)(size)*100.0f,
                   it->second.second/1024.0f, (float)it->second.second/(float)(size)*100.0f);
        }
    }

	return 0;
}
