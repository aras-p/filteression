// filteression - public domain
// authored on 2017 by Aras Pranckevicius
// no warranty implied; use at your own risk

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace filteression
{
	enum TextureFormat
	{
        kTextureFormatBC1 = 0, // aka DXT1
        kTextureFormatBC3, // aka DXT5
        kTextureFormatASTC_6x6,
        kTextureFormatCount
	};


	bool Encode(const void* data, size_t byteSize, int width, int height, TextureFormat format, void* outData);
    bool Decode(const void* data, size_t byteSize, int width, int height, TextureFormat format, void* outData);

} // namespace filteression
