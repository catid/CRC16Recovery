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

#include "crc16_ecc240.h"
#include "crc16_ecc240_tables.h"

namespace crc16ecc240 {


//-----------------------------------------------------------------------------
// crc16_ecc240_generate

static CRC16_ECC240_FORCE_INLINE uint16_t crc16_reduce(uint16_t r)
{
    return CRC16_ECC240_REDUCE[0][r & 0xff] ^ CRC16_ECC240_REDUCE[1][r >> 8];
}

extern "C" uint16_t crc16_ecc240_generate(uint8_t * CRC16_ECC240_RESTRICT data, int bytes)
{
    uint16_t r = 0;

    for (int i = 0; i < bytes; i += 2)
    {
        // Convert data into a 16-bit word
        uint16_t w = (uint16_t)data[i + 1] | ((uint16_t)data[i] << 8);

        r = crc16_reduce(r ^ w);
    }

    return r;
}


//-----------------------------------------------------------------------------
// crc16_ecc240_correct

static uint16_t crc_backwards(uint16_t crc)
{
    // Run the CRC backwards
    if (crc & 1)
    {
        crc = (crc >> 1) ^ (CRC16_ECC240_POLY >> 1);
    }
    else
    {
        crc >>= 1;
    }
    return crc;
}

static int GetSingleErrorBitLocation(uint16_t crc, int bytes)
{
    for (int i = 0; i < bytes * 8 + 16; i++)
    {
        crc = crc_backwards(crc);

        if (crc == 1)
        {
            return i; // Return the error location
        }
    }

    return -1; // Not found
}

extern "C" int crc16_ecc240_check(uint8_t* receivedData, int bytes, uint16_t receivedCRC)
{
    // Find error syndrome
    // This works because the error pattern is xor-additive with the CRC of the original data
    uint16_t actualCRC = crc16_ecc240_generate(receivedData, bytes);
    uint16_t errorSyndrome = actualCRC ^ receivedCRC;
    if (errorSyndrome == 0)
    {
        // Already fine
        return 0;
    }

    // Try to find the single error bit location
    int location = GetSingleErrorBitLocation(errorSyndrome, bytes);
    if (location == -1)
    {
        // Not found
        return -1;
    }

    // Correct the error
    int dataByteOffset = ((14 + bytes * 8) - location) / 8;
    int dataBitOffset = 7 - (((14 + bytes * 8) - location) % 8);
    receivedData[dataByteOffset] ^= 1 << dataBitOffset;

    // Check if the CRC matches now
    if (crc16_ecc240_generate(receivedData, bytes) != receivedCRC)
    {
        return -2;
    }

    return 0;
}


extern "C" int crc16_ecc240_self_test()
{
    static const int DataLength = 30;
    uint8_t data[DataLength];
    for (int i = 0; i < DataLength; ++i)
        data[i] = (uint8_t)i;

    uint16_t actual_crc = crc16_ecc240_generate(data, DataLength);
    static const uint16_t kExpectedCRC = 3995;

    if (actual_crc != kExpectedCRC)
    {
        return -1;
    }

    return 0;
}


} // namespace crc16ecc240
