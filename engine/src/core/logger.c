#include "core/logger.h"
#include "kohi_asserts.h"
#include <stdarg.h>
#include "core/kstring.h"
#include "memory/kmemory.h"
#include "platform/filesystem.h"
#include <string.h>

typedef struct LoggerSystemState {
    FileHandle logFileHandle;
} LoggerSystemState;

static LoggerSystemState* statePtr;

void append_to_log_file(const char* message) {
    if (statePtr && statePtr->logFileHandle.isValid) {
        // Since the message already contains a '\n', just write the bytes directly.
        u64 length = strlen(message);
        u64 written = 0;
        // if (!filesystem_write(&statePtr->logFileHandle, length, message, &written)) {
        //     platform_console_write_error("ERROR writing to console.log.", LOG_LEVEL_ERROR);
        // }
    }
}


b8 initialize_logging(u64* memoryRequirement, void* state ){
    *memoryRequirement = sizeof(LoggerSystemState);
    if(state == 0){
        return true;
    }
    statePtr = state;
    if(!filesystem_open("kohi.log",FILE_MODE_WRITE,false,&statePtr->logFileHandle)){
        platform_console_write_error("Unable to open log file for writing",LOG_LEVEL_ERROR);
        return false;
    }

     // TODO: create log file.
     return true;


}
void shutdown_logging(u64* memoryRequirement, void* state){
    

    // TODO: cleanup logging/write queued entries.

    statePtr = 0;

}

void log_output(LogLevel level, const char* message, ...){
    // TODO: These string operations are all pretty slow. This needs to be
    // moved to another thread eventually, along with the file writes, to
    // avoid slowing things down while the engine is trying to run.
     const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
     b8 is_error = level < LOG_LEVEL_WARN;


    // Technically imposes a 32k character limit on a single log entry, but...
    // DON'T DO THAT!
    char out_message[32000];
    kzero_memory(out_message, sizeof(out_message));

    // Format original message.
    // NOTE: Oddly enough, MS's headers override the GCC/Clang va_list type with a "typedef char* va_list" in some
    // cases, and as a result throws a strange error here. The workaround for now is to just use __builtin_va_list,
    // which is the type GCC/Clang's va_start expects.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    string_format_v(out_message, message, arg_ptr);
    va_end(arg_ptr);

     // Prepend log level to message.
    string_format(out_message, "%s%s\n", level_strings[level], out_message);

    
    // Platform-specific output.
    if (is_error) {
        platform_console_write_error(out_message, level);
    } else {
        platform_console_write(out_message, level);
    }

    char log_message[32000];
    kzero_memory(log_message, sizeof(log_message));
    string_ncopy(log_message,out_message,32000);
     // Queue a copy to be written to the log file.
    append_to_log_file(log_message);

}
void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
} 