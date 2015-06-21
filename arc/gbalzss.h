#ifndef _GBALZSS_H_
#define _GBALZSS_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void Encode(FILE *infile, FILE *outfile);
void Decode(FILE *infile, FILE *outfile);

#ifdef __cplusplus
}
#endif

#endif // _GBALZSS_H_
