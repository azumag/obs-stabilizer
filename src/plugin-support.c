/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "plugin-support.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_OBS_HEADERS
#include <stdio.h>
#endif

const char *PLUGIN_NAME = "obs-stabilizer";
const char *PLUGIN_VERSION = "0.1.0";

void obs_log(int log_level, const char *format, ...)
{
	// Enhanced input validation
	if (!PLUGIN_NAME || !format) return;
	
	// Validate format string length to prevent abuse
	size_t format_len = strlen(format);
	if (format_len > 2048) {
		// Format string too long - potential security risk
		return;
	}
	
	// Use safe stack allocation for typical log messages
	const size_t STACK_BUFFER_SIZE = 1024; // Safe default buffer size
	char stack_buffer[STACK_BUFFER_SIZE];
	char *template = stack_buffer;
	
	// More conservative buffer size calculation with bounds checking
	size_t plugin_name_len = strlen(PLUGIN_NAME);
	if (plugin_name_len > 256) plugin_name_len = 256; // Sanity limit
	
	// Calculate minimum required buffer size with safety margin
	size_t min_buffer_size = plugin_name_len + format_len + 32; // Extra space for "[%s] %s" template
	size_t buffer_size = min_buffer_size + 512; // Additional safety margin
	
	// Cap maximum buffer size to prevent excessive memory allocation
	const size_t MAX_BUFFER_SIZE = 8192;
	if (buffer_size > MAX_BUFFER_SIZE) {
		buffer_size = MAX_BUFFER_SIZE;
	}
	
	// Use heap allocation only for very large messages
	bool use_heap = buffer_size > STACK_BUFFER_SIZE;
	if (use_heap) {
		template = malloc(buffer_size);
		if (!template) {
			// Critical memory allocation failure - log error and return gracefully
			fprintf(stderr, "[%s] CRITICAL: Memory allocation failed for log message (size: %zu)\n", 
					PLUGIN_NAME, buffer_size);
			return;
		}
	} else {
		buffer_size = STACK_BUFFER_SIZE;
	}
	
	// Safe template construction with bounds checking
	int ret = snprintf(template, buffer_size, "[%s] %s", PLUGIN_NAME, format);
	if (ret < 0 || ret >= (int)buffer_size) {
		// Template construction failed or was truncated
		if (use_heap) free(template);
		return;
	}
	
	va_list args;
	va_start(args, format);
#ifdef HAVE_OBS_HEADERS
	blogva(log_level, template, args);
#else
	// Standalone mode - use printf for logging
	vprintf(template, args);
	printf("\n");
#endif
	va_end(args);
	
	if (use_heap) free(template);
}
