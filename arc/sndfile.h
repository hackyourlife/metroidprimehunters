// all values are BIG ENDIAN!!!

#ifndef __SNDFILE_H__
#define __SNDFILE_H__

#include "endianess.h"

typedef struct _SNDFILE_HEADER {
	u8	magic[7];	// 'SNDFILE'
	u32	unknown1;
	u8	entry_count;
	u32	file_size;
	u8	unknown2[16];
}  __attribute__ ((__packed__)) SNDFILE_HEADER;

typedef struct _SNDFILE_ENTRY {
	u8	name[32];	// name, '\0'-padded
	u32	offset;		// offset of file in the image (including header)
	u32	file_length;
	u32	unknown1;	// 1. entry: file_length - 0x0C
	u8	unknown[20];
}  __attribute__ ((__packed__)) SNDFILE_ENTRY;

// routine to correct the header (LITTLE -> BIG ENDIAN)
void SNDFILE_correct_file_header(SNDFILE_HEADER* header) {
	header->unknown1 = get_32bitBE((u8*) &(header->unknown1));
	header->file_size = get_32bitBE((u8*) &(header->file_size));
}

void SNDFILE_correct_entry(SNDFILE_ENTRY* entry) {
	entry->offset = get_32bitBE((u8*) &(entry->offset));
	entry->file_length = get_32bitBE((u8*) &(entry->file_length));
}

#endif
