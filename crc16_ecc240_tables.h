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

#ifndef CRC16_ECC240_Tables_h
#define CRC16_ECC240_Tables_h

#include <stdint.h>

namespace crc16ecc240 {


/*
    CRC16_ECC240_POLY

    Chosen CRC16 polynomial

    This polynomial is one of four that:

    (1) Is a Galois Field generator polynomial and so has perfect error detection for 16 bits of data.

        The crc16_ecc240() function is a 1:1 mapping between 16 bits of input and 16 bits of output.

    (2) Can detect "burst" error runs of length 1-240 bits.

        It produces unique output for any single run of ones.

    (3) Can recover from "burst" error runs of length 1-15 bits.

        XORing the received and actual CRC produces an error syndrome that has a 1:1 map back to
        the error location and length, which enables recovery.
*/
#define CRC16_ECC240_POLY 0x140cd


/*
    CRC16_ECC240_TABLE

    CRC reduction table.

    Table[0] is reduction for xx00 << 8
    Table[1] is reduction for 00yy << 16

    This enables a much smaller lookup table that can fit comfortably in L1 cache for better speed.
*/
extern const uint16_t CRC16_ECC240_REDUCE[2][256];

/*
    CRC16_ECC240_RUN_INVERSE

    This table is designed as a 1:1 map between the CRC16 final output from 240 bits of input, and
    the offset and length of 1-15 bit consecutive bit flips.

    This enables error recovery for data of exactly 240 bits.

    The received CRC16 of the original data vector XOR by the CRC16 of the received data vector is
    the map input.  The output is a 16-bit integer packed like this:

        (Error Offset << 5) | (Error Length in Bits)

    The error recovery should proceed as:

    (Step 1) Receive: ActualData(30 bytes), ActualCRC(2 bytes)

        uint8_t ActualData[30];
        uint16_t ActualCRC;

    (Step 2) Compute ExpectedCRC = CRC16(ActualData, 30)

        uint16_t ExpectedCRC = crc16_ecc(ActualData, 30);

    (Step 3) If Expected CRC is the Actual CRC, then there is no problem, accept the data.

        Otherwise the Expected CRC is not the Actual CRC:

    (Step 4) uint16_t RecoveryParams = CRC16_ECC240_RUN_INVERSE[ExpectedCRC ^ ActualCRC];

        If RecoveryParams is zero, then we cannot recover from this error, reject the data.

    (Step 5) Recover the error location and run length:

        int ErrorLocation = RecoveryParams / 32, ErrorRunLength = RecoveryParams % 32;

    (Step 6) Flip bits in the ActualData based on the suggested error location and run length:

        for (int i = 0; i < ErrorRunLength; ++i)
        {
            ActualData[i / 8] ^= 1 << (i % 8);
        }

    (Step 7) Recompute the modified CRC:

        uint16_t ModifiedCRC = crc16_ecc(ActualData, 30);

    (Step 8) Accept the modified data if the modified CRC now matches.

        if (ModifiedCRC == ExpectedCRC)
        {
            // Accept the data.  Error recovered!
        }
*/

extern const uint16_t CRC16_ECC240_RUN_INVERSE[0x10000];


//-----------------------------------------------------------------------------
// Tools

//#define CRC16_ENABLE_TABLE_GENERATION_CODE

#ifdef CRC16_ENABLE_TABLE_GENERATION_CODE

    void GenerateAndPrint_CRC16_ECC240_REDUCTION_TABLE();
    void GenerateAndPrint_CRC16_ECC240_RUN_INVERSE();

#endif // CRC16_ENABLE_TABLE_GENERATION_CODE

} // namespace crc16ecc240

#endif // CRC16_ECC240_Tables_h
