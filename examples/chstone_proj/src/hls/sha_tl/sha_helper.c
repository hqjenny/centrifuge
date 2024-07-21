/*
+--------------------------------------------------------------------------+
| CHStone : a suite of benchmark programs for C-based High-Level Synthesis |
| ======================================================================== |
|                                                                          |
| * Collected and Modified : Y. Hara, H. Tomiyama, S. Honda,               |
|                            H. Takada and K. Ishii                        |
|                            Nagoya University, Japan                      |
|                                                                          |
| * Remark :                                                               |
|    1. This source code is modified to unify the formats of the benchmark |
|       programs in CHStone.                                               |
|    2. Test vectors are added for CHStone.                                |
|    3. If "main_result" is 0 at the end of the program, the program is    |
|       correctly executed.                                                |
|    4. Please follow the copyright of each benchmark program.             |
+--------------------------------------------------------------------------+
*/
/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach uh@alumni.caltech edu */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */

/* NIST's proposed modification to SHA of 7/11/94 may be */
/* activated by defining USE_MODIFIED_SHA */


/* SHA f()-functions */

#define f1(x,y,z)	((x & y) | (~x & z))
#define f2(x,y,z)	(x ^ y ^ z)
#define f3(x,y,z)	((x & y) | (x & z) | (y & z))
#define f4(x,y,z)	(x ^ y ^ z)

/* SHA constants */

#define CONST1		0x5a827999L
#define CONST2		0x6ed9eba1L
#define CONST3		0x8f1bbcdcL
#define CONST4		0xca62c1d6L

/* 32-bit rotate */

#define ROT32(x,n)	((x << n) | (x >> (32 - n)))

#define FUNC(n,i)						\
    temp = ROT32(A,5) + f##n(B,C,D) + E + W[i] + CONST##n;	\
    E = D; D = C; C = ROT32(B,30); B = A; A = temp

void
local_memset (INT32 * s, int c, int n, int e)
{
  INT32 uc;
  INT32 *p;
  int m;

  m = n / 4;
  uc = c;
  p = (INT32 *) s;
  while (e-- > 0)
    {
      p++;
    }
  while (m-- > 0)
    {
      *p++ = uc;
    }
}

void
local_memcpy (INT32 * s1, const BYTE * s2, int n)
{
  INT32 *p1;
  BYTE *p2;
  INT32 tmp;
  int m;
  m = n / 4;
  p1 = (INT32 *) s1;
  p2 = (BYTE *) s2;

  while (m-- > 0)
    {
      tmp = 0;
      tmp |= 0xFF & *p2++;
      tmp |= (0xFF & *p2++) << 8;
      tmp |= (0xFF & *p2++) << 16;
      tmp |= (0xFF & *p2++) << 24;
      *p1 = tmp;
      p1++;
    }
}

/* do SHA transformation */

static void
sha_transform (INT32* sha_info_digest, INT32* sha_info_data)
{
  int i;
  INT32 temp, A, B, C, D, E, W[80];

  for (i = 0; i < 16; ++i)
    {
      W[i] = sha_info_data[i];
    }
  for (i = 16; i < 80; ++i)
    {
      W[i] = W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16];
    }
  A = sha_info_digest[0];
  B = sha_info_digest[1];
  C = sha_info_digest[2];
  D = sha_info_digest[3];
  E = sha_info_digest[4];

  for (i = 0; i < 20; ++i)
    {
      FUNC (1, i);
    }
  for (i = 20; i < 40; ++i)
    {
      FUNC (2, i);
    }
  for (i = 40; i < 60; ++i)
    {
      FUNC (3, i);
    }
  for (i = 60; i < 80; ++i)
    {
      FUNC (4, i);
    }

  sha_info_digest[0] += A;
  sha_info_digest[1] += B;
  sha_info_digest[2] += C;
  sha_info_digest[3] += D;
  sha_info_digest[4] += E;
}

/* initialize the SHA digest */

void
sha_init ()
{
//  sha_info_digest[0] = 0x67452301L;
//  sha_info_digest[1] = 0xefcdab89L;
//  sha_info_digest[2] = 0x98badcfeL;
//  sha_info_digest[3] = 0x10325476L;
//  sha_info_digest[4] = 0xc3d2e1f0L;
//  sha_info_count_lo = 0L;
//  sha_info_count_hi = 0L;
}

void
sha_update (const BYTE * buffer, int count, INT32* sha_info_digest, INT32* sha_info_data, INT32* sha_info_count)
{
#pragma HLS INTERFACE m_axi port=buffer offset=slave bundle=gmem0 // Direct is for AXI with full 32 bit address space
#pragma HLS INTERFACE m_axi port=sha_info_digest offset=slave bundle=gmem0 // Slave is for AXI4Lite, with burst mode disabled
#pragma HLS INTERFACE m_axi port=sha_info_data offset=slave bundle=gmem0 // Slave is for AXI4Lite, with burst mode disabled
#pragma HLS INTERFACE m_axi port=sha_info_count offset=slave bundle=gmem0 // Slave is for AXI4Lite, with burst mode disabled

#pragma HLS INTERFACE s_axilite port=buffer bundle=control
#pragma HLS INTERFACE s_axilite port=count bundle=control
#pragma HLS INTERFACE s_axilite port=sha_info_digest bundle=control
#pragma HLS INTERFACE s_axilite port=sha_info_data bundle=control
#pragma HLS INTERFACE s_axilite port=sha_info_count bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

  INT32 sha_info_count_local0 = sha_info_count[0];
  INT32 sha_info_count_local1 = sha_info_count[1];
  if ((sha_info_count_local0 + ((INT32) count << 3)) < sha_info_count_local0)
    {
      ++sha_info_count_local1;
    }
  sha_info_count_local0 += (INT32) count << 3;
  sha_info_count_local1 += (INT32) count >> 29;
  sha_info_count[0] = sha_info_count_local0;
  sha_info_count[1] = sha_info_count_local1;
  while (count >= SHA_BLOCKSIZE)
    {
      local_memcpy (sha_info_data, buffer, SHA_BLOCKSIZE);
      sha_transform (sha_info_digest, sha_info_data);
      buffer += SHA_BLOCKSIZE;
      count -= SHA_BLOCKSIZE;
    }
  local_memcpy (sha_info_data, buffer, count);
}

/* finish computing the SHA digest */

void sha_final (INT32* sha_info_digest, INT32* sha_info_data, INT32* sha_info_count){
  int count;
  INT32 lo_bit_count;
  INT32 hi_bit_count;

  lo_bit_count = sha_info_count[0];
  hi_bit_count = sha_info_count[1];
  count = (int) ((lo_bit_count >> 3) & 0x3f);
  sha_info_data[count++] = 0x80;
  if (count > 56)
    {
      local_memset (sha_info_data, 0, 64 - count, count);
      sha_transform (sha_info_digest, sha_info_data);
      local_memset (sha_info_data, 0, 56, 0);
    }
  else
    {
      local_memset (sha_info_data, 0, 56 - count, count);
    }
  sha_info_data[14] = hi_bit_count;
  sha_info_data[15] = lo_bit_count;
  sha_transform (sha_info_digest, sha_info_data);
}

/* compute the SHA digest of a FILE stream */
void
sha_stream ()
{
  int i, j;
  const BYTE *p;

  INT32 sha_info_data[16];
  sha_info_digest[0] = 0x67452301L;
  sha_info_digest[1] = 0xefcdab89L;
  sha_info_digest[2] = 0x98badcfeL;
  sha_info_digest[3] = 0x10325476L;
  sha_info_digest[4] = 0xc3d2e1f0L;
  //int sha_info_count_lo = 0L;
  //int sha_info_count_hi = 0L;
  int sha_info_count[2]={0L, 0L};

  uint64_t begin, end, dur;

  sha_init ();
  begin = read_cycle();
  for (j = 0; j < VSIZE; j++)
    {
      i = in_i[j];
      p = &indata[j][0];

  #ifdef CUSTOM_DRIVER
    sha_update_wrapper(p, i, sha_info_digest, sha_info_data, sha_info_count);
  #else
    sha_update (p, i, sha_info_digest, sha_info_data, sha_info_count);
  #endif 

    }
  end = read_cycle();
  duration(begin, end);
  sha_final (sha_info_digest, sha_info_data, sha_info_count);
}
