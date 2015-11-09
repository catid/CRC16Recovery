// Hamming Distance CRC Polyonmial Evaluation
// usage:  hdlen  <polylist
//         hdlen poly
//         hdlen startHD stopHD  <polylist
//         hdlen poly startHD stopHD                        
// stdin is hex CRC polynomial up to 32 bits in implicit +1 notation
// if startHD and stopHD are not specified all HD lengths are computed from 3 up
// Example input:
//      ./hdlen 0x82608edb
// gives entire HD profile of CRC-32, which is:
//   0x82608edb {4294967263,91607,2974,268,171,91,57,34,21,12,10,10,10}
//
//      ./hdlen 0x82608edb 5 7
// gives HD=5, 6, and 7 profile of CRC-32 (skips slow HD computations)
//   0x82608edb {?,?,2974,268,171,?,?,?,?,?,?,?,?}
//  the "?" entries avoid confusion about which weights were computed
// Each "example" shown in output is a minimum-length codeword at the HD 
//     Example: Len=2975 {0,2215,2866} (0x80000000) (Bits=4)
//  means a 2975 bit long data word with first bit (bit zero) set, bits 2215
//     and bit 2866 set gives a computed CRC result of 0x80000000 for a total
//     of four bits set in that codeword. This is an example demonstrating that
//     CRC-32 0x82608edb fails to provide HD=5 at that length (gives only HD=4)
//
// Version 1.0.0  July 26, 2015
// Copyright 2015, Philip Koopman  koopman@cmu.edu
// Creative Commons Attribution-ShareAlike 4.0 International
//       http://creativecommons.org/licenses/by-sa/4.0/
// No warranty express or implied; use at your own risk
// User assumes responsibility for validating suitability for use
// We suggest using the non-optimized version for validation.
//
// Written as 64-bit code for g++ 4.5.3 but beware of portability problems 
//  Compile with:   g++ -O4 hdlen.cpp -DOPTZ -o hdlen
// Supports up to 64-bit CRCs, but realistically good large CRCs are going
//   to be too slow to compute to be viable at small HD values 

#include <iostream>
#include <cstdio>
#include <sstream>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <inttypes.h>
using namespace std;

//#define OPTZ  // If defined invokes some algorithmic optimizations

// Define 64-bit data types and I/O helpers
typedef uint64_t Poly_t;    // CRC polynomial value, implicit +1 notation
typedef uint64_t Length_t;  // dataword length
typedef uint32_t Count_t;   // used as an recursion, #bits set, or other count

typedef bool Flag_t;        // Success flag value

static const Count_t maxNumBitsPoly = sizeof(Poly_t) * 8;    // #bits in poly type
static const Count_t maxNumWeights = maxNumBitsPoly + 5;    // weight table size
static const Length_t unusedValue = 0xFFFFffffFFFFffffULL;  // all bits set

#ifdef OPTZ
#define ASSERT(f,s)   // no run-time overhead if running optimized
#else
inline void ASSERT(Flag_t f, const char *s)
{
    if (!f)
    {
        cerr << "********************" << endl << s << endl;
        exit(EXIT_FAILURE);
    }
}
#endif

//////////////////////// Helper to count bits //////////.///////////////////////
// return number of set bits in a polynomial
inline Count_t BitCount(Poly_t value)
{
    // helper lookup table for bit counting
    // array of # bits in a byte; index is byte value to be evaluated
    static unsigned int bitcountArray[256] =
    { // use int for native machine word size (efficiency) 
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
    }; // should be 256 entries

    // Count bits in each byte using lookup table
    // usually faster working with 32 bit intermediate value
    uint32_t halfVal = value;  // intentionally get just the low bits  

    Count_t result = bitcountArray[(halfVal)& 0xff];
    result += bitcountArray[(halfVal >> 8) & 0xff];
    result += bitcountArray[(halfVal >> 16) & 0xff];
    result += bitcountArray[(halfVal >> 24) & 0xff];

    // skip for common case of 32-bit polynomials with all zero top bits
    halfVal = value >> 32;
    if (halfVal)  // tally high half-word of bit counts
    {
        result += bitcountArray[(halfVal)& 0xff];
        result += bitcountArray[(halfVal >> 8) & 0xff];
        result += bitcountArray[(halfVal >> 16) & 0xff];
        result += bitcountArray[(halfVal >> 24) & 0xff];
    }
    return(result);
}

////////////////  Array to record minimum length that violates HD /////////////
class HDLen{
    friend ostream& operator <<(ostream& hout, const HDLen& h);
private:
    Poly_t   hPoly;
    Length_t HDlengths[maxNumWeights + 1];

public:
    HDLen(Poly_t poly);
    void     SetLen(Count_t weightNum, Length_t len); // Remember HD length
    Length_t GetLen(Count_t weightNum);               // Get HD length
};

// Constructor; remember poly and init table
HDLen::HDLen(Poly_t poly)
{
    hPoly = poly;
    // Init HD length table to all unused entries   
    for (Count_t weightNum = 0; weightNum < maxNumWeights; weightNum++)
    {
        HDlengths[weightNum] = unusedValue;
    }
}

// Set a particular weight number to be a certain dataword length
inline void HDLen::SetLen(Count_t weightNum, Length_t len)
{
    HDlengths[weightNum] = len;
}

// Retrieve dataword length of a weight number.  unusedValue if not found yet
inline Length_t HDLen::GetLen(Count_t weightNum)
{
    return(HDlengths[weightNum]);
}

// Print all computed HD lengths, putting in "?" for uncomputed lengths
//void HDLen::HDprint()
ostream& operator <<(ostream& hout, const HDLen& hval)
{
    //std::_Ios_Fmtflags current_flags = hout.flags();
    // print HD Lengths found surrounded by '{' '}'  
    char separator = '{';
    // print feedback polynomial
    hout << hex << "0x" << hval.hPoly << dec << " ";

    // print "?" for any uncomputed length starting at length 3
    Count_t weightNum = 3;
    while ((weightNum <= BitCount(hval.hPoly) + 1)
        && (hval.HDlengths[weightNum] == unusedValue)
        && (hval.HDlengths[weightNum] != 0)
        )
    {
        hout << separator << "?"; // ensures '{' and then ',' list separators
        separator = ',';
        weightNum++;
    } // end while

    // print all computed lengths until we get a zero or an unused value
    while ((weightNum <= BitCount(hval.hPoly) + 1)
        && (hval.HDlengths[weightNum] != unusedValue)
        && (hval.HDlengths[weightNum] != 0)
        )
    {
        hout << separator << hval.HDlengths[weightNum];
        separator = ',';  // ensures '{' and then ',' list separators
        weightNum++;
    } // end while

    // print "?" for any uncomputed length up to maximum HD supported by poly
    while (weightNum <= BitCount(hval.hPoly) + 1)
    {
        hout << separator << "?";
        separator = ',';
        weightNum++;
    } // end while

    // If we ended up with a null set then print leading {
    if (separator == '{')
    {
        hout << "{";
        separator = ',';
    }
    // finally ... end the list
    hout << "}";
    //hout.flags(current_flags);
    return hout;
}


////////////  Array to build up list of bits in an undetected error/////////////

class UndetectedClass
{
    friend ostream& operator <<(ostream& uout, const UndetectedClass& uval);
private:
    Poly_t uPoly;
    Length_t uLen;           // data word length being checked for HD violation
    Poly_t   uFCS;           // FCS value of undetected codeword
    Length_t posnList[maxNumWeights]; // bits set in first undetected codeword
    // subtract 1 from these values to get 0-based bit position
public:
    UndetectedClass(Poly_t p);  // Constructor
    void UInit(Poly_t p);            // Re-init data structure
    void SetFCS(Poly_t fcsval);      // Record FCS
    void SetLen(Length_t len);       // Record length for bit positioning
    void SetBitPosn(Count_t bitIndex, Length_t posn); // Record codeword bit
    void PrintBits(Length_t dataWordLen);             // Print example codeword
};

// Init undetected bit position data structure
UndetectedClass::UndetectedClass(Poly_t p)
{
    UInit(p);
}

// Re-initialize the undetected list for re-use (same init as constructor)
void UndetectedClass::UInit(Poly_t p)
{
    uPoly = p;
    uFCS = uPoly;
    uLen = unusedValue;
    // init all entries on list to unused
    for (Count_t bitIndex = 0; bitIndex < maxNumWeights; bitIndex++)
    {
        posnList[bitIndex] = unusedValue;
    }
}

// Remember the FCS, which is the result of CRC computation
void inline UndetectedClass::SetFCS(Poly_t fcsval)
{
    uFCS = fcsval;
}

// Remember the length, which is needed for proper printing
// It feels like a bit of a hack to do it this way, but otherwise we'd need
//   to pass total length down the recursion chain, which would be a pretty
//   big speed hit for no real reason. Putting it in Init got too messy.
// This function must be called before printing.  ASSERT used to flag this.
void inline UndetectedClass::SetLen(Length_t len)
{
    uLen = len;
}

// Set a particular bit position in the codeword bit list
void inline UndetectedClass::SetBitPosn(Count_t bitIndex, Length_t posn)
{
    posnList[bitIndex] = posn;
}

// Print out all bits in undetected codeword plus the FCS
ostream& operator <<(ostream& uout, const UndetectedClass& uval)
{
    //std::_Ios_Fmtflags current_flags = uout.flags();
    ASSERT(uval.uLen != unusedValue, "Must call SetLen first");

    Count_t bitsSet = 0;
    char separator = '{';  // make separated list elements look pretty

    // print all bit positions that have been recorded
    // Note that k-bit CRC highest possible HD is k; k+1 bit example is possible
    for (Count_t bitPosn = maxNumBitsPoly + 1; bitPosn != 0; bitPosn--)
    { // print only active entries; ignore unused entries
        if (uval.posnList[bitPosn] != unusedValue)
        {
            Length_t thisPosn = uval.uLen - uval.posnList[bitPosn] - 1;
            uout << separator << thisPosn;
            separator = ',';
            bitsSet++;  // tally up the number of entries actually found
        }
    }

    if (separator == '{') { uout << "{"; }         // print "{" if empty list
    uout << "} (0x" << hex << uval.uFCS << dec << ")";   // print } and FCS

    // # bits codeword = # bits in dataword (already computed) + # bits in FCS 
    bitsSet += BitCount(uval.uFCS);
    uout << " (Bits=" << bitsSet << ")";
    //uout.flags(current_flags);
    return uout;
}

///////////////////// Polynomial Class ////////////////////////////////////////

class CRCpoly{
private:
    Poly_t  cPoly;  // implicit +1 polynomial representation
    Count_t size;   // Active bits in the poly 1..64, excluding implicit +1
    Poly_t  cTopBitSet;    // word with only the top polynomial bit set
    Flag_t  cDivXP1;       // flag true if divisible by x+1
    Count_t cNumBitsSet;   // number of bits set in the polynomial
    HDLen * HDArray;               // HDLen array for this poly
    UndetectedClass * Undetected;  // Undetected bit array for this poly

    Flag_t CheckAccum(                    // Helper multiple bit FCS detection
        Poly_t accum, Length_t len, Count_t recursionsLeft);
    inline Flag_t CheckLastTwo(           // Helper last two bit detection
        Poly_t newAccumulator, Length_t len);

    Length_t FindHD3();                   // Optimized helper for HD=3 case
    Length_t FindHD4();                   // Optimized helper for HD=4 case
    Flag_t   FindHDRecurse(               // Dive deep to find HD general case
        Poly_t accumulator, Length_t maxLen, Count_t recursionsLeft);
    Count_t  FindHD(Count_t hdGoal);      // Base case for iteration

public:
    CRCpoly(Poly_t crcPoly);       // Constructor
    ~CRCpoly();                    // Destructor; release resources
    inline Poly_t Poly();                 // Retrieve binary polynomial value
    inline Poly_t TopBitSet();            // Retrieve just top bit in poly
    inline Poly_t DivXP1();               // Retrieve div x+1 flag
    inline Count_t NumBitsSet();          // Number of bits set in this poly
    inline Poly_t RollBy1(Poly_t val);    // Roll CRC 1 bit
    void   PolyHD(                        // Iterate across all HDs in search                
        Poly_t polyx, Count_t startHD, Count_t maxHD);
};

// Constructor. Cache results of characterization of poly and set up lists
CRCpoly::CRCpoly(Poly_t poly)
{
    // HDArray used to record HD lengths
    HDArray = new HDLen(poly);
    // Undetected array used to record bits in example undetected codeword
    Undetected = new UndetectedClass(poly);

    cPoly = poly;

    // Figure out size of polynomial 1..64
    // Count number of bits shifted out until last bit is shifted, leaving zero
    size = 0;
    while (poly != 0)
    {
        size += 1;
        poly = poly >> 1;  // C compiler shifts 0 into unsigned hi bit
    } // end while

    // Recreate top bit set now that we know the polynomial size
    // 64 polynomial has bit #63 set, so subtract 1 from size
    cTopBitSet = Poly_t(1ULL) << (size - 1); // avoid 32-bit intermediate

    // Determine if divisible by x+1 based on parity of bit count
    cNumBitsSet = BitCount(cPoly);
    cDivXP1 = ((cNumBitsSet & 1) != 0);
}

// Destructor -- releas HDArray and Undetected to avoid memory leaks
CRCpoly::~CRCpoly()
{
    delete HDArray;
    delete Undetected;
}

// Retrieve binary polynomial value
inline Poly_t CRCpoly::Poly()
{
    return(cPoly);
}

// Retrieve just top bit in poly
inline Poly_t CRCpoly::TopBitSet()
{
    return(cTopBitSet);
}

// Retriev div x+1 flag
inline Poly_t CRCpoly::DivXP1()
{
    return(cDivXP1);
}

// Number of bits set in this poly
inline Count_t CRCpoly::NumBitsSet()
{
    return(cNumBitsSet);
}

// Roll CRC 1 bit using shift and conditional XOR of the polynomial value
inline Poly_t CRCpoly::RollBy1(Poly_t val)
{
    Poly_t result = val >> 1;
    if (val & 1)  { result = result ^ cPoly; }
    return(result);
}

// Helper to determine if residual number of bits in a computed FCS value
//  is small enough to violate the HD.  RecursionsLeft gives bit budget left
inline Flag_t CRCpoly::CheckAccum(Poly_t accum,
    Length_t len, Count_t recursionsLeft)
{
    Flag_t retval = 0;
    if (BitCount(accum) <= recursionsLeft)
    { // too few bits in FCS, so this codeword fails to provide HD
        Undetected->SetFCS(accum);          // Record FCS and this bit position
        Undetected->SetBitPosn(recursionsLeft + 1, len);
        retval = 1;
    }
    return(retval);  // exit loop with successful find
}

// Flatten last two levels of recursion into a search for a single codeword
//    bit that leaves only top bit set as FCS residue
// (Note: case of two bits in FCS must be handled separately with CheckAccum)
// Last level of recursion never finds anything new because highest bit set 
//   in FCS before a bit in the dataword gets a chance to cancel out.
//   So don't need to look for pair of bits that leaves zero FCS residue. 
// Return Value: 1 means found HD violation; 0 means no violation
inline Flag_t CRCpoly::CheckLastTwo(Poly_t accum, Length_t len)
{
    Flag_t retval = 0; // default is no HD violation
    const Count_t recursionsLeft = 2;  // only called when 2; used for readability

    // When we start we have several codeword bits set including one at len
    // Want to find a codeword bit that differs from current accum value
    //    by having only highest bit inverted, which would leave that bit in FCS
    const Poly_t matchValue = accum ^ cTopBitSet;
    Length_t innerLen = 0;
    Poly_t rollingValue = cPoly;  // Roll the candidate bit from 0 toward len 

    while (innerLen != len)   // all bit positions until get to len, exclusive
    { // Roll this bit looking for a match 
        if (rollingValue == matchValue)
        { // Top bit set, exactly giving number of bits needed to violate HD 
            Undetected->SetFCS(rollingValue ^ accum);
            Undetected->SetBitPosn(recursionsLeft - 1, innerLen);
            Undetected->SetBitPosn(recursionsLeft, len);
            retval = 1;  break; // exit loop with successful find 
        }
        innerLen++;
        rollingValue = RollBy1(rollingValue);  // roll to next bit position
    }  // end while
    return(retval);
}

//////////////////////// Recursively find codeword violating HD ////////////////

// Recursively find whether this polynomial at this length provides certain HD 
// Dives into the middle of a computation with accumulator representing
//   the aggregate contribution to final FCS value from previous bits
// Length starts from bit that is the last bit before the FCS and works
//   "backward" toward longer word lengths.  The codeword bit at the
//   maxLen position has its bit set from the calling level of recursion          
//   maxLen must be 1 or greater   
// Return: 1 if counter-example found; 0 poly meets this HD at this length
Flag_t CRCpoly::FindHDRecurse(
    Poly_t accum, Length_t maxLen, Count_t recursionsLeft)
{
    int retval = 0;  // 1 means found an HD violation
    Poly_t rollingValue = cPoly;  // rolling a bit at position zero
    Length_t len = 1;  // speed optz: zero length checked by caller CheckAccum

    ASSERT((maxLen != 0), "maxLen should be zero");
    // Check all lengths up to, but excluding maxLen (which already has a bit set)
    // Use "break" to exit the while loop only if a HD violation has been found
    while (len != maxLen)
    {
        rollingValue = RollBy1(rollingValue);  // roll to next bit position
        Poly_t newAccum = rollingValue ^ accum;

        // More than one bit in FCS might cause HD violation, so do a bit count
        if (CheckAccum(newAccum, len, recursionsLeft))  { retval = 1; break; }

#ifdef OPTZ   // Only include for optimized code      
        // tail recursion elimination, special case for bottom of recursion dive
        //ASSERT((recursionsLeft >= 2), "Should never be 0 or 1 recursions");
        if (recursionsLeft == 2)
        {
            if (CheckLastTwo(newAccum, len)) { retval = 1; break; }
        }
        else
#else  // Alternative code if not optimized
        // If unoptimized make sure to check for the last bit set in dataword
        if ((recursionsLeft > 0))
#endif    
        { // recurse to add contribution of the next bit in the dataword 
            //   looking for # bits in codeword corrupted less than or equal to HD-1
            if (   // Record this bit and recurse to check more dataword bits
                FindHDRecurse(newAccum, len, recursionsLeft - 1)
                // Do bit position 0 before recursing to simplify while loop
                || CheckAccum(newAccum ^ cPoly, 0, recursionsLeft - 1)
                )
            { // If either search found a match, record this bit and unwind recursion
                Undetected->SetBitPosn(recursionsLeft + 1, len);
                retval = 1; break; // exit loop with successful find   (retval = 1)
            }
        }
        len++; // Try the next bit at this level of recursion
    }  // end while
    return(retval); // Returns 1 if HD violation
    // Returns 0 if no HD violation found after loop completes
}

// Special case for HD=3
// Look for one bit in dataword that results in top bit set in FCS
//   (This always happens before a self-cancelling dataword corruption)
Length_t CRCpoly::FindHD3()
{
    Flag_t doneFlag = 0;
    // Start with one bit set at the closest bit position to FCS
    Poly_t accum = cPoly;
    Length_t len = 0;

    // Roll the CRC to increasing lengths
    // First HD=3 undetected codeword will be 0x8000 or similar (top bit set)
    while (accum != cTopBitSet)
    {
        accum = RollBy1(accum);
        len++;
    } // end while

    // Record undetected codeword 
    Undetected->SetFCS(TopBitSet());
    Undetected->SetBitPosn(3 - 1, len);
    return(len);
}

// Special case for HD=4
// Two termination criteria:
//     Case 1:  1 bit dataword results in 2-bit FCS
//     Case 2:  2 bits in dataword results in 1-bit FCS with top bit set
Length_t CRCpoly::FindHD4()
{
    // Work two bits in data word.  Outer loop is first bit starting at posn 0
    Length_t len = 0;
    Poly_t accum = cPoly;

    // Run loop until we find a match
    Flag_t doneFlag = 0;
    while (!doneFlag)
    {
        // Print out "I'm alive" outer loop progress to Stderr; can take a while
        if ((len & 0xFFFF) == 0xFFFF) cerr << "... working; len=" << len + 1 << endl;

        // Is outer loop bit itself enough to cause HD=3 via one or two bit FCS?
        if (BitCount(accum) <= 2)
        {
            Undetected->SetFCS(accum);
            break;
        }

        // Move on to next outer loop position
        len++;
        accum = RollBy1(accum);

        // Consider all possible second bits that result in to bit set FCS
        // Find this by inverting top bit in outer loop FCS val checking equality
        const Poly_t matchVal = accum ^ cTopBitSet;
        Length_t innerLen = 0;
        Poly_t innerAccum = cPoly;

        // Check all lengths up to but excluding outer loop bit position    
        while (innerLen != len)
        { // If inner loop FCS value = outer loop xor top bit, it's a 3-bit codeword
            if (innerAccum == matchVal)
            {
                Undetected->SetFCS(TopBitSet());
                Undetected->SetBitPosn(3 - 2, innerLen);
                doneFlag = 1;
                break;
            }
            // Roll for next inner loop bit position
            innerAccum = RollBy1(innerAccum);
            innerLen++;
        }  // end inner while
    }  // end outer while
    // Always finds something; record outer loop bit position
    Undetected->SetBitPosn(3 - 1, len);
    return(len);
}


/////////////////////////////// Driver Routine /////////////////////////////////

// Outer loop to find longest dataword length at a particular HD
Count_t CRCpoly::FindHD(Count_t hdGoal)
{
    Flag_t doneFlag = 0;
    Count_t bitsSet = 0;
    Poly_t accum = Poly();  // Accumulator walking first bit error                    
    Length_t len = 0;

#ifdef OPTZ  // Only compile if optimizing
    if (hdGoal == 3)  // HD=3 special case
    {
        Undetected->UInit(Poly());
        len = FindHD3();
    }
    else if (DivXP1()              // Recycle result if div by x+1 permits 
        && (HDArray->GetLen(hdGoal - 1) != unusedValue)  // valid result avail?
        && ((hdGoal & 1) == 0)    // Can only recycle even HD results
        )
    { // skip computation because same as next lower HD for div by x+1
        // note that we do not re-init Undetected because we're going to reuse it
        len = HDArray->GetLen(hdGoal - 1);
    }
    else if (hdGoal == 4)  // HD=4 special case and can't recycle (not div x+1)
    {
        Undetected->UInit(Poly());
        len = FindHD4();
    }
    else  // Do it the hard way 
#endif
    {
        Undetected->UInit(Poly());

        // Roll CRC for one bit that defines length of the dataword
        //     (leading zeros do not affect codeword for HD purposes)
        // Use break statements to exit loop for speed

        // Check length zero special case
        if (!CheckAccum(accum, len, hdGoal - 2))
            while (1)
            {
                // advance the first bit further away from the FCS field by 1 bit
                len++;
                accum = RollBy1(accum);

                // Check to see if this one bit causes HD violation
                if (CheckAccum(accum, len, hdGoal - 2))            { break; }
                // add in other bits to see if they cause a failure
                // Check bit position zero before recursing
                else if (CheckAccum(accum^ cPoly, 0, hdGoal - 3))  { break; }
                // Recurse to see if there is a HD violation at this length
                else if (FindHDRecurse(accum, len, hdGoal - 3))    { break; }
            } // end while

        // found the first error that exceeds HD; roll back to good len 
        Undetected->SetBitPosn(hdGoal - 1, len);
    }
    // Exit loop when first bit has found what we are looking for
    HDArray->SetLen(hdGoal, len);

    // zero happens if the very first bit exceeds HD threshold
    cout << "# 0x" << hex << Poly() << dec << "  HD=" << hdGoal;
    if (len == 0)  // Impossible to meet this HD
    {
        cout << "  NONE  ";
    }
    else  // have found a length that supports this HD
    {
        cout << "  len=" << len << "  ";
    }

    // Print the example showing next longer dataword violates HD
    cout << "Example: Len=" << len + 1 << " ";
    Undetected->SetLen(len + 1);
    cout << *Undetected << endl;
    return(len);
}


// Print selected range of HDs for a polynomial
void CRCpoly::PolyHD(Poly_t polyx, Count_t startHD, Count_t maxHD)
{
    // All CRCs have HD=2 at infinite length, so clamp starting HD at 3
    if (startHD < 3)  { startHD = 3; }
    // All CRCs violate HD= # active bits in polynomial+1  at length=1
    //   So clamp max HD at that number +1 to show the "NONE" entry
    if (maxHD < startHD) { maxHD = BitCount(polyx) + 2; }

    // Iterate currentHD across requested range
    Count_t currentHD = startHD;

    // Status to stderr to track progress when running a batch
    cerr << "Poly=0x" << hex << polyx << dec;
    cerr << " startHD=" << currentHD << " maxHD=" << maxHD << endl;

    // Find HD for requested HD range
    while (currentHD <= maxHD)
    {
        FindHD(currentHD);
        currentHD++;
    } // end while
    // Print summary findings as last item
    cout << *HDArray << endl;
}

//////////////////////////// Main //////////////////////////////////////////////

int main_1(int argc, char **argv)
{
    Poly_t p = 0;             // Working CRC polynomial
    Count_t count = 0;      // I/O item count
    Count_t startHD = 0;    // Requested start HD range, if any
    Count_t maxHD = 0;      // Requested max HD range, if any
    Flag_t useStdin = 0;    // Are we getting inputs from STDIN?
    Flag_t useAutoHD = 0;   // Are we using auto-scaled HD ranges?

    istringstream cmd;      // command line argument stream
    Flag_t failure = 0;     // command line parsing failure if 1

    switch (argc)
    { //  CMD:   ./hdlen   <polyfile.txt
    case 1: // no command line inputs; full HD from stdin
        useStdin = 1;
        useAutoHD = 1;
        break;

        //  CMD:   ./hdlen 0x82608edb      
    case 2: // poly as only command line input
        useStdin = 0;
        useAutoHD = 1;
        cmd.clear(); cmd.str(argv[1]); if (!(cmd >> hex >> p)) { failure = 1; }
        break;


        //  CMD:   ./hdlen 5 7  <polyfile.txt     
    case 3: // minHD and maxHD as command line inputs
        useStdin = 1;
        useAutoHD = 0;
        cmd.clear(); cmd.str(argv[1]); if (!(cmd >> dec >> startHD)) { failure = 1; }
        cmd.clear(); cmd.str(argv[2]); if (!(cmd >> dec >> maxHD)) { failure = 1; }
        break;

        //  CMD:   ./hdlen 0x82608edb 5 7      
    case 4: // poly, minHD and maxHD as command line inputs
        useStdin = 0;
        useAutoHD = 1;
        cmd.clear(); cmd.str(argv[1]); if (!(cmd >> hex >> p)) { failure = 1; }
        cmd.clear(); cmd.str(argv[2]); if (!(cmd >> dec >> startHD)) { failure = 1; }
        cmd.clear(); cmd.str(argv[3]); if (!(cmd >> dec >> maxHD)) { failure = 1; }
        break;

    default: // illegal number of command line parameters
        failure = 1;
        break;
    } // end of switch

    if (failure)
    {
        cerr << "Usage: " << argv[0]
            << " [poly] | [StartHD MaxHD] | [poly StartHD MaxHD]" << endl;
    }
    else
        do //  do ... while to ensure always process at least one poly if available 
        {
            // Get next polynomial to analyze
            if (useStdin)
            {
                cin >> hex >> p >> dec;
                if (cin.fail()) { break; }
            }

            // do complete computation for one polynomial
            CRCpoly * Poly = new CRCpoly(p);
            Poly->PolyHD(p, startHD, maxHD);
            delete Poly;
        } while ((!feof(stdin)) && useStdin);   // end while

        return(failure);
}