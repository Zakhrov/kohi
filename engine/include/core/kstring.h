#pragma once
#include "../defines.h"

/**
 * @brief Performs string formatting to dest given format string and parameters.
 * @param dest The destination for the formatted string.
 * @param format The format string to use for the operation
 * @param ... The format arguments.
 * @returns The length of the newly-formatted string.
 */
KAPI i32 string_format(char* dest, const char* format, ...);

/**
 * @brief Performs variadic string formatting to dest given format string and va_list.
 * @param dest The destination for the formatted string.
 * @param format The string to be formatted.
 * @param va_list The variadic argument list.
 * @returns The size of the data written.
 */
KAPI i32 string_format_v(char* dest, const char* format, void* va_list);
