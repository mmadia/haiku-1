/*
 * PackBits.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PACKBITS_H
#define __PACKBITS_H

int	pack_bits_size(const unsigned char *in, int bytes);
int	pack_bits(unsigned char *out, const unsigned char *in, int bytes);

#endif	/* __PACKBITS_H */
