/*
OBS Stabilizer Plugin - Frame Data Adapter for Standalone/OBS Builds
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <cstdint>

#ifndef BUILD_STANDALONE
    // OBS plugin mode - use obs_source_frame
    #include <obs-module.h>
    
    namespace obs_stabilizer {
        using frame_t = struct obs_source_frame;
        
        // Use OBS video format constants
        using video_format = enum video_format;
        
        inline uint32_t get_frame_width(const frame_t* frame) { return frame->width; }
        inline uint32_t get_frame_height(const frame_t* frame) { return frame->height; }
        inline uint32_t get_frame_format(const frame_t* frame) { return frame->format; }
        inline uint8_t* get_frame_data(const frame_t* frame, size_t plane) { 
            return frame->data[plane]; 
        }
        inline uint32_t get_frame_linesize(const frame_t* frame, size_t plane) { 
            return frame->linesize[plane]; 
        }
    }
#else
    // Standalone mode - define minimal frame structure
    namespace obs_stabilizer {
        enum video_format {
            VIDEO_FORMAT_I420 = 1,
            VIDEO_FORMAT_NV12 = 2,
            VIDEO_FORMAT_RGBA = 3,
            VIDEO_FORMAT_BGRA = 4,
            VIDEO_FORMAT_BGRX = 5,
            VIDEO_FORMAT_Y800 = 6
        };
        
        struct standalone_frame {
            uint32_t width;
            uint32_t height;
            uint32_t format;
            uint8_t* data[8];        // Maximum 8 planes
            uint32_t linesize[8];    // Line sizes for each plane
            uint64_t timestamp;      // Frame timestamp
        };
        
        using frame_t = standalone_frame;
        
        inline uint32_t get_frame_width(const frame_t* frame) { return frame->width; }
        inline uint32_t get_frame_height(const frame_t* frame) { return frame->height; }
        inline uint32_t get_frame_format(const frame_t* frame) { return frame->format; }
        inline uint8_t* get_frame_data(const frame_t* frame, size_t plane) { 
            return frame->data[plane]; 
        }
        inline uint32_t get_frame_linesize(const frame_t* frame, size_t plane) { 
            return frame->linesize[plane]; 
        }
    }
#endif