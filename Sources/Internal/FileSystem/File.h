#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class FilePath;

extern const String extDvpl;
/**
	\ingroup filesystem
	\brief class to work with file on disk
 */
class File : public BaseObject
{
public:
    /** File attributes enumeration
     Remember engine support only
     OPEN | READ - open existent file for reading
     OPEN | READ | WRITE - open existing file for read-writing
     CREATE | WRITE - create new file or open existing and truncate it
     APPEND | WRITE - open existing file and move to end or create new
	*/
    enum eFileAttributes : uint32
    {
        CREATE = 0x1, //!< only for [CREATE | WRITE] mode
        OPEN = 0x2, //!< for [OPEN | READ] or [OPEN | READ | WRITE]
        READ = 0x4, //!< for [OPEN | READ] or [OPEN | READ | WRITE]
        WRITE = 0x8, //!< for [CREATE | WRITE] or [APPEND | WRITE]
        APPEND = 0x10 //!< for [APPEND | WRITE]
    };

    //! File seek enumeration
    enum eFileSeek : uint32
    {
        SEEK_FROM_START = 1, //! Seek from start of file
        SEEK_FROM_END = 2, //! Seek from end of file
        SEEK_FROM_CURRENT = 3, //! Seek from current file position relatively
    };

    /**
		\brief function to create a file instance with give attributes.
        Use framework notation for paths.
		\param[in] filePath absolute or relative framework specific path to file
		\param[in] attributes combinations of eFileAttributes
		\returns file instance
	 */
    static File* Create(const FilePath& filePath, uint32 attributes);
    /**
        \brief Get this file name
        \returns name of this file
     */
    virtual const FilePath& GetFilename();

    /**
		\brief Write [dataSize] bytes to this file from [pointerToData]
		\param[in] sourceBuffer function get data from this buffer
		\param[in] dataSize size of data we want to write
		\returns number of bytes actually written
	 */
    virtual uint32 Write(const void* sourceBuffer, uint32 dataSize);

    /**
		\brief Write [sizeof(T)] bytes to this file from [value]
		\param[in] value function get data from this buffer
		\returns number of bytes actually written
	 */
    template <class T>
    uint32 Write(const T* value);

    /**
		\brief Write string.
		write null-terminated string from current position in file.
		\param[in] string string data loaded to this variable/
        \param[in] shouldNullBeWritten indicates does it require to save null terminator.
		\return true if success otherwise false
	 */
    virtual bool WriteString(const String& string, bool shouldNullBeWritten = true);

    /**
     \brief Write string
     write string without '\0' from current position in file
     \param[in] string - string data loaded to this variable
     \return true if success otherwise false
	 */
    virtual bool WriteNonTerminatedString(const String& string);

    /**
		\brief Write one line of text
		Write string and add /r/n in the end.
		\param[in] string - string to write
		\return true if success otherwise false
	 */
    virtual bool WriteLine(const String& string);

    /**
		\brief Read [dataSize] bytes from this file to [pointerToData]
		\param[in,out] destinationBuffer function write data to this pointer
		\param[in] dataSize size of data we want to read
		\return number of bytes actually read
	*/
    virtual uint32 Read(void* destinationBuffer, uint32 dataSize);

    /**
		\brief Read [sizeof(T)] bytes from this file to [value]
		\param[in,out] value function write data to this pointer
		\return number of bytes actually read
	 */
    template <class T>
    uint32 Read(T* value);

    /**
		\brief Read one line from text file to [pointerToData] buffer
		\param[in,out] destinationBuffer function write data to this buffer
		\param[in] bufferSize size of [pointerToData] buffer
		\return number of bytes actually read
	*/
    uint32 ReadLine(void* destinationBuffer, uint32 bufferSize);

    /**
    \brief Read one line from text file without line endings
     */
    String ReadLine();

    /**
		\brief Read string line from file to destination buffer with destinationBufferSize
		\param[in,out] destinationBuffer buffer for the data
		\param[in] destinationBufferSize size of the destinationBuffer, for security reasons
		\returns actual length of the string that was read
	 */
    virtual uint32 ReadString(char8* destinationBuffer, uint32 destinationBufferSize);
    uint32 ReadString(String& destinationString);

    /**
		\brief Get current file position
	*/
    virtual uint64 GetPos() const;

    /**
		\brief Get current file size if writing
		       and get real file size if file for reading
	*/
    virtual uint64 GetSize() const;

    /**
		\brief Set current file position
		\param position - position to set
		\param seekType - \ref IO::eFileSeek flag to set type of positioning
		\return true if successful otherwise false.
	*/
    virtual bool Seek(int64 position, eFileSeek seekType);

    //! return true if end of file reached and false in another case
    virtual bool IsEof() const;

    /**
        \brief Truncate a file to a specified length
        \param size A size, that file is going to be truncated to
    */
    virtual bool Truncate(uint64 size);

    /**
        \brief Flushes file buffers to output device
        \return true on success
    */
    virtual bool Flush();

    static String GetModificationDate(const FilePath& filePathname);

protected:
    File() = default;
    ~File() override;

    FilePath filename;

private:
    static File* LoadFileFromMountedArchive(const String& packName, const String& relative);
    static bool IsFileInMountedArchive(const String& packName, const String& relative);
    /**
    \brief funciton to create a file instance with give attributes directly without framework path management.
    \param[in] filePath absolute system path to file
    \param[in] attributes combinations of eFileAttributes
    \returns file instance
    */
    static File* PureCreate(const FilePath& filePath, uint32 attributes);
    static File* CompressedCreate(const FilePath& filename, uint32 attributes);
    // reads 1 byte from current line in the file and sets it in next char if it is not a line ending char. Returns true if read was successful.
    bool GetNextChar(uint8* nextChar);

    FILE* file = nullptr;
    uint64 size = 0;
};

template <class T>
uint32 File::Read(T* value)
{
    return static_cast<uint32>(Read(value, sizeof(T)));
}

template <class T>
uint32 File::Write(const T* value)
{
    return static_cast<uint32>(Write(value, sizeof(T)));
}
};
