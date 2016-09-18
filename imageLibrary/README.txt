Glop Image Library

Goal
 - simple and flexible API for basic image operations
 - Relative high speed decoding/encoding
 - Minimal computational overhead for pixel access

Building
 - make clean
 - make

Current Support
Decoding
 - PNG - all types
Encoding
 - PNG - all types except pallete for images with more than 256 colors

Current issues
 - Builds my own main for testing, not a library to compile with
 - unstable API - though probably not major changes
 - limited format support

Planned Format Support
 - PNG - animated
 - Jpeg
 - BMP
 - gif (with a g sound)