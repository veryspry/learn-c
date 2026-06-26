#include <stddef.h>
#include <stdio.h>

#ifndef LINES_H 
#define LINES_H 

void stream_to_upper(FILE *ifp, FILE *ofp);

void truncate_lines_hard(FILE *ifp, FILE *ofp);

#endif
