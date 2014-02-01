#ifndef __STRINGS_H__
#define __STRINGS_H__

typedef struct {
	unsigned int	length;
	unsigned int	name;
	unsigned int	unknown;
	char		category;
	char*		string;
} STRING;

typedef struct {
	unsigned int	length;
	STRING*		strings;
} STRINGTABLE;

STRINGTABLE*	STRINGTABLE_read(const char* filename);
unsigned int	STRINGTABLE_id(unsigned char* s);
char*		STRINGTABLE_find(STRINGTABLE* table, unsigned int string);
void		STRINGTABLE_free(STRINGTABLE* table);

#endif
