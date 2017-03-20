// filteression - public domain
// authored on 2017 by Aras Pranckevicius
// no warranty implied; use at your own risk

#include "filteression.h"

using namespace filteression;

#define _FLTESS_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))


struct StreamDesc
{
    int bitStart;
    int bitCount;
};
static const StreamDesc kStreamsBC1[] =
{
    /*
    {0,5}, // color endpoint 0 blue
    {16,5}, // color endpoint 1 blue
    {11,5}, // color endpoint 0 red
    {27,5}, // color endpoint 1 red
    {5,6}, // color endpoint 0 green
    {21,6}, // color endpoint 1 green
     */
    {0,32}, // endpoints
    {32,32}, // selector bits
};

static const StreamDesc kStreamsBC3[] =
{
    {0,32}, // alpha endpoints
    {64,32}, // color endpoints
    {32,32}, // alpha selector bits
    {96,32}, // color selector bits
};

static bool ArgsCheck(const void* data, size_t byteSize, int width, int height, void* outData)
{
    if (!data || byteSize == 0 || width <= 0 || height <= 0 || !outData)
        return false;
    return true;
}


static bool GetFormatDesc(TextureFormat format, int& outWidth, int& outHeight, int& outBlockBytes, const StreamDesc*& outStreams, size_t& outStreamCount)
{
    switch(format)
    {
        case kTextureFormatBC1:
            outWidth = outHeight = 4;
            outBlockBytes = 8;
            outStreams = kStreamsBC1; outStreamCount = _FLTESS_ARRAY_SIZE(kStreamsBC1);
            return true;
        case kTextureFormatBC3:
            outWidth = outHeight = 4;
            outBlockBytes = 16;
            outStreams = kStreamsBC3; outStreamCount = _FLTESS_ARRAY_SIZE(kStreamsBC3);
            return true;
        default:
            outWidth = outHeight = outBlockBytes = 1;
            outStreams = NULL;
            outStreamCount = 0;
            return false;
    }
}


//@TODO: optimize!
static inline bool TestBit(const uint8_t* bits, size_t index)
{
    return (bits[index / 8] & (1 << (index & 7))) != 0;
}

inline void SetBit(uint8_t* bits, size_t index, bool value)
{
    if (value)
        bits[index / 8] |= 1 << (index & 7);
    else
        bits[index / 8] &= ~(1 << (index & 7));
}


bool filteression::Encode(const void* data, size_t byteSize, int width, int height, TextureFormat format, void* outData)
{
    if (!ArgsCheck(data, byteSize, width, height, outData))
        return false;
    int blockWidth, blockHeight, blockBytes;
    const StreamDesc* streams;
    size_t streamCount;
    if (!GetFormatDesc(format, blockWidth, blockHeight, blockBytes, streams, streamCount))
        return false;

    //@TODO: check if data size is enough

    const int blocksX = (width+blockWidth-1) / blockWidth;
    const int blocksY = (height+blockHeight-1) / blockHeight;
    const int blockCount = blocksX * blocksY;
    const uint8_t* srcPtr = (const uint8_t*)data;
    uint8_t* dstPtr = (uint8_t*)outData;
    size_t dstBit = 0;
    for (size_t is = 0; is < streamCount; ++is)
    {
        const uint8_t* src = srcPtr;
        for (size_t iblock = 0; iblock < blockCount; ++iblock)
        {
            for (size_t ibit = 0; ibit < streams[is].bitCount; ++ibit)
            {
                bool v = TestBit(src, streams[is].bitStart + ibit);
                SetBit(dstPtr, dstBit, v);
                dstBit++;
            }
            src += blockBytes;
        }
    }
    
    return true;
}


bool filteression::Decode(const void* data, size_t byteSize, int width, int height, TextureFormat format, void* outData)
{
    if (!ArgsCheck(data, byteSize, width, height, outData))
        return false;
    int blockWidth, blockHeight, blockBytes;
    const StreamDesc* streams;
    size_t streamCount;
    if (!GetFormatDesc(format, blockWidth, blockHeight, blockBytes, streams, streamCount))
        return false;

    //@TODO: check if data size is enough

    const int blocksX = (width+blockWidth-1) / blockWidth;
    const int blocksY = (height+blockHeight-1) / blockHeight;
    const int blockCount = blocksX * blocksY;
    const uint8_t* srcPtr = (const uint8_t*)data;
    uint8_t* dstPtr = (uint8_t*)outData;
    size_t srcBit = 0;
    for (size_t is = 0; is < streamCount; ++is)
    {
        uint8_t* dst = dstPtr;
        for (size_t iblock = 0; iblock < blockCount; ++iblock)
        {
            for (size_t ibit = 0; ibit < streams[is].bitCount; ++ibit)
            {
                bool v = TestBit(srcPtr, srcBit);
                SetBit(dst, streams[is].bitStart + ibit, v);
                srcBit++;
            }
            dst += blockBytes;
        }
    }
    
    return true;
}
