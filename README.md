# CRC16Recovery
Optimized CRC16 with error recovery in C

Fairly compact CRC16 codebase for up to 240 bits (30 bytes).  It recovers from single bit errors.

I wrote this while exploring what was possible for error recovery using CRCs.  Single-bit/burst errors seem efficiently correctable, but other correctable errors seem to require a large awkward table.

If you want to use it, include the crc16_ecc240.h and crc16_ecc240.cpp files in your project.  The rest are not needed.


Future work:

+ Monte Carlo statistical tests to give more warm fuzzies about the approach.

+ Possibly, small burst errors can be corrected too.  Worried about (1) false alarms (2) reduced error detection capabilities.
