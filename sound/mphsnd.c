#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "types.h"
#include "endianess.h"
#include "adpcm.h"
#include "fileformat.h"

#define BLOCKSIZE	1

int main(int argc, char** argv) {
	FILE* in_file;
	FILE* out_file;
	int i;
	u32 sample_count;
	int sample_index;
	int step_index;
	int start_sample;
	ADPCMInfo adpcm;
	u32* offsets;
	unsigned long start_offset;
	unsigned long bytes_read = 0;
	unsigned long total_bytes_read;
	byte* buffer;
	s16* out_buffer;
	unsigned long in_file_size;
	SAMPLE_HEADER smp_header;

	if(argc != 4) {
		printf("usage: %s infile.bin outfile.raw sample_index\n", argv[0]);
		return -1;
	}

	if(strcmp(argv[1], "-") == 0)
		in_file = stdin;
	else
		in_file = fopen(argv[1], "rb");

	if(in_file == 0) {
		printf("%s: Can't open input file!\n", argv[0]);
		return -1;
	}

	if(strcmp(argv[2], "-") == 0)
		in_file = stdout;
	else
		out_file = fopen(argv[2], "wb");

	if(out_file == 0) {
		printf("%s: Can't open output file!\n", argv[0]);
		return -1;
	}

	sscanf(argv[3], "%d", &sample_index);

	fseek(in_file, 0, SEEK_END);
	in_file_size = ftell(in_file);
	fseek(in_file, 0, SEEK_SET);

	fread(&sample_count, 4, 1, in_file);
	sample_count = get32bit_LE((u8*) &sample_count);

	// read offset table
	offsets = (u32*) malloc(sample_count * 4);
	fread(offsets, 4, sample_count, in_file);

	start_offset = get32bit_LE((u8*) &offsets[sample_index]);
	fseek(in_file, start_offset, SEEK_SET);
	fread(&smp_header, sizeof(SAMPLE_HEADER), 1, in_file);
	convert_header(&smp_header);

	step_index = smp_header.init_step_index;
	start_sample = (s16)smp_header.init_sample;

	init_adpcm(&adpcm, step_index, start_sample);

	printf("%s: sample rate = %d Hz\n", argv[0], smp_header.sample_rate);

	buffer = (byte*) malloc(BLOCKSIZE);
	out_buffer = (s16*) malloc(BLOCKSIZE * 2 * sizeof(s16));

	fseek(in_file, start_offset + sizeof(SAMPLE_HEADER), SEEK_SET);

	total_bytes_read = 0;

	while(total_bytes_read < smp_header.samples) {
		bytes_read = fread(buffer, 1, BLOCKSIZE, in_file);
		decode_adpcm(&adpcm, buffer, out_buffer, bytes_read);
		fwrite(out_buffer, bytes_read * 2, 2, out_file);
		total_bytes_read += bytes_read;
	}

	free(offsets);
	free(buffer);
	free(out_buffer);

	fclose(in_file);
	fclose(out_file);

	return 0;
}
