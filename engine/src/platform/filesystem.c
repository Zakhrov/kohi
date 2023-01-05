#include "platform/filesystem.h"
#include "core/logger.h"
#include "memory/kmemory.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


b8 filesystem_exists(const char* path){
    struct stat buffer;
    return stat(path, &buffer) == 0;
}


b8 filesystem_open(const char* path, FileModes mode, b8 binary, FileHandle* outHandle){
     outHandle->isValid = false;
    outHandle->handle = 0;
    const char* mode_str;

    if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) != 0) {
        mode_str = binary ? "w+b" : "w+";
    } else if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) == 0) {
        mode_str = binary ? "rb" : "r";
    } else if ((mode & FILE_MODE_READ) == 0 && (mode & FILE_MODE_WRITE) != 0) {
        mode_str = binary ? "wb" : "w";
    } else {
        KERROR("Invalid mode passed while trying to open file: '%s'", path);
        return false;
    }

    // Attempt to open the file.
    FILE* file = fopen(path, mode_str);
    if (!file) {
        KERROR("Error opening file: '%s'", path);
        return false;
    }

    outHandle->handle = file;
    outHandle->isValid = true;

    return true;

}


void filesystem_close(FileHandle* handle){
    if (handle->handle) {
        fclose((FILE*)handle->handle);
        handle->handle = 0;
        handle->isValid = false;
    }
}

b8 filesystem_read_line(FileHandle* handle, u64 max_length, char** line_buf, u64* out_line_length) {
    if (handle->handle && line_buf && out_line_length && max_length > 0) {
        char* buf = *line_buf;
        if (fgets(buf, max_length, (FILE*)handle->handle) != 0) {
            *out_line_length = strlen(*line_buf);
        }
    }
    return false;
}

b8 filesystem_write_line(FileHandle* handle, const char* text){
     if (handle->handle) {
        i32 result = fputs(text, (FILE*)handle->handle);
        if (result != EOF) {
            result = fputc('\n', (FILE*)handle->handle);
        }

        // Make sure to flush the stream so it is written to the file immediately.
        // This prevents data loss in the event of a crash.
        fflush((FILE*)handle->handle);
        return result != EOF;
    }
    return false;
}

b8 filesystem_read(FileHandle* handle, u64 dataSize, void* outData, u64* outBytesRead){
     if (handle->handle && outData) {
        *outBytesRead = fread(outData, 1, dataSize, (FILE*)handle->handle);
        if (*outBytesRead != dataSize) {
            return false;
        }
        return true;
    }
    return false;
}


b8 filesystem_read_all_bytes(FileHandle* handle, u8** outBytes, u64* outBytesRead){
     if (handle->handle) {
        // File size
        fseek((FILE*)handle->handle, 0, SEEK_END);
        u64 size = ftell((FILE*)handle->handle);
        rewind((FILE*)handle->handle);

        *outBytes = kallocate(sizeof(u8) * size, MEMORY_TAG_STRING);
        *outBytesRead = fread(*outBytes, 1, size, (FILE*)handle->handle);
        if (*outBytesRead != size) {
            return false;
        }
        return true;
    }
    return false;
}

b8 filesystem_write(FileHandle* handle, u64 dataSize, const void* data, u64* outBytesWritten){
    if (handle->handle) {
        *outBytesWritten = fwrite(data, 1, dataSize, (FILE*)handle->handle);
        if (*outBytesWritten != dataSize) {
            return false;
        }
        fflush((FILE*)handle->handle);
        return true;
    }
    return false;
}
