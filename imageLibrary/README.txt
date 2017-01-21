Glop Image Library

Goal
 - simple and flexible API for basic image operations
 - Relative high speed decoding/encoding
 - Minimal computational overhead for pixel access

Building
 - make full-clean
 - make

Current Support
Decoding
 - PNG - all types
 - JPG - all types
 - BMP - v5 type and only for non-indexed color and for 16,24,32 bit pixels
Encoding
 - PNG - all types except pallete
 - BMP - v5 type with or without Alpha, and 8bit channels only
 - JPG - all types

Current issues
 - Builds my own main for testing, not a library to compile with
 - unstable API - though probably not major changes
 - limited format support
 - relies on having the system installed with 2 specific libraries

Planned Format Support
 - PNG - animated
 - gif (with a g sound)