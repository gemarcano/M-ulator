/* Mulator - An extensible {e,si}mulator
 * Copyright 2011-2020 Pat Pannuto <pat.pannuto@gmail.com>
 *
 * Licensed under either of the Apache License, Version 2.0
 * or the MIT license, at your option.
 */

#ifndef __HELPERS_H
#define __HELPERS_H

#include "core/common.h"
#include "core/isa/arm_types.h"

//XXX ?
#include "core/isa/decode_helpers.h"

#define HIGH_BIT(_r) (!!(_r & 0x80000000))

uint32_t reverse_bits(uint32_t val);

void AddWithCarry(uint32_t x, uint32_t y, bool carry_in,
		uint32_t *result, bool *carry_out, bool *overflow_out)
		__attribute__ ((nonnull));

void LoadWritePC(uint32_t addr);
void BXWritePC(uint32_t addr);
void BLXWritePC(uint32_t addr);
void BranchTo(uint32_t addr);
void BranchWritePC(uint32_t addr);

#ifdef M_PROFILE

#ifdef FAVOR_SPEED
#define _CHECK_SET_THUMB_BIT(_t)
#else
#define _CHECK_SET_THUMB_BIT(_t)\
	do {\
		if (! (_t) ) {\
			WARN("Setting Thumb bit False in M series will cause an execption!\n");\
		}\
	} while (0)
#endif

#define SET_THUMB_BIT(_t) \
	do {\
		_CHECK_SET_THUMB_BIT(_t);\
		union epsr_t epsr = CORE_epsr_read();\
		epsr.bits.T = _t;\
		CORE_epsr_write(epsr);\
	} while (0)

inline __attribute__ ((always_inline))
void set_isetstate(uint32_t iset __attribute__ ((unused))) {
	// NOP for M profile
	return;
}

#endif //M_PROFILE

#endif // __HELPERS_H
