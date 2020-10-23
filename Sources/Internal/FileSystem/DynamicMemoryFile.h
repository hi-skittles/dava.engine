#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "FileSystem/File.h"

namespace DAVA
{
class DynamicMemoryFile : public File
{
protected:
    DynamicMemoryFile();
    ~DynamicMemoryFile() override = default;

public:
    DynamicMemoryFile(DynamicMemoryFile&&) noexcept;
    DynamicMemoryFile& operator=(DynamicMemoryFile&&) noexcept;
    DynamicMemoryFile(const DynamicMemoryFile&) = delete;
    DynamicMemoryFile& operator=(DynamicMemoryFile&) = delete;

    static DynamicMemoryFile* Create(Vector<uint8>&& data, uint32 attributes, const FilePath& name);
    /**
     \brief funciton to create a file instance with give attributes
     \param[in] data pointer to data to create file from
     \param[in] dataSize size of data to create file from
     \param[in] attributes combinations of eFileAttributes
     \returns file instance
     */
    static DynamicMemoryFile* Create(const uint8* data, int32 dataSize, uint32 attributes);

    /**
     \brief funciton to create a file instance with give attributes
     \param[in] attributes combinations of eFileAttributes
     \returns file instance
     */
    static DynamicMemoryFile* Create(uint32 attributes);

    /**
     \brief returns pointer to the data contained in the memory file
     \returns pointer to the first byte of the file data if file is empty returns pointer to empty buffer
     */
    const uint8* GetData() const;

    const Vector<uint8>& GetDataVector() const;

    /**
     \brief Write [dataSize] bytes to this file from [pointerToData]
     \param[in] pointerToData function get data from this pointer
     \param[in] dataSize size of data we want to write
     \returns number of bytes actually written
     */
    uint32 Write(const void* pointerToData, uint32 dataSize) override;

    /**
     \brief Read [dataSize] bytes from this file to [pointerToData]
     \param[in, out] pointerToData function write data to this pointer
     \param[in] dataSize size of data we want to read
     \return number of bytes actually read
     */
    uint32 Read(void* pointerToData, uint32 dataSize) override;

    /**
     \brief Get current file position
     */
    uint64 GetPos() const override;

    /**
     \brief Get current file size if writing
            and get real file size if file for reading
     */
    uint64 GetSize() const override;

    /**
     \brief Set current file position
     \param position position to set
     \param seekType - \ref IO::eFileSeek flag to set type of positioning
     \return true if successful otherwise false.
     */
    bool Seek(int64 position, eFileSeek seekType) override;

    //! return true if end of file reached and false in another case
    bool IsEof() const override;

    /**
    \brief Truncate a file to a specified length
    \param size A size, that file is going to be truncated to
    */
    bool Truncate(uint64 size) override;

    bool Flush() override;

protected:
    uint64 currentPtr;
    Vector<uint8> data;
    uint32 fileAttributes;
    bool isEof = false;
};

inline const Vector<uint8>& DynamicMemoryFile::GetDataVector() const
{
    return data;
}
};
