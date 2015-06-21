// Modified on 03.11.2009
// sndfileextractor by Daniel Pekarek
// to extract data from MetroidPrimeHunters's archives in /archives/ folder
// ATTENTION: these files are compressed, and must be decompressed before use with this tool (LZSS)
// I call it sndfileextractor because the magic is 'SNDFILE'

// sndfileextractor.c, sndfile.h, endianess.h by Daniel Pekarek
// gbalzss.* by and Haruhiko Okumura and Andre Perrot


/**************************************************************
	sndfileextractor.c -- An Archiver Program
	(tab = 8 spaces)
***************************************************************
	Use, distribute, and modify this program freely.
**************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#define u8	uint8_t
#define u16	uint16_t
#define u32	uint32_t
#define u64	uint64_t
#define s8	int8_t
#define s16	int16_t
#define s32	int32_t
#define s64	int64_t

#include "gbalzss.h"
#include "endianess.h"
#include "sndfile.h"

long i;

static inline long min(long a, long b) {
	return a < b ? a : b;
}

void printUsage(char* tool_name) {
	printf("usage: %s [-d] <infile> [outdir]\n", tool_name);
}

void dumpFile(const char * name, FILE* src, u32 offset, u32 length)
{
	int t = ftell(src);
	fseek(src, offset, SEEK_SET);
	
	FILE* out = fopen(name, "wb");
	if(out == NULL)
		return;
	
	u32 read = 0;
	u8 buff[1024];
	while(read < length)
	{
		int r = fread(buff, 1, min(1024u, length - read), src);
		fwrite(buff, 1, r, out);
		read += r;
	}
	
	fclose(out);
	fseek(src, t, SEEK_SET);
}

void extractSNDFILE(FILE* in_file, char* dir_name) {
	char filename[255] = {0};
	char buffer[512];
	long read_data = 0;
	SNDFILE_HEADER header;
	SNDFILE_ENTRY entry;
	fread(&header, sizeof(header), 1, in_file);
	SNDFILE_correct_file_header(&header);
	if(memcmp(&header.magic, "SNDFILE", 7) != 0) {
		printf("not a valid file!\n");
		return;
	}
	printf("file size: %d bytes\n", header.file_size);
	printf("files:     %d\n", header.entry_count);
	for(i = 0; i < header.entry_count; i++) {
		fread(&entry, sizeof(entry), 1, in_file);
		SNDFILE_correct_entry(&entry);
		strcpy(filename, dir_name);
		strcat(filename, (char*) &entry.name);
		printf("%s\n", filename);
		dumpFile((char*) &filename, in_file, entry.offset, entry.file_length);
	}
}

int main(int argc, char* argv[]) {
	puts("sndfileextractor by Daniel Pekarek\n");
	if(argc < 2) {
		printUsage(argv[0]);
		return -1;
	}
	FILE* in_file;
	char dir_name[255] = {0};
	if(strcmp(argv[1], "-d") == 0) {
		FILE * srcFile = fopen(argv[2], "rb");
		FILE * dstFile = fopen(argv[3], "wb");
		if(srcFile == NULL) {
			printf("Error: Couldn't open input file!\n");
			return -1;
		}
		if(dstFile == NULL) {
			printf("Error: Couldn't open output file!\n");
			return -1;
		}

		Decode(srcFile, dstFile);
		fclose(dstFile);
		fclose(srcFile);
		return 0;
	}

	in_file = fopen(argv[1], "rb");
	if(in_file == NULL) {
		perror("Can\'t open input file");
		return -1;
	}

	if(argc == 2) {
		strcat(dir_name, argv[1]);
		strcat(dir_name, "_dir/");
	} else if(argc == 3)
		strcat(dir_name, argv[2]);

	mkdir(dir_name, 0);
	extractSNDFILE(in_file, &dir_name[0]);
	fclose(in_file);
	return 0;
}
