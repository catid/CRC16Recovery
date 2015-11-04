/*
	Copyright (c) 2015 Christopher A. Taylor.  All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	* Neither the name of CRC16_ECC240 nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CRC16_ECC240_h
#define CRC16_ECC240_h

#include <stdint.h> // uint32_t etc
#include <string.h> // memcpy, memset

// Library version
#define CRC16_ECC240_VERSION 1

// Works with 30 bytes of data
#define CRC16_ECC240_DATA_BYTES 30


//-----------------------------------------------------------------------------
// Platform-Specific Definitions
//
// Edit these to port to your architecture

#ifdef _MSC_VER

    // Compiler-specific C++11 restrict keyword
    #define CRC16_ECC240_RESTRICT __restrict

    // Compiler-specific force inline keyword
    #define CRC16_ECC240_FORCE_INLINE __forceinline

    // Compiler-specific alignment keyword
    #define CRC16_ECC240_ALIGNED __declspec(align(16))

    // Compiler-specific 128-bit SIMD register keyword
    #define CRC16_ECC240_M128 __m128i

    // Compiler-specific SSE headers
    #include <wmmintrin.h> // _mm_clmulepi64_si128

#else

    #error "Compiler unsupported : Add support here."

#endif


#ifdef __cplusplus
extern "C" {
#endif


// Returns 0 on success.
// Returns non-zero on failure.
int crc16_ecc240_self_test();

// Compute the CRC16 for 240 bits (30 bytes) of data.
// data: Zero-padded 30 bytes.
uint16_t crc16_ecc240_generate(uint8_t* data);

// May modify the data to correct errors.
// Returns 0 on success.
// Returns non-zero on failure to correct the data.
int crc16_ecc240_correct(uint8_t* receivedData, uint16_t receivedCRC);


#ifdef __cplusplus
}
#endif


#endif // CRC16_ECC240_h
