#include "types.h"

/*
Struktur:

u32	audio_count;
u32	offsets[audio_count];

offset:
u32	data_length;
u32	zero;
u32	sample_rate;
u32	unknown1;
u32	unknown2;
u32	unknown3;

u16	init_sample;
u8	init_step_index;
u8	zero;

*/

typedef struct {
	u32	samples;
	u32	zero1;
	u32	sample_rate;
	u32	unknown1;
	u32	unknown2;
	u32	unknown3;
	u16	init_sample;
	u8	init_step_index;
	u8	zero2;
} SAMPLE_HEADER;

static void convert_header(SAMPLE_HEADER* hdr) {
	hdr->samples = get32bit_LE((u8*) &hdr->samples);
	hdr->sample_rate = get32bit_LE((u8*) &hdr->sample_rate);
	hdr->init_sample = get16bit_LE((u8*) &hdr->init_sample);
}
