/*
 * SM4 Encryption alogrithm (SMS4 algorithm)
 * GM/T 0002-2012 Chinese National Standard ref:http://www.oscca.gov.cn/ 
 * thanks to Xyssl
 * thnaks and refers to http://hi.baidu.com/numax/blog/item/80addfefddfb93e4cf1b3e61.html
 * author:goldboar
 * email:goldboar@163.com
 * 2012-4-20
 */

// Test vector 1
// plain: 01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
// key:   01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
// 	   round key and temp computing result:
// 	   rk[ 0] = f12186f9 X[ 0] = 27fad345
// 		   rk[ 1] = 41662b61 X[ 1] = a18b4cb2
// 		   rk[ 2] = 5a6ab19a X[ 2] = 11c1e22a
// 		   rk[ 3] = 7ba92077 X[ 3] = cc13e2ee
// 		   rk[ 4] = 367360f4 X[ 4] = f87c5bd5
// 		   rk[ 5] = 776a0c61 X[ 5] = 33220757
// 		   rk[ 6] = b6bb89b3 X[ 6] = 77f4c297
// 		   rk[ 7] = 24763151 X[ 7] = 7a96f2eb
// 		   rk[ 8] = a520307c X[ 8] = 27dac07f
// 		   rk[ 9] = b7584dbd X[ 9] = 42dd0f19
// 		   rk[10] = c30753ed X[10] = b8a5da02
// 		   rk[11] = 7ee55b57 X[11] = 907127fa
// 		   rk[12] = 6988608c X[12] = 8b952b83
// 		   rk[13] = 30d895b7 X[13] = d42b7c59
// 		   rk[14] = 44ba14af X[14] = 2ffc5831
// 		   rk[15] = 104495a1 X[15] = f69e6888
// 		   rk[16] = d120b428 X[16] = af2432c4
// 		   rk[17] = 73b55fa3 X[17] = ed1ec85e
// 		   rk[18] = cc874966 X[18] = 55a3ba22
// 		   rk[19] = 92244439 X[19] = 124b18aa
// 		   rk[20] = e89e641f X[20] = 6ae7725f
// 		   rk[21] = 98ca015a X[21] = f4cba1f9
// 		   rk[22] = c7159060 X[22] = 1dcdfa10
// 		   rk[23] = 99e1fd2e X[23] = 2ff60603
// 		   rk[24] = b79bd80c X[24] = eff24fdc
// 		   rk[25] = 1d2115b0 X[25] = 6fe46b75
// 		   rk[26] = 0e228aeb X[26] = 893450ad
// 		   rk[27] = f1780c81 X[27] = 7b938f4c
// 		   rk[28] = 428d3654 X[28] = 536e4246
// 		   rk[29] = 62293496 X[29] = 86b3e94f
// 		   rk[30] = 01cf72e5 X[30] = d206965e
// 		   rk[31] = 9124a012 X[31] = 681edf34
// cypher: 68 1e df 34 d2 06 96 5e 86 b3 e9 4f 53 6e 42 46
// 		
// test vector 2
// the same key and plain 1000000 times coumpting 
// plain:  01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
// key:    01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
// cypher: 59 52 98 c7 c6 fd 27 1f 04 02 f8 04 c3 3d 3f 66

#include <string.h>
#include "sm4.h"
/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_ULONG_BE 
#define GET_ULONG_BE(n,b,i)                   \
{                                             \
    (n) = ( (u32) (b)[(i)    ] << 24 )        \
        | ( (u32) (b)[(i) + 1] << 16 )        \
        | ( (u32) (b)[(i) + 2] <<  8 )        \
        | ( (u32) (b)[(i) + 3]       );       \
}
#endif

#ifndef PUT_ULONG_BE
#define PUT_ULONG_BE(n,b,i)          \
{                                    \
    (b)[(i)    ] = ((u8*)&(n))[3];   \
	(b)[(i) + 1] = ((u8*)&(n))[2];   \
    (b)[(i) + 2] = ((u8*)&(n))[1];   \
    (b)[(i) + 3] = ((u8*)&(n))[0];   \
}
#endif

/*
 *rotate shift left marco definition
 *
 */
#define  SHL(x,n) ((x) << n)
#define ROTL(x,n) (SHL((x),n) | ((x) >> (32 - n)))

#define SWAP(a,b) { u32 t = a; a = b; b = t; t = 0; }

/*
 * Expanded SM4 S-boxes
 /* Sbox table: 8bits input convert to 8 bits output*/
 
static const u8 SboxTable[16][16] = 
{
{0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05},
{0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99},
{0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62},
{0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6},
{0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8},
{0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35},
{0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87},
{0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e},
{0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1},
{0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3},
{0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f},
{0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51},
{0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8},
{0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0},
{0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84},
{0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48}
};

static const u32 S1[256]=
{
0x000000d6,0x00000090,0x000000e9,0x000000fe,0x000000cc,0x000000e1,0x0000003d,0x000000b7,0x00000016,0x000000b6,0x00000014,0x000000c2,0x00000028,0x000000fb,0x0000002c,0x00000005,
0x0000002b,0x00000067,0x0000009a,0x00000076,0x0000002a,0x000000be,0x00000004,0x000000c3,0x000000aa,0x00000044,0x00000013,0x00000026,0x00000049,0x00000086,0x00000006,0x00000099,
0x0000009c,0x00000042,0x00000050,0x000000f4,0x00000091,0x000000ef,0x00000098,0x0000007a,0x00000033,0x00000054,0x0000000b,0x00000043,0x000000ed,0x000000cf,0x000000ac,0x00000062,
0x000000e4,0x000000b3,0x0000001c,0x000000a9,0x000000c9,0x00000008,0x000000e8,0x00000095,0x00000080,0x000000df,0x00000094,0x000000fa,0x00000075,0x0000008f,0x0000003f,0x000000a6,
0x00000047,0x00000007,0x000000a7,0x000000fc,0x000000f3,0x00000073,0x00000017,0x000000ba,0x00000083,0x00000059,0x0000003c,0x00000019,0x000000e6,0x00000085,0x0000004f,0x000000a8,
0x00000068,0x0000006b,0x00000081,0x000000b2,0x00000071,0x00000064,0x000000da,0x0000008b,0x000000f8,0x000000eb,0x0000000f,0x0000004b,0x00000070,0x00000056,0x0000009d,0x00000035,
0x0000001e,0x00000024,0x0000000e,0x0000005e,0x00000063,0x00000058,0x000000d1,0x000000a2,0x00000025,0x00000022,0x0000007c,0x0000003b,0x00000001,0x00000021,0x00000078,0x00000087,
0x000000d4,0x00000000,0x00000046,0x00000057,0x0000009f,0x000000d3,0x00000027,0x00000052,0x0000004c,0x00000036,0x00000002,0x000000e7,0x000000a0,0x000000c4,0x000000c8,0x0000009e,
0x000000ea,0x000000bf,0x0000008a,0x000000d2,0x00000040,0x000000c7,0x00000038,0x000000b5,0x000000a3,0x000000f7,0x000000f2,0x000000ce,0x000000f9,0x00000061,0x00000015,0x000000a1,
0x000000e0,0x000000ae,0x0000005d,0x000000a4,0x0000009b,0x00000034,0x0000001a,0x00000055,0x000000ad,0x00000093,0x00000032,0x00000030,0x000000f5,0x0000008c,0x000000b1,0x000000e3,
0x0000001d,0x000000f6,0x000000e2,0x0000002e,0x00000082,0x00000066,0x000000ca,0x00000060,0x000000c0,0x00000029,0x00000023,0x000000ab,0x0000000d,0x00000053,0x0000004e,0x0000006f,
0x000000d5,0x000000db,0x00000037,0x00000045,0x000000de,0x000000fd,0x0000008e,0x0000002f,0x00000003,0x000000ff,0x0000006a,0x00000072,0x0000006d,0x0000006c,0x0000005b,0x00000051,
0x0000008d,0x0000001b,0x000000af,0x00000092,0x000000bb,0x000000dd,0x000000bc,0x0000007f,0x00000011,0x000000d9,0x0000005c,0x00000041,0x0000001f,0x00000010,0x0000005a,0x000000d8,
0x0000000a,0x000000c1,0x00000031,0x00000088,0x000000a5,0x000000cd,0x0000007b,0x000000bd,0x0000002d,0x00000074,0x000000d0,0x00000012,0x000000b8,0x000000e5,0x000000b4,0x000000b0,
0x00000089,0x00000069,0x00000097,0x0000004a,0x0000000c,0x00000096,0x00000077,0x0000007e,0x00000065,0x000000b9,0x000000f1,0x00000009,0x000000c5,0x0000006e,0x000000c6,0x00000084,
0x00000018,0x000000f0,0x0000007d,0x000000ec,0x0000003a,0x000000dc,0x0000004d,0x00000020,0x00000079,0x000000ee,0x0000005f,0x0000003e,0x000000d7,0x000000cb,0x00000039,0x00000048
};
static const u32 S2[256]=
{
0x0000d600,0x00009000,0x0000e900,0x0000fe00,0x0000cc00,0x0000e100,0x00003d00,0x0000b700,0x00001600,0x0000b600,0x00001400,0x0000c200,0x00002800,0x0000fb00,0x00002c00,0x00000500,
0x00002b00,0x00006700,0x00009a00,0x00007600,0x00002a00,0x0000be00,0x00000400,0x0000c300,0x0000aa00,0x00004400,0x00001300,0x00002600,0x00004900,0x00008600,0x00000600,0x00009900,
0x00009c00,0x00004200,0x00005000,0x0000f400,0x00009100,0x0000ef00,0x00009800,0x00007a00,0x00003300,0x00005400,0x00000b00,0x00004300,0x0000ed00,0x0000cf00,0x0000ac00,0x00006200,
0x0000e400,0x0000b300,0x00001c00,0x0000a900,0x0000c900,0x00000800,0x0000e800,0x00009500,0x00008000,0x0000df00,0x00009400,0x0000fa00,0x00007500,0x00008f00,0x00003f00,0x0000a600,
0x00004700,0x00000700,0x0000a700,0x0000fc00,0x0000f300,0x00007300,0x00001700,0x0000ba00,0x00008300,0x00005900,0x00003c00,0x00001900,0x0000e600,0x00008500,0x00004f00,0x0000a800,
0x00006800,0x00006b00,0x00008100,0x0000b200,0x00007100,0x00006400,0x0000da00,0x00008b00,0x0000f800,0x0000eb00,0x00000f00,0x00004b00,0x00007000,0x00005600,0x00009d00,0x00003500,
0x00001e00,0x00002400,0x00000e00,0x00005e00,0x00006300,0x00005800,0x0000d100,0x0000a200,0x00002500,0x00002200,0x00007c00,0x00003b00,0x00000100,0x00002100,0x00007800,0x00008700,
0x0000d400,0x00000000,0x00004600,0x00005700,0x00009f00,0x0000d300,0x00002700,0x00005200,0x00004c00,0x00003600,0x00000200,0x0000e700,0x0000a000,0x0000c400,0x0000c800,0x00009e00,
0x0000ea00,0x0000bf00,0x00008a00,0x0000d200,0x00004000,0x0000c700,0x00003800,0x0000b500,0x0000a300,0x0000f700,0x0000f200,0x0000ce00,0x0000f900,0x00006100,0x00001500,0x0000a100,
0x0000e000,0x0000ae00,0x00005d00,0x0000a400,0x00009b00,0x00003400,0x00001a00,0x00005500,0x0000ad00,0x00009300,0x00003200,0x00003000,0x0000f500,0x00008c00,0x0000b100,0x0000e300,
0x00001d00,0x0000f600,0x0000e200,0x00002e00,0x00008200,0x00006600,0x0000ca00,0x00006000,0x0000c000,0x00002900,0x00002300,0x0000ab00,0x00000d00,0x00005300,0x00004e00,0x00006f00,
0x0000d500,0x0000db00,0x00003700,0x00004500,0x0000de00,0x0000fd00,0x00008e00,0x00002f00,0x00000300,0x0000ff00,0x00006a00,0x00007200,0x00006d00,0x00006c00,0x00005b00,0x00005100,
0x00008d00,0x00001b00,0x0000af00,0x00009200,0x0000bb00,0x0000dd00,0x0000bc00,0x00007f00,0x00001100,0x0000d900,0x00005c00,0x00004100,0x00001f00,0x00001000,0x00005a00,0x0000d800,
0x00000a00,0x0000c100,0x00003100,0x00008800,0x0000a500,0x0000cd00,0x00007b00,0x0000bd00,0x00002d00,0x00007400,0x0000d000,0x00001200,0x0000b800,0x0000e500,0x0000b400,0x0000b000,
0x00008900,0x00006900,0x00009700,0x00004a00,0x00000c00,0x00009600,0x00007700,0x00007e00,0x00006500,0x0000b900,0x0000f100,0x00000900,0x0000c500,0x00006e00,0x0000c600,0x00008400,
0x00001800,0x0000f000,0x00007d00,0x0000ec00,0x00003a00,0x0000dc00,0x00004d00,0x00002000,0x00007900,0x0000ee00,0x00005f00,0x00003e00,0x0000d700,0x0000cb00,0x00003900,0x00004800
};
static const u32 S3[256]=
{
0x00d60000,0x00900000,0x00e90000,0x00fe0000,0x00cc0000,0x00e10000,0x003d0000,0x00b70000,0x00160000,0x00b60000,0x00140000,0x00c20000,0x00280000,0x00fb0000,0x002c0000,0x00050000,
0x002b0000,0x00670000,0x009a0000,0x00760000,0x002a0000,0x00be0000,0x00040000,0x00c30000,0x00aa0000,0x00440000,0x00130000,0x00260000,0x00490000,0x00860000,0x00060000,0x00990000,
0x009c0000,0x00420000,0x00500000,0x00f40000,0x00910000,0x00ef0000,0x00980000,0x007a0000,0x00330000,0x00540000,0x000b0000,0x00430000,0x00ed0000,0x00cf0000,0x00ac0000,0x00620000,
0x00e40000,0x00b30000,0x001c0000,0x00a90000,0x00c90000,0x00080000,0x00e80000,0x00950000,0x00800000,0x00df0000,0x00940000,0x00fa0000,0x00750000,0x008f0000,0x003f0000,0x00a60000,
0x00470000,0x00070000,0x00a70000,0x00fc0000,0x00f30000,0x00730000,0x00170000,0x00ba0000,0x00830000,0x00590000,0x003c0000,0x00190000,0x00e60000,0x00850000,0x004f0000,0x00a80000,
0x00680000,0x006b0000,0x00810000,0x00b20000,0x00710000,0x00640000,0x00da0000,0x008b0000,0x00f80000,0x00eb0000,0x000f0000,0x004b0000,0x00700000,0x00560000,0x009d0000,0x00350000,
0x001e0000,0x00240000,0x000e0000,0x005e0000,0x00630000,0x00580000,0x00d10000,0x00a20000,0x00250000,0x00220000,0x007c0000,0x003b0000,0x00010000,0x00210000,0x00780000,0x00870000,
0x00d40000,0x00000000,0x00460000,0x00570000,0x009f0000,0x00d30000,0x00270000,0x00520000,0x004c0000,0x00360000,0x00020000,0x00e70000,0x00a00000,0x00c40000,0x00c80000,0x009e0000,
0x00ea0000,0x00bf0000,0x008a0000,0x00d20000,0x00400000,0x00c70000,0x00380000,0x00b50000,0x00a30000,0x00f70000,0x00f20000,0x00ce0000,0x00f90000,0x00610000,0x00150000,0x00a10000,
0x00e00000,0x00ae0000,0x005d0000,0x00a40000,0x009b0000,0x00340000,0x001a0000,0x00550000,0x00ad0000,0x00930000,0x00320000,0x00300000,0x00f50000,0x008c0000,0x00b10000,0x00e30000,
0x001d0000,0x00f60000,0x00e20000,0x002e0000,0x00820000,0x00660000,0x00ca0000,0x00600000,0x00c00000,0x00290000,0x00230000,0x00ab0000,0x000d0000,0x00530000,0x004e0000,0x006f0000,
0x00d50000,0x00db0000,0x00370000,0x00450000,0x00de0000,0x00fd0000,0x008e0000,0x002f0000,0x00030000,0x00ff0000,0x006a0000,0x00720000,0x006d0000,0x006c0000,0x005b0000,0x00510000,
0x008d0000,0x001b0000,0x00af0000,0x00920000,0x00bb0000,0x00dd0000,0x00bc0000,0x007f0000,0x00110000,0x00d90000,0x005c0000,0x00410000,0x001f0000,0x00100000,0x005a0000,0x00d80000,
0x000a0000,0x00c10000,0x00310000,0x00880000,0x00a50000,0x00cd0000,0x007b0000,0x00bd0000,0x002d0000,0x00740000,0x00d00000,0x00120000,0x00b80000,0x00e50000,0x00b40000,0x00b00000,
0x00890000,0x00690000,0x00970000,0x004a0000,0x000c0000,0x00960000,0x00770000,0x007e0000,0x00650000,0x00b90000,0x00f10000,0x00090000,0x00c50000,0x006e0000,0x00c60000,0x00840000,
0x00180000,0x00f00000,0x007d0000,0x00ec0000,0x003a0000,0x00dc0000,0x004d0000,0x00200000,0x00790000,0x00ee0000,0x005f0000,0x003e0000,0x00d70000,0x00cb0000,0x00390000,0x00480000
};
static const u32 S4[256]=
{
0xd6000000,0x90000000,0xe9000000,0xfe000000,0xcc000000,0xe1000000,0x3d000000,0xb7000000,0x16000000,0xb6000000,0x14000000,0xc2000000,0x28000000,0xfb000000,0x2c000000,0x05000000,
0x2b000000,0x67000000,0x9a000000,0x76000000,0x2a000000,0xbe000000,0x04000000,0xc3000000,0xaa000000,0x44000000,0x13000000,0x26000000,0x49000000,0x86000000,0x06000000,0x99000000,
0x9c000000,0x42000000,0x50000000,0xf4000000,0x91000000,0xef000000,0x98000000,0x7a000000,0x33000000,0x54000000,0x0b000000,0x43000000,0xed000000,0xcf000000,0xac000000,0x62000000,
0xe4000000,0xb3000000,0x1c000000,0xa9000000,0xc9000000,0x08000000,0xe8000000,0x95000000,0x80000000,0xdf000000,0x94000000,0xfa000000,0x75000000,0x8f000000,0x3f000000,0xa6000000,
0x47000000,0x07000000,0xa7000000,0xfc000000,0xf3000000,0x73000000,0x17000000,0xba000000,0x83000000,0x59000000,0x3c000000,0x19000000,0xe6000000,0x85000000,0x4f000000,0xa8000000,
0x68000000,0x6b000000,0x81000000,0xb2000000,0x71000000,0x64000000,0xda000000,0x8b000000,0xf8000000,0xeb000000,0x0f000000,0x4b000000,0x70000000,0x56000000,0x9d000000,0x35000000,
0x1e000000,0x24000000,0x0e000000,0x5e000000,0x63000000,0x58000000,0xd1000000,0xa2000000,0x25000000,0x22000000,0x7c000000,0x3b000000,0x01000000,0x21000000,0x78000000,0x87000000,
0xd4000000,0x00000000,0x46000000,0x57000000,0x9f000000,0xd3000000,0x27000000,0x52000000,0x4c000000,0x36000000,0x02000000,0xe7000000,0xa0000000,0xc4000000,0xc8000000,0x9e000000,
0xea000000,0xbf000000,0x8a000000,0xd2000000,0x40000000,0xc7000000,0x38000000,0xb5000000,0xa3000000,0xf7000000,0xf2000000,0xce000000,0xf9000000,0x61000000,0x15000000,0xa1000000,
0xe0000000,0xae000000,0x5d000000,0xa4000000,0x9b000000,0x34000000,0x1a000000,0x55000000,0xad000000,0x93000000,0x32000000,0x30000000,0xf5000000,0x8c000000,0xb1000000,0xe3000000,
0x1d000000,0xf6000000,0xe2000000,0x2e000000,0x82000000,0x66000000,0xca000000,0x60000000,0xc0000000,0x29000000,0x23000000,0xab000000,0x0d000000,0x53000000,0x4e000000,0x6f000000,
0xd5000000,0xdb000000,0x37000000,0x45000000,0xde000000,0xfd000000,0x8e000000,0x2f000000,0x03000000,0xff000000,0x6a000000,0x72000000,0x6d000000,0x6c000000,0x5b000000,0x51000000,
0x8d000000,0x1b000000,0xaf000000,0x92000000,0xbb000000,0xdd000000,0xbc000000,0x7f000000,0x11000000,0xd9000000,0x5c000000,0x41000000,0x1f000000,0x10000000,0x5a000000,0xd8000000,
0x0a000000,0xc1000000,0x31000000,0x88000000,0xa5000000,0xcd000000,0x7b000000,0xbd000000,0x2d000000,0x74000000,0xd0000000,0x12000000,0xb8000000,0xe5000000,0xb4000000,0xb0000000,
0x89000000,0x69000000,0x97000000,0x4a000000,0x0c000000,0x96000000,0x77000000,0x7e000000,0x65000000,0xb9000000,0xf1000000,0x09000000,0xc5000000,0x6e000000,0xc6000000,0x84000000,
0x18000000,0xf0000000,0x7d000000,0xec000000,0x3a000000,0xdc000000,0x4d000000,0x20000000,0x79000000,0xee000000,0x5f000000,0x3e000000,0xd7000000,0xcb000000,0x39000000,0x48000000
};
/* System parameter */
static const u32 FK[4] = {0xa3b1bac6,0x56aa3350,0x677d9197,0xb27022dc};

/* fixed parameter */
static const u32 CK[32] =
{
0x00070e15,0x1c232a31,0x383f464d,0x545b6269,
0x70777e85,0x8c939aa1,0xa8afb6bd,0xc4cbd2d9,
0xe0e7eef5,0xfc030a11,0x181f262d,0x343b4249,
0x50575e65,0x6c737a81,0x888f969d,0xa4abb2b9,
0xc0c7ced5,0xdce3eaf1,0xf8ff060d,0x141b2229,
0x30373e45,0x4c535a61,0x686f767d,0x848b9299,
0xa0a7aeb5,0xbcc3cad1,0xd8dfe6ed,0xf4fb0209,
0x10171e25,0x2c333a41,0x484f565d,0x646b7279
};

/* private function:
 * Calculating round encryption key.
 * args:    [in] a: a is a 32 bits unsigned value;
 * return: sk[i]: i{0,1,2,3,...31}.
 */
static u32 sm4CalciRK(u32 ka)
{
    u32 bb = 0;
    u32 rk = 0;
    bb =  S1[(ka    ) & 0xff] 
            ^ S2[(ka>> 8) & 0xff] 
            ^ S3[(ka>>16) & 0xff] 
            ^ S4[(ka>>24) & 0xff];
    rk = bb^(ROTL(bb, 13))^(ROTL(bb, 23));
    return rk;
}

static void sm4_setkey( u32 SK[32], const u8 key[16] )
{
    u32 MK[4];
    u32 k[36];
    u32 i = 0;

    GET_ULONG_BE( MK[0], key, 0 );
    GET_ULONG_BE( MK[1], key, 4 );
    GET_ULONG_BE( MK[2], key, 8 );
    GET_ULONG_BE( MK[3], key, 12 );
    k[0] = MK[0]^FK[0];
    k[1] = MK[1]^FK[1];
    k[2] = MK[2]^FK[2];
    k[3] = MK[3]^FK[3];
    for(; i<32; i++)
    {
        k[i+4] = k[i] ^ (sm4CalciRK(k[i+1]^k[i+2]^k[i+3]^CK[i]));
        SK[i] = k[i+4];
	}

}

/*
 * SM4 key schedule (128-bit, encryption)
 */
void sm4_setkey_enc( sm4_context *ctx, const u8 key[16] )
{
    ctx->mode = SM4_ENCRYPT;
	sm4_setkey( ctx->sk, key );
}

/*
 * SM4 key schedule (128-bit, decryption)
 */
void sm4_setkey_dec( sm4_context *ctx, const u8 key[16] )
{
    int i;
	ctx->mode = SM4_DECRYPT;
    sm4_setkey( ctx->sk, key );
    for( i = 0; i < 16; i ++ )
    {
        SWAP( ctx->sk[ i ], ctx->sk[ 31-i] );
    }
}


/*
 * SM4-ECB block encryption/decryption
 */
void sm4_crypt( sm4_context *ctx,
                const u8 input[SM4_BLOCK_SIZE],
                u8 output[SM4_BLOCK_SIZE])
{
    u32 i = 0;
    u32 bb = 0;
    u32 ka = 0;
    u32 t0,t1,t2,t3,t4;

    GET_ULONG_BE( t0, input, 0 )
    GET_ULONG_BE( t1, input, 4 )
    GET_ULONG_BE( t2, input, 8 )
    GET_ULONG_BE( t3, input, 12 )
    
    while(i<32)
    {
        ka = t1^t2^t3^(ctx->sk[i]);
        bb =  S1[(ka    ) & 0xff] 
            ^ S2[(ka>> 8) & 0xff] 
            ^ S3[(ka>>16) & 0xff] 
            ^ S4[(ka>>24) & 0xff];
        
        //t0 = t0^bb^ROTL(bb,2)^ROTL(bb,10)^ROTL(bb,18)^ROTL(bb,24)
        asm("xor %1,%0":"+r"(t0):"r"(bb));
        asm("rol %1,%0":"+r"(bb):"I"(2));
        asm("xor %1,%0":"+r"(t0):"r"(bb));
        asm("rol %1,%0":"+r"(bb):"I"(8));
        asm("xor %1,%0":"+r"(t0):"r"(bb));
        asm("rol %1,%0":"+r"(bb):"I"(8));
        asm("xor %1,%0":"+r"(t0):"r"(bb));
        asm("rol %1,%0":"+r"(bb):"I"(6));
        asm("xor %1,%0":"+r"(t0):"r"(bb));

        t4 = t0;t0 = t1;
        t1 = t2;t2 = t3;
        t3 = t4;
	    ++i;
    }
    PUT_ULONG_BE(t3,output,0);
	PUT_ULONG_BE(t2,output,4);
	PUT_ULONG_BE(t1,output,8);
	PUT_ULONG_BE(t0,output,12);
}
