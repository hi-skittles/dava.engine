#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
namespace FileAPI
{
/**
	fileName - utf8 string
*/
FILE* OpenFile(const String& fileName, const String& mode);

/**
	close file with double check
*/
int32 Close(FILE* f);
/**
	fileName - utf8 string
	return 0 on success
*/
int32 RemoveFile(const String& fileName);
/**
	oldfileName - utf8 string
	newFileName - utf8 string
	return value - 0 upon success or non-zero on error
*/
int32 RenameFile(const String& oldFileName, const String& newFileName);

/**
	fileName - utf8 string
*/
bool IsRegularFile(const String& fileName);

/**
	dirName - utf8 string
*/
bool IsDirectory(const String& dirName);

/**
	fileName - utf8 string
	return std::numeric_limits<uint64>::max() on error
*/
uint64 GetFileSize(const String& fileName);
}
}
