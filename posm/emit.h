/* emit.h -- this file contains utility for compiler to emit/write opcode or bits */

#ifndef POCOL_BYTES_H
#define POCOL_BYTES_H

#include <stdio.h>
#include <stdint.h>

#include "../common.h"

/* Serialize 64-bit value into 8 bytes with Little-Endian order */
/* stackoverflow 69968061 */
ST_INLN void emit64(FILE *out, uint64_t val)
{
	uint8_t bytes[8];
	for (int i = 0; i<8; i++)
		bytes[i] = (val >> (i * 8)) & 0xFF;

	fwrite(bytes, 1, 8, out);
}

#endif
