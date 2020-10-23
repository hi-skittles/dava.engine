#include "Bitstream.h"

namespace DAVA
{
Bitstream::Bitstream(void* bitstreamPointer, uint32 bitstreamLength)
{
    head = reinterpret_cast<uint32*>(bitstreamPointer);
    tail = reinterpret_cast<uint32*>(bitstreamPointer);

    uint32 tmBuffer = *head;
    bufferA = tmBuffer;

    tmBuffer = *(head + 1);
    bufferB = tmBuffer;

    length = bitstreamLength;
    position = 0;
    writeBuffer = 0;
}

Bitstream::~Bitstream()
{
}

void Bitstream::Reset()
{
    tail = head;

    uint32 tmBuffer = *head;
    bufferA = tmBuffer;

    tmBuffer = *(head + 1);
    bufferB = tmBuffer;

    position = 0;
    writeBuffer = 0;
}

uint32 Bitstream::GetPosition()
{
    return (static_cast<uint32>(tail - head) * 32 + position);
}

uint32 Bitstream::Length()
{
    uint32 tmByteCount = static_cast<uint32>(reinterpret_cast<uint8*>(tail) - reinterpret_cast<uint8*>(head));

    if (position)
    {
        *tail = writeBuffer;
        tmByteCount += 4 * (position + 31) / 32;
    }
    return tmByteCount;
}

void Bitstream::Skip(uint32 bitCount)
{
    position += bitCount;
    if (position >= 32)
    {
        bufferA = bufferB;
        uint32 tmBuffer = *(tail + 2);
        bufferB = tmBuffer;
        tail++;
        position -= 32;
    }
}

void Bitstream::Forward(uint32 bitCount)
{
    position += bitCount;
    if (position >= 32)
    {
        uint32 tmBuffer = writeBuffer;
        *tail++ = tmBuffer;
        position -= 32;
        writeBuffer = 0;
    }
}

uint32 Bitstream::ReadBits(uint32 bitCount)
{
    uint32 tmResultBits = ShowBits(bitCount);
    Skip(bitCount);
    return tmResultBits;
}

uint32 Bitstream::ShowBits(uint32 bitCount)
{
    int32 tmBit = bitCount + position - 32;
    if (tmBit > 0)
    {
        return (((bufferA & (0xffffffff >> position)) << tmBit) | (bufferB >> (32 - tmBit)));
    }
    else
    {
        return ((bufferA & (0xffffffff >> position)) >> (-tmBit));
    }
}

void Bitstream::WriteBit(uint32 bitValue)
{
    if (bitValue)
        writeBuffer |= (0x80000000 >> position);
    Forward(1);
}

void Bitstream::WriteBits(uint32 bitValue, uint32 bitCount)
{
    int32 tmBit = bitCount + position - 32;
    uint32 tmBit2 = 32 - bitCount - position;

    if (tmBit2 <= 32)
    {
        writeBuffer |= (bitValue << tmBit2);
        Forward(bitCount);
    }
    else
    {
        writeBuffer |= (bitValue >> tmBit);
        Forward(bitCount - tmBit);
        int32 tmRemain = tmBit;

        writeBuffer |= (bitValue << (32 - tmBit));
        Forward(tmRemain);
    }
}

// Pad to byte boundary
void Bitstream::AlignToByte()
{
    uint32 tmRemainBits = 8 - position % 8;
    if (tmRemainBits == 1)
        WriteBits(0, tmRemainBits);
    else
    if (tmRemainBits < 8)
        WriteBits(((1 << (tmRemainBits - 1)) - 1), tmRemainBits);
}
};