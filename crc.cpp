#include <iostream>
#include <iomanip>
using namespace std;

#include <stdint.h>

#include "crc16_ecc240.h"


static void test_get_error(uint16_t crc, int bytes, int& errorOffset, uint16_t& errorSyndrome)
{
    int i;

    int smallestCRCOffset = -1;
    uint16_t smallestCRC = 0xffff;

    for (i = 0; i < bytes * 8 + 16; i++)
    {
        if (crc & 1)
        {
            crc = (crc >> 1) ^ (CRC16_ECC240_POLY >> 1);
        }
        else
        {
            crc >>= 1;
        }

        //cout << i << " has CRC : " << crc << endl;

        if (crc <= smallestCRC)
        {
            smallestCRC = crc;
            smallestCRCOffset = i;
        }
    }

    errorOffset = smallestCRCOffset;
    errorSyndrome = smallestCRC;
}



int main()
{
#ifdef CRC16_ENABLE_TABLE_GENERATION_CODE
    crc16ecc240::GenerateAndPrint_CRC16_ECC240_REDUCTION_TABLE();
#endif

    static const int DataLength = 30; // Must be even

    if (0 != crc16_ecc240_self_test())
    {
        cout << "FAILURE: Self test failed" << endl;
        exit(1);
    }

    // Generate a 32-byte test packet (DataLength bytes of data, 2 bytes of CRC)
    uint8_t data[DataLength];
    for (int i = 0; i < DataLength; ++i)
    {
        data[i] = (uint8_t)i;
    }

    uint16_t actual_crc = crc16_ecc240_generate(data, DataLength);

    for (int j = 0; j < DataLength * 8; ++j)
    {
        // Simulate a channel of transmission
        uint8_t modified_data[DataLength];

        memcpy(modified_data, data, DataLength);

        // Simulate a burst error
        modified_data[j / 8] ^= 1 << (j % 8);

        // Attempt correction
        if (0 == crc16_ecc240_check(modified_data, DataLength, actual_crc))
        {
            for (int j = 0; j < DataLength; ++j)
            {
                if (data[j] != modified_data[j])
                {
                    cout << "FAILURE: Data mismatch" << endl;
                    exit(1);
                }
            }
        }
        else
        {
            cout << "FAILURE: Could not correct the error" << endl;
            exit(2);
        }
    }

    cout << "Recovery success!" << endl;
    return 0;
}
