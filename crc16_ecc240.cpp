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

#ifdef CRC16_ENABLE_TABLE_GENERATION_CODE
    #include <iostream>
    using namespace std;
#endif

namespace crc16ecc240 {


//-----------------------------------------------------------------------------
// crc16_ecc240_generate

/*
    CRC reduction table.

    Table[0] is reduction for xx00 << 8
    Table[1] is reduction for 00yy << 16

    This enables a much smaller lookup table that can fit comfortably in L1 cache for better speed.
*/
const uint16_t CRC16_ECC240_REDUCE[2][256] = {
    {
        0, 0x5935, 0xb26a, 0xeb5f, 0x3de1, 0x64d4, 0x8f8b, 0xd6be, 0x7bc2, 0x22f7, 0xc9a8, 0x909d, 0x4623, 0x1f16, 0xf449, 0xad7c,
        0xf784, 0xaeb1, 0x45ee, 0x1cdb, 0xca65, 0x9350, 0x780f, 0x213a, 0x8c46, 0xd573, 0x3e2c, 0x6719, 0xb1a7, 0xe892, 0x3cd, 0x5af8,
        0xb63d, 0xef08, 0x457, 0x5d62, 0x8bdc, 0xd2e9, 0x39b6, 0x6083, 0xcdff, 0x94ca, 0x7f95, 0x26a0, 0xf01e, 0xa92b, 0x4274, 0x1b41,
        0x41b9, 0x188c, 0xf3d3, 0xaae6, 0x7c58, 0x256d, 0xce32, 0x9707, 0x3a7b, 0x634e, 0x8811, 0xd124, 0x79a, 0x5eaf, 0xb5f0, 0xecc5,
        0x354f, 0x6c7a, 0x8725, 0xde10, 0x8ae, 0x519b, 0xbac4, 0xe3f1, 0x4e8d, 0x17b8, 0xfce7, 0xa5d2, 0x736c, 0x2a59, 0xc106, 0x9833,
        0xc2cb, 0x9bfe, 0x70a1, 0x2994, 0xff2a, 0xa61f, 0x4d40, 0x1475, 0xb909, 0xe03c, 0xb63, 0x5256, 0x84e8, 0xdddd, 0x3682, 0x6fb7,
        0x8372, 0xda47, 0x3118, 0x682d, 0xbe93, 0xe7a6, 0xcf9, 0x55cc, 0xf8b0, 0xa185, 0x4ada, 0x13ef, 0xc551, 0x9c64, 0x773b, 0x2e0e,
        0x74f6, 0x2dc3, 0xc69c, 0x9fa9, 0x4917, 0x1022, 0xfb7d, 0xa248, 0xf34, 0x5601, 0xbd5e, 0xe46b, 0x32d5, 0x6be0, 0x80bf, 0xd98a,
        0x6a9e, 0x33ab, 0xd8f4, 0x81c1, 0x577f, 0xe4a, 0xe515, 0xbc20, 0x115c, 0x4869, 0xa336, 0xfa03, 0x2cbd, 0x7588, 0x9ed7, 0xc7e2,
        0x9d1a, 0xc42f, 0x2f70, 0x7645, 0xa0fb, 0xf9ce, 0x1291, 0x4ba4, 0xe6d8, 0xbfed, 0x54b2, 0xd87, 0xdb39, 0x820c, 0x6953, 0x3066,
        0xdca3, 0x8596, 0x6ec9, 0x37fc, 0xe142, 0xb877, 0x5328, 0xa1d, 0xa761, 0xfe54, 0x150b, 0x4c3e, 0x9a80, 0xc3b5, 0x28ea, 0x71df,
        0x2b27, 0x7212, 0x994d, 0xc078, 0x16c6, 0x4ff3, 0xa4ac, 0xfd99, 0x50e5, 0x9d0, 0xe28f, 0xbbba, 0x6d04, 0x3431, 0xdf6e, 0x865b,
        0x5fd1, 0x6e4, 0xedbb, 0xb48e, 0x6230, 0x3b05, 0xd05a, 0x896f, 0x2413, 0x7d26, 0x9679, 0xcf4c, 0x19f2, 0x40c7, 0xab98, 0xf2ad,
        0xa855, 0xf160, 0x1a3f, 0x430a, 0x95b4, 0xcc81, 0x27de, 0x7eeb, 0xd397, 0x8aa2, 0x61fd, 0x38c8, 0xee76, 0xb743, 0x5c1c, 0x529,
        0xe9ec, 0xb0d9, 0x5b86, 0x2b3, 0xd40d, 0x8d38, 0x6667, 0x3f52, 0x922e, 0xcb1b, 0x2044, 0x7971, 0xafcf, 0xf6fa, 0x1da5, 0x4490,
        0x1e68, 0x475d, 0xac02, 0xf537, 0x2389, 0x7abc, 0x91e3, 0xc8d6, 0x65aa, 0x3c9f, 0xd7c0, 0x8ef5, 0x584b, 0x17e, 0xea21, 0xb314,
    }, {
        0, 0xd53c, 0xf34d, 0x2671, 0xbfaf, 0x6a93, 0x4ce2, 0x99de, 0x266b, 0xf357, 0xd526, 0x1a, 0x99c4, 0x4cf8, 0x6a89, 0xbfb5,
        0x4cd6, 0x99ea, 0xbf9b, 0x6aa7, 0xf379, 0x2645, 0x34, 0xd508, 0x6abd, 0xbf81, 0x99f0, 0x4ccc, 0xd512, 0x2e, 0x265f, 0xf363,
        0x99ac, 0x4c90, 0x6ae1, 0xbfdd, 0x2603, 0xf33f, 0xd54e, 0x72, 0xbfc7, 0x6afb, 0x4c8a, 0x99b6, 0x68, 0xd554, 0xf325, 0x2619,
        0xd57a, 0x46, 0x2637, 0xf30b, 0x6ad5, 0xbfe9, 0x9998, 0x4ca4, 0xf311, 0x262d, 0x5c, 0xd560, 0x4cbe, 0x9982, 0xbff3, 0x6acf,
        0x6a6d, 0xbf51, 0x9920, 0x4c1c, 0xd5c2, 0xfe, 0x268f, 0xf3b3, 0x4c06, 0x993a, 0xbf4b, 0x6a77, 0xf3a9, 0x2695, 0xe4, 0xd5d8,
        0x26bb, 0xf387, 0xd5f6, 0xca, 0x9914, 0x4c28, 0x6a59, 0xbf65, 0xd0, 0xd5ec, 0xf39d, 0x26a1, 0xbf7f, 0x6a43, 0x4c32, 0x990e,
        0xf3c1, 0x26fd, 0x8c, 0xd5b0, 0x4c6e, 0x9952, 0xbf23, 0x6a1f, 0xd5aa, 0x96, 0x26e7, 0xf3db, 0x6a05, 0xbf39, 0x9948, 0x4c74,
        0xbf17, 0x6a2b, 0x4c5a, 0x9966, 0xb8, 0xd584, 0xf3f5, 0x26c9, 0x997c, 0x4c40, 0x6a31, 0xbf0d, 0x26d3, 0xf3ef, 0xd59e, 0xa2,
        0xd4da, 0x1e6, 0x2797, 0xf2ab, 0x6b75, 0xbe49, 0x9838, 0x4d04, 0xf2b1, 0x278d, 0x1fc, 0xd4c0, 0x4d1e, 0x9822, 0xbe53, 0x6b6f,
        0x980c, 0x4d30, 0x6b41, 0xbe7d, 0x27a3, 0xf29f, 0xd4ee, 0x1d2, 0xbe67, 0x6b5b, 0x4d2a, 0x9816, 0x1c8, 0xd4f4, 0xf285, 0x27b9,
        0x4d76, 0x984a, 0xbe3b, 0x6b07, 0xf2d9, 0x27e5, 0x194, 0xd4a8, 0x6b1d, 0xbe21, 0x9850, 0x4d6c, 0xd4b2, 0x18e, 0x27ff, 0xf2c3,
        0x1a0, 0xd49c, 0xf2ed, 0x27d1, 0xbe0f, 0x6b33, 0x4d42, 0x987e, 0x27cb, 0xf2f7, 0xd486, 0x1ba, 0x9864, 0x4d58, 0x6b29, 0xbe15,
        0xbeb7, 0x6b8b, 0x4dfa, 0x98c6, 0x118, 0xd424, 0xf255, 0x2769, 0x98dc, 0x4de0, 0x6b91, 0xbead, 0x2773, 0xf24f, 0xd43e, 0x102,
        0xf261, 0x275d, 0x12c, 0xd410, 0x4dce, 0x98f2, 0xbe83, 0x6bbf, 0xd40a, 0x136, 0x2747, 0xf27b, 0x6ba5, 0xbe99, 0x98e8, 0x4dd4,
        0x271b, 0xf227, 0xd456, 0x16a, 0x98b4, 0x4d88, 0x6bf9, 0xbec5, 0x170, 0xd44c, 0xf23d, 0x2701, 0xbedf, 0x6be3, 0x4d92, 0x98ae,
        0x6bcd, 0xbef1, 0x9880, 0x4dbc, 0xd462, 0x15e, 0x272f, 0xf213, 0x4da6, 0x989a, 0xbeeb, 0x6bd7, 0xf209, 0x2735, 0x144, 0xd478,
    }
};

static CRC16_ECC240_FORCE_INLINE uint16_t crc16_reduce(uint16_t r)
{
    return CRC16_ECC240_REDUCE[0][r & 0xff] ^ CRC16_ECC240_REDUCE[1][r >> 8];
}

extern "C" uint16_t crc16_ecc240_generate(const void* vdata, int bytes)
{
    const uint8_t * CRC16_ECC240_RESTRICT data = reinterpret_cast<const uint8_t * CRC16_ECC240_RESTRICT>(vdata);
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


#ifdef CRC16_ENABLE_TABLE_GENERATION_CODE

void GenerateAndPrint_CRC16_ECC240_REDUCTION_TABLE()
{
    uint16_t tables[2][256];

    // Generate table:

    for (uint32_t r = 0; r < 256; ++r)
    {
        uint32_t t = r << 8;

        for (int i = 0; i < 8; ++i)
        {
            t <<= 1;
            if (t >= 0x10000)
            {
                t ^= CRC16_ECC240_POLY;
            }
        }

        tables[0][r] = t;
    }

    for (uint32_t r = 0; r < 256; ++r)
    {
        uint32_t t = r << 8;

        for (int i = 0; i < 16; ++i)
        {
            t <<= 1;
            if (t >= 0x10000)
            {
                t ^= CRC16_ECC240_POLY;
            }
        }

        tables[1][r] = t;
    }

    // Print table:

    cout << "const uint16_t CRC16_ECC240_REDUCE[2][256] = {" << endl;
    cout << "    {" << endl << "        ";
    for (int seen = 0, i = 0; i < 256; ++i)
    {
        uint16_t x = tables[0][i];
        if (x == 0)
        {
            cout << "0, ";
        }
        else
        {
            cout << "0x" << hex << x << dec << ", ";
        }
        if ((++seen & 15) == 0) cout << endl << "        ";
    }
    cout << "    }, {" << endl;
    for (int seen = 0, i = 0; i < 256; ++i)
    {
        uint16_t x = tables[1][i];
        if (x == 0)
        {
            cout << "0, ";
        }
        else
        {
            cout << "0x" << hex << x << dec << ", ";
        }
        if ((++seen & 15) == 0) cout << endl << "        ";
    }
    cout << "    }" << endl;
    cout << "};" << endl;
}

#endif // CRC16_ENABLE_TABLE_GENERATION_CODE


} // namespace crc16ecc240
