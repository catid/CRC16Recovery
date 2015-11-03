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


// Input is the high 16 bits of the polynomial.
// The low bit is implicitly 1.
static bool CheckTruncatedPoly(uint32_t poly)
{
    uint32_t lfsr = 1;

    int count = 0;
    for (int ii = 0; ii < 0x10000 - 1; ++ii)
    {
        uint32_t lsb = lfsr & 1;
        lfsr >>= 1;
        if (lsb) lfsr ^= poly;
        if (lfsr == 1) ++count;
    }

    if (lfsr == 1 && count == 1)
    {
        return true;
    }

    return false;
}

// 240 bits of data for self test
static const int SelfTest_datalen = 30;
static uint8_t SelfTest_data[30];
static bool* SelfTest_seen;

static bool check_n_runs(int n)
{
    for (int i = 0; i < SelfTest_datalen * 8 - n + 1; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            int k = i + j;
            SelfTest_data[k / 8] ^= 1 << (k % 8);
        }

        uint16_t r = crc16_ecc240_generate(SelfTest_data);

        for (int j = 0; j < n; ++j)
        {
            int k = i + j;
            SelfTest_data[k / 8] = 0;
        }

        // Check for repeats within the current n, which would be a failure.
        if (SelfTest_seen[r])
        {
            return false;
        }
        SelfTest_seen[r] = true;
    }

    return true;
}

static bool check_detect_240()
{
    for (int n = 1; n <= 240; ++n)
    {
        memset(SelfTest_seen, 0, sizeof(bool) * 0x10000);

        if (!check_n_runs(n))
        {
            return false;
        }
    }

    return true;
}

static bool check_correct_15()
{
    memset(SelfTest_seen, 0, sizeof(bool) * 0x10000);

    for (int n = 1; n <= 15; ++n)
    {
        if (!check_n_runs(n))
        {
            return false;
        }
    }

    return true;
}


//-----------------------------------------------------------------------------
// API


extern "C" int crc16_ecc240_self_test()
{
    // Verify polynomial has perfect error detection for 16 bits.
    if (!CheckTruncatedPoly(CRC16_ECC240_POLY >> 1))
    {
        return -1;
    }

    SelfTest_seen = new bool[0x10000];

    // Verify that CRC16_ECC can detect any run of errors over 240 bits.
    if (!check_detect_240())
    {
        delete[] SelfTest_seen;
        return -2;
    }

    // Verify that CRC16_ECC can correct any run of errors from 1-15 bits.
    if (!check_correct_15())
    {
        delete[] SelfTest_seen;
        return -3;
    }

    delete[] SelfTest_seen;
    return 0;
}


extern "C" uint16_t crc16_ecc240_generate(uint8_t * CRC16_ECC240_RESTRICT data)
{
    uint16_t r = 0;

    // Unroll i = 0
    r = (uint16_t)data[1] | ((uint16_t)data[0] << 8);

    for (int i = 2; i < 30; i += 2)
    {
        r = CRC16_ECC240_REDUCTION_TABLE[r];
        r ^= (uint16_t)data[i + 1] | ((uint16_t)data[i] << 8);
    }

    return CRC16_ECC240_REDUCTION_TABLE[r];
}


extern "C" int crc16_ecc240_correct(uint8_t* receivedData, uint16_t receivedCRC)
{
    uint16_t actualCRC = crc16_ecc240_generate(receivedData);

    uint16_t errorSyndrome = actualCRC ^ receivedCRC;

    if (errorSyndrome == 0)
    {
        // Already fine.
        return 0;
    }

    uint16_t runInverseInfo = CRC16_ECC240_RUN_INVERSE[errorSyndrome];

    if (runInverseInfo == 0)
    {
        // No data on how to fix this.
        return -1;
    }

    int errorOffset = runInverseInfo / 32;
    int errorRunLength = runInverseInfo % 32;

    for (int i = 0; i < errorRunLength; ++i)
    {
        const int j = errorOffset + i;
        receivedData[j / 8] ^= 1 << (j % 8);
    }

    uint16_t modifiedCRC = crc16_ecc240_generate(receivedData);

    if (modifiedCRC != receivedCRC)
    {
        return -2;
    }

    return 0;
}


} // namespace crc16ecc240
