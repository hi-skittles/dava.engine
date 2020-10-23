#ifndef __LOGENGINE_BITSTREAM_H__
#define __LOGENGINE_BITSTREAM_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
//! \brief class to work with bitstreams
//!
class Bitstream
{
public:
    Bitstream(void* bitstreamPointer, uint32 bitstreamLength);
    virtual ~Bitstream();

    //! \brief function reset bitstream to start position
    void Reset();

    //! \brief function return size of bitstream
    uint32 Length();

    //! \brief function return current bitstream position
    uint32 GetPosition();

    //! \brief function to skip bits, by default skip 1 bit
    //! \param bitCount bit count to skip
    void Skip(uint32 bitCount = 1);

    //! \brief function to skip bits in writing mode by default skip 1 bit
    //! \param bitCount bit count to skip
    void Forward(uint32 bitCount = 1);

    //! \brief function to read bits from stream
    //! \param bitCount bit count to read
    uint32 ReadBits(uint32 bitCount = 1);

    //! \brief function to show next [bitCount] bits from stream
    //! \param bitCount bit count to show
    uint32 ShowBits(uint32 bitCount = 1);

    //! \brief function to write bitCount bits to stream
    //! \param bitCount bit count to write
    void WriteBits(uint32 bitValue, uint32 bitCount);

    //! \brief function to write 1 bit to stream
    void WriteBit(uint32 bitValue);

    //! \brief Align current stream to byte boundary
    void AlignToByte();

private:
    // Bitstream pointer
    uint32* head;
    uint32* tail;
    uint32 position;
    uint32 length;
    uint32 bufferA; // Read Buffer [0]
    uint32 bufferB; // Read Buffer [1]
    uint32 writeBuffer; // Write Buffer
};
};

#endif // __LOGENGINE_BITSTREAM_H__