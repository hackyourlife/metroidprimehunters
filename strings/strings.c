#include <stdio.h>
#include <string.h>
#include "stringtable.h"

int main(int argc, char** argv) {
	unsigned int i;
	if(argc != 2) {
		printf("usage: %s strings.bin\n", *argv);
		return 1;
	}
	STRINGTABLE* table = STRINGTABLE_read(argv[1]);
	if(!table) {
		printf("error reading file\n");
	}
	printf("%d strings\n", table->length);
	for(i = 0; i < table->length; i++) {
		printf("%03d (%04d/%04d, %c%c%c%c, %c/%d): '%s'\n", i,
				table->strings[i].length,
				strlen(table->strings[i].string),
				(table->strings[i].name >> 24) & 0xFF,
				(table->strings[i].name >> 16) & 0xFF,
				(table->strings[i].name >>  8) & 0xFF,
				table->strings[i].name & 0xFF,
				table->strings[i].category,
				table->strings[i].unknown,
				table->strings[i].string);
	}
	STRINGTABLE_free(table);
	return 0;
}
