// filteression - public domain
// authored on 2017 by Aras Pranckevicius
// no warranty implied; use at your own risk

#include "filteression.h"

using namespace filteression;

#define _FLTESS_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

static uint32_t fltess_ZigEncode(int32_t i, int bits)
{
    uint32_t bitmask = (uint32_t)((1LL << bits)-1);
    uint32_t u = uint32_t(i) & bitmask;
    uint32_t mask = u & (1<<(bits-1)) ? bitmask : 0;
    uint32_t res = (u << 1) ^ mask;
    return res;
}

static int32_t fltess_ZigDecode(uint32_t u, int bits)
{
    uint32_t bitmask = (uint32_t)((1LL << bits)-1);
    u &= bitmask;
    int32_t v = (u & 1) ? ((u >> 1) ^ ~0) : (u >> 1);
    return v;
}


struct StreamDesc
{
    int bitStart;
    int bitCount;
    bool delta;
};
static const StreamDesc kStreamsBC1[] =
{
    /*
    {0,5,true}, // color endpoint 0 blue
    {16,5,true}, // color endpoint 1 blue
    {11,5,true}, // color endpoint 0 red
    {27,5,true}, // color endpoint 1 red
    {5,6,true}, // color endpoint 0 green
    {21,6,true}, // color endpoint 1 green
     */
    //{0,16,true}, // endpoint 0
    //{16,16,true}, // endpoint 1
    {0,32,false}, // endpoints
    {32,32,false}, // selector bits
};

static const StreamDesc kStreamsBC3[] =
{
    /*
    {0,16,true}, // alpha endpoint 0
    {16,16,true}, // alpha endpoint 1
    {64,16,true}, // color endpoint 0
    {80,16,true}, // color endpoint 1
     */
    {0,32,false}, // alpha endpoints
    {64,32,false}, // color endpoints
    {32,32,false}, // alpha selector bits
    {96,32,false}, // color selector bits
};

static const StreamDesc kStreamsASTC[] =
{
    {0,5,true},
    {5,4,true},
    {9,2,true},
    //{11,6,false}, // partition count + CEM for some cases
    //{17,15,false},
    {11,21,false},
    {32,32,false},
    {64,32,false},
    {96,32,false},
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
        case kTextureFormatASTC_6x6:
            outWidth = outHeight = 6;
            outBlockBytes = 16;
            outStreams = kStreamsASTC; outStreamCount = _FLTESS_ARRAY_SIZE(kStreamsASTC);
            return true;
        default:
            outWidth = outHeight = outBlockBytes = 1;
            outStreams = NULL;
            outStreamCount = 0;
            return false;
    }
}


static inline uint32_t GetBits(const uint8_t* bits, size_t index, uint32_t bitCountMask)
{
    bits += index/8;
    uint32_t word = *(const uint32_t*)bits; //@TODO: unaligned read
    word >>= index & 7;
    word &= bitCountMask;
    return word;
}

static inline void SetBits(uint8_t* bits, size_t index, uint32_t bitCountMask, uint32_t value)
{
    bits += index/8;
    uint32_t word = *(const uint32_t*)bits; //@TODO: unaligned read
    word &= ~(bitCountMask << (index & 7));
    word |= value << (index & 7);
    *(uint32_t*)bits = word; //@TODO: unaligned write
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
    const size_t rowStride = blocksX * blockBytes;
    const uint8_t* srcPtr = (const uint8_t*)data;
    const uint8_t* srcPrevRow = srcPtr - rowStride;
    uint8_t* dstPtr = (uint8_t*)outData;
    size_t dstBit = 0;
    for (size_t is = 0; is < streamCount; ++is)
    {
        const uint8_t* src = srcPtr;
        const uint8_t* srcPrev = srcPrevRow;
        const uint32_t streamBitMask = (uint32_t)((1ULL<<streams[is].bitCount)-1);
        uint32_t prevBits = 0;
        for (size_t iblock = 0; iblock < blockCount; ++iblock)
        {
            uint32_t bits = GetBits(src, streams[is].bitStart, streamBitMask);
            uint32_t topBits = iblock >= blocksX ? GetBits(srcPrev, streams[is].bitStart, streamBitMask) : prevBits;
            uint32_t avgBits = (prevBits + topBits) / 2;
            int32_t delta = bits - avgBits;
            uint32_t deltaBits = fltess_ZigEncode(delta, streams[is].bitCount) & streamBitMask;
            prevBits = bits;
            SetBits(dstPtr, dstBit, streamBitMask, streams[is].delta ? deltaBits : bits);
            dstBit += streams[is].bitCount;
            src += blockBytes;
            srcPrev += blockBytes;
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
    const size_t rowStride = blocksX * blockBytes;
    const uint8_t* srcPtr = (const uint8_t*)data;
    uint8_t* dstPtr = (uint8_t*)outData;
    size_t srcBit = 0;
    for (size_t is = 0; is < streamCount; ++is)
    {
        uint8_t* dst = dstPtr;
        const uint32_t streamBitMask = (uint32_t)((1ULL<<streams[is].bitCount)-1);
        uint32_t prevBits = 0;
        for (size_t iblock = 0; iblock < blockCount; ++iblock)
        {
            uint32_t deltaBits = GetBits(srcPtr, srcBit, streamBitMask);
            uint32_t bits;
            if (streams[is].delta)
            {
                int32_t delta = fltess_ZigDecode(deltaBits, streams[is].bitCount);
                uint32_t topBits = iblock >= blocksX ? GetBits(dst-rowStride, streams[is].bitStart, streamBitMask) : prevBits;
                uint32_t avgBits = (prevBits + topBits) / 2;
                bits = (avgBits + delta) & streamBitMask;
            }
            else
            {
                bits = deltaBits;
            }
            prevBits = bits;
            SetBits(dst, streams[is].bitStart, streamBitMask, bits);
            srcBit += streams[is].bitCount;
            dst += blockBytes;
        }
    }
    
    return true;
}
