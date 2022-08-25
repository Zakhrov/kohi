#pragma once
#include "../defines.h"
#ifdef __cplusplus
extern "C"
{
#endif



typedef struct FileHandle {
    void* handle;
    b8 isValid;

}FileHandle;

typedef enum FileModes{
 FILE_MODE_READ = 0x1,
 FILE_MODE_WRITE = 0x2,

}FileModes;


/**
 * Checks if a file with the given path exists.
 * @param path The path of the file to be checked.
 * @returns True if exists; otherwise false.
 */
KAPI b8 filesystem_exists(const char* path);

/** 
 * Attempt to open file located at path.
 * @param path The path of the file to be opened.
 * @param mode Mode flags for the file when opened (read/write). See file_modes enum in filesystem.h.
 * @param binary Indicates if the file should be opened in binary mode.
 * @param out_handle A pointer to a FileHandle structure which holds the handle information.
 * @returns True if opened successfully; otherwise false.
 */
KAPI b8 filesystem_open(const char* path, FileModes mode, b8 binary, FileHandle* outHandle);

/** 
 * Closes the provided handle to a file.
 * @param handle A pointer to a FileHandle structure which holds the handle to be closed.
 */
KAPI void filesystem_close(FileHandle* handle);

/** 
 * Reads up to a newline or EOF. Allocates *line_buf, which must be freed by the caller.
 * @param handle A pointer to a FileHandle structure.
 * @param line_buf A pointer to a character array which will be allocated and populated by this method.
 * @returns True if successful; otherwise false.
 */
KAPI b8 filesystem_read_line(FileHandle* handle, char** lineBuf);

/** 
 * Writes text to the provided file, appending a '\n' afterward.
 * @param handle A pointer to a FileHandle structure.
 * @param text The text to be written.
 * @returns True if successful; otherwise false.
 */
KAPI b8 filesystem_write_line(FileHandle* handle, const char* text);

/** 
 * Reads up to data_size bytes of data into out_bytes_read. 
 * Allocates *out_data, which must be freed by the caller.
 * @param handle A pointer to a FileHandle structure.
 * @param data_size The number of bytes to read.
 * @param out_data A pointer to a block of memory to be populated by this method.
 * @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
KAPI b8 filesystem_read(FileHandle* handle, u64 dataSize, void* outData, u64* outBytesRead);

/** 
 * Reads up to data_size bytes of data into out_bytes_read. 
 * Allocates *out_bytes, which must be freed by the caller.
 * @param handle A pointer to a FileHandle structure.
 * @param out_bytes A pointer to a byte array which will be allocated and populated by this method.
 * @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
KAPI b8 filesystem_read_all_bytes(FileHandle* handle, u8** outBytes, u64* outBytesRead);

/** 
 * Writes provided data to the file.
 * @param handle A pointer to a FileHandle structure.
 * @param data_size The size of the data in bytes.
 * @param data The data to be written.
 * @param out_bytes_written A pointer to a number which will be populated with the number of bytes actually written to the file.
 * @returns True if successful; otherwise false.
 */
KAPI b8 filesystem_write(FileHandle* handle, u64 dataSize, const void* data, u64* outBytesWritten);

#ifdef __cplusplus
}
#endif