// decodes NDS ADPCM data
#ifndef __NDS_ADPCM_H__
#define __NDS_ADPCM_H__

#include "types.h"

#if !defined(bool) && !defined(__cplusplus)
	typedef unsigned char bool;
	#define true	1
	#define false	0
#endif /* !bool */

#ifndef byte
	typedef unsigned char byte;
#endif /* !byte */
#ifndef sbyte
	typedef signed char sbyte;
#endif /* !sbyte */

typedef struct {
	bool	low;
	int	samp;
	int	stepIndex;
} ADPCMInfo;

//bool nsSampDecodeBlock(byte* dest, const byte* blocks, size_t blockSize, int nSamples, int waveType, int channels);

static ADPCMInfo* init_adpcm(ADPCMInfo* adpcm, int step_index, int sample)
{
	adpcm->low		= true;
	adpcm->samp		= sample;
	adpcm->stepIndex	= step_index;
	return adpcm;
}

static void clamp_step_index(int* stepIndex)
{
	if(*stepIndex < 0 ) *stepIndex = 0;
	if(*stepIndex > 88) *stepIndex = 88;
}

static void clamp_sample(int* decompSample)
{
	if (*decompSample < -32768) *decompSample = -32768;
	if (*decompSample >  32767) *decompSample =  32767;
}

static void process_nibble(unsigned char code, int* stepIndex, int* decompSample)
{
	/*
	const unsigned ADPCMTable[89] = {
		7, 8, 9, 10, 11, 12, 13, 14,
		16, 17, 19, 21, 23, 25, 28, 31,
		34, 37, 41, 45, 50, 55, 60, 66,
		73, 80, 88, 97, 107, 118, 130, 143,
		157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658,
		724, 796, 876, 963, 1060, 1166, 1282, 1411,
		1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
		3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
		7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
		32767
	};
	*/

	const unsigned ADPCMTable[89] = {
		    7,     8,     9,    10,    11,    12,    13,    14,
		   16,    17,    19,    21,    23,    25,    28,    31,
		   34,    37,    41,    45,    50,    55,    60,    66,
		   73,    80,    88,    97,   107,   118,   130,   143,
		  157,   173,   190,   209,   230,   253,   279,   307,
		  337,   371,   408,   449,   494,   544,   598,   658,
		  724,   796,   876,   963,  1060,  1166,  1282,  1411,
		 1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
		 3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
		 7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
		32767
	};

	const int IMA_IndexTable[16] = {
		-1, -1, -1, -1, 2, 4, 6, 8,
		-1, -1, -1, -1, 2, 4, 6, 8
	};

	unsigned step;
	int diff;

	code &= 0x0F;

	step = ADPCMTable[*stepIndex];
	diff = step >> 3;
	if (code & 1) diff += step >> 2;
	if (code & 2) diff += step >> 1;
	if (code & 4) diff += step;
	// Windows:
	//if (code & 8) *decompSample -= diff;
	//else          *decompSample += diff;
	//if (*decompSample < -32768) *decompSample = -32768;
	//if (*decompSample >  32767) *decompSample = 32767;
	// Nitro: (has minor clipping-error, see GBATEK for details)
	if (code & 8) {
		*decompSample -= diff;
		if (*decompSample < -32767)
			*decompSample = -32767;
	} else {
		*decompSample += diff;
		if (*decompSample > 32767)
			*decompSample = 32767;
	}
	(*stepIndex) += IMA_IndexTable[code];
	clamp_step_index(stepIndex);
}


static unsigned long decode_adpcm(ADPCMInfo* adpcm, const byte* in, s16* out, unsigned long bytes_to_read)
{
	unsigned char code;
	unsigned long index = 0;
	while(index < bytes_to_read) {
		code = in[index];
		if(!adpcm->low) {
			code = code >> 4;
			index++;
		}
		process_nibble(code, &adpcm->stepIndex, &adpcm->samp);
		*out++ = adpcm->samp;
		adpcm->low = !adpcm->low;
	}
	return bytes_to_read;
}

static unsigned long __decode_adpcm(ADPCMInfo* adpcm, const byte* in, void (*write_handler)(ADPCMInfo*, int), unsigned long bytes_to_read)
{
	unsigned char code;
	unsigned long index = 0;
	while(index < bytes_to_read) {
		code = in[index];
		if(!adpcm->low) {
			code = code >> 4;
			index++;
		}
		process_nibble(code, &adpcm->stepIndex, &adpcm->samp);
		write_handler(adpcm, adpcm->samp);
		adpcm->low = !adpcm->low;
	}
	return bytes_to_read;
}

#endif
