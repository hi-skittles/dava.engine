#pragma once

enum ResourceArchiverResult : int
{
    OK = 0,

    // user errors
    ERROR_WRONG_COMMAND_LINE = -1,

    // file system errors
    ERROR_CANT_OPEN_FILE = -10,
    ERROR_CANT_WRITE_FILE = -11,
    ERROR_CANT_CREATE_DIR = -12,
    ERROR_CANT_CHANGE_DIR = -13,

    // archive format errors
    ERROR_CANT_OPEN_ARCHIVE = -30,
    ERROR_CANT_EXTRACT_FILE = -31,
    ERROR_EMPTY_ARCHIVE = -32,

    // program errors
    ERROR_INTERNAL = -40,
};
