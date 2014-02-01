#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "endianess.h"
#include "types.h"
#include "stringtable.h"

typedef struct {
	u32		length;
} STRINGTABLE_HEADER;

typedef struct {
	u32		name;
	u32		offset;
	u16		length;
	u8		unknown;
	u8		category;
} STRINGTABLE_ENTRY;

STRINGTABLE* STRINGTABLE_read(const char* filename) {
	unsigned int i;
	STRINGTABLE_HEADER header;
	STRINGTABLE_ENTRY entry;
	u32* offsets;
	STRINGTABLE* out = (STRINGTABLE*)malloc(sizeof(STRINGTABLE));
	if(!out)
		return NULL;
	FILE* in = fopen(filename, "rb");
	if(in == NULL)
		return NULL;
	fread(&header, sizeof(header), 1, in);

	out->length = get32bit_LE((u8*) &header.length);
	out->strings = (STRING*) malloc(out->length * sizeof(STRING));
	if(!out->strings)
		return NULL;

	offsets = (u32*) malloc(out->length * sizeof(u32));
	if(!offsets)
		return NULL;

	if(out->length > 200) {
		fread(&header, 4, 1, in); // read header length
	}

	for(i = 0; i < out->length; i++) {
		int n = fread(&entry, sizeof(STRINGTABLE_ENTRY), 1, in);
		out->strings[i].string = NULL;
		out->strings[i].length = get16bit_LE((u8*) &entry.length);
		out->strings[i].unknown = entry.unknown;
		out->strings[i].category = entry.category;
		out->strings[i].name = get32bit_LE((u8*) &entry.name);
		offsets[i] = get32bit_LE((u8*) &entry.offset);
	}

	for(i = 0; i < out->length; i++) {
		fseek(in, offsets[i], SEEK_SET);
		out->strings[i].string = (char*)
				malloc((out->strings[i].length + 1) *
						sizeof(char));
		fread(out->strings[i].string, out->strings[i].length + 1, 1,
				in);
	}
	free(offsets);
	fclose(in);

	return out;
}

unsigned int STRINGTABLE_id(unsigned char* s) {
	if(strlen(s) != 4)
		return 0;
	return s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
}

char* STRINGTABLE_find(STRINGTABLE* table, unsigned int string) {
	unsigned int i;
	for(i = 0; i < table->length; i++) {
		if(table->strings[i].name == string) {
			return table->strings[i].string;
		}
	}
	return NULL;
}

void STRINGTABLE_free(STRINGTABLE* table) {
	unsigned int i;
	for(i = 0; i < table->length; i++) {
		free(table->strings[i].string);
	}
	free(table->strings);
	free(table);
}
