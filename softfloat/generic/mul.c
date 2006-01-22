/*
 * Copyright (C) 2005 Josef Cejka
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include<sftypes.h>
#include<mul.h>
#include<comparison.h>

/** Multiply two 32 bit float numbers
 *
 */
float32 mulFloat32(float32 a, float32 b)
{
	float32 result;
	__u64 mant1, mant2;
	__s32 exp;

	result.parts.sign = a.parts.sign ^ b.parts.sign;
	
	if (isFloat32NaN(a) || isFloat32NaN(b) ) {
		/* TODO: fix SigNaNs */
		if (isFloat32SigNaN(a)) {
			result.parts.mantisa = a.parts.mantisa;
			result.parts.exp = a.parts.exp;
			return result;
		};
		if (isFloat32SigNaN(b)) { /* TODO: fix SigNaN */
			result.parts.mantisa = b.parts.mantisa;
			result.parts.exp = b.parts.exp;
			return result;
		};
		/* set NaN as result */
		result.binary = FLOAT32_NAN;
		return result;
	};
		
	if (isFloat32Infinity(a)) { 
		if (isFloat32Zero(b)) {
			/* FIXME: zero * infinity */
			result.binary = FLOAT32_NAN;
			return result;
		}
		result.parts.mantisa = a.parts.mantisa;
		result.parts.exp = a.parts.exp;
		return result;
	}

	if (isFloat32Infinity(b)) { 
		if (isFloat32Zero(a)) {
			/* FIXME: zero * infinity */
			result.binary = FLOAT32_NAN;
			return result;
		}
		result.parts.mantisa = b.parts.mantisa;
		result.parts.exp = b.parts.exp;
		return result;
	}

	/* exp is signed so we can easy detect underflow */
	exp = a.parts.exp + b.parts.exp;
	exp -= FLOAT32_BIAS;
	
	if (exp >= FLOAT32_MAX_EXPONENT) {
		/* FIXME: overflow */
		/* set infinity as result */
		result.binary = FLOAT32_INF;
		result.parts.sign = a.parts.sign ^ b.parts.sign;
		return result;
	};
	
	if (exp < 0) { 
		/* FIXME: underflow */
		/* return signed zero */
		result.parts.mantisa = 0x0;
		result.parts.exp = 0x0;
		return result;
	};
	
	mant1 = a.parts.mantisa;
	if (a.parts.exp > 0) {
		mant1 |= FLOAT32_HIDDEN_BIT_MASK;
	} else {
		++exp;
	};
	
	mant2 = b.parts.mantisa;

	if (b.parts.exp > 0) {
		mant2 |= FLOAT32_HIDDEN_BIT_MASK;
	} else {
		++exp;
	};

	mant1 <<= 1; /* one bit space for rounding */

	mant1 = mant1 * mant2;
/* round and return */
	
	while ((exp < FLOAT32_MAX_EXPONENT) && (mant1 >= ( 1 << (FLOAT32_MANTISA_SIZE + 2)))) { 
		/* 23 bits of mantisa + one more for hidden bit (all shifted 1 bit left)*/
		++exp;
		mant1 >>= 1;
	};

	/* rounding */
	//++mant1; /* FIXME: not works - without it is ok */
	mant1 >>= 1; /* shift off rounding space */
	
	if ((exp < FLOAT32_MAX_EXPONENT) && (mant1 >= (1 << (FLOAT32_MANTISA_SIZE + 1)))) {
		++exp;
		mant1 >>= 1;
	};

	if (exp >= FLOAT32_MAX_EXPONENT ) {	
		/* TODO: fix overflow */
		/* return infinity*/
		result.parts.exp = FLOAT32_MAX_EXPONENT;
		result.parts.mantisa = 0x0;
		return result;
	}
	
	exp -= FLOAT32_MANTISA_SIZE;

	if (exp <= FLOAT32_MANTISA_SIZE) { 
		/* denormalized number */
		mant1 >>= 1; /* denormalize */
		while ((mant1 > 0) && (exp < 0)) {
			mant1 >>= 1;
			++exp;
		};
		if (mant1 == 0) {
			/* FIXME : underflow */
		result.parts.exp = 0;
		result.parts.mantisa = 0;
		return result;
		};
	};
	result.parts.exp = exp; 
	result.parts.mantisa = mant1 & ( (1 << FLOAT32_MANTISA_SIZE) - 1);
	
	return result;	
	
}

/** Multiply two 64 bit float numbers
 *
 */
float64 mulFloat64(float64 a, float64 b)
{
	float64 result;
	__u64 mant1, mant2;
	__s32 exp;

	result.parts.sign = a.parts.sign ^ b.parts.sign;
	
	if (isFloat64NaN(a) || isFloat64NaN(b) ) {
		/* TODO: fix SigNaNs */
		if (isFloat64SigNaN(a)) {
			result.parts.mantisa = a.parts.mantisa;
			result.parts.exp = a.parts.exp;
			return result;
		};
		if (isFloat64SigNaN(b)) { /* TODO: fix SigNaN */
			result.parts.mantisa = b.parts.mantisa;
			result.parts.exp = b.parts.exp;
			return result;
		};
		/* set NaN as result */
		result.binary = FLOAT64_NAN;
		return result;
	};
		
	if (isFloat64Infinity(a)) { 
		if (isFloat64Zero(b)) {
			/* FIXME: zero * infinity */
			result.binary = FLOAT64_NAN;
			return result;
		}
		result.parts.mantisa = a.parts.mantisa;
		result.parts.exp = a.parts.exp;
		return result;
	}

	if (isFloat64Infinity(b)) { 
		if (isFloat64Zero(a)) {
			/* FIXME: zero * infinity */
			result.binary = FLOAT64_NAN;
			return result;
		}
		result.parts.mantisa = b.parts.mantisa;
		result.parts.exp = b.parts.exp;
		return result;
	}

	/* exp is signed so we can easy detect underflow */
	exp = a.parts.exp + b.parts.exp;
	exp -= FLOAT64_BIAS;
	
	if (exp >= FLOAT64_MAX_EXPONENT) {
		/* FIXME: overflow */
		/* set infinity as result */
		result.binary = FLOAT64_INF;
		result.parts.sign = a.parts.sign ^ b.parts.sign;
		return result;
	};
	
	if (exp < 0) { 
		/* FIXME: underflow */
		/* return signed zero */
		result.parts.mantisa = 0x0;
		result.parts.exp = 0x0;
		return result;
	};
	
	mant1 = a.parts.mantisa;
	if (a.parts.exp > 0) {
		mant1 |= FLOAT64_HIDDEN_BIT_MASK;
	} else {
		++exp;
	};
	
	mant2 = b.parts.mantisa;

	if (b.parts.exp > 0) {
		mant2 |= FLOAT64_HIDDEN_BIT_MASK;
	} else {
		++exp;
	};

	mant1 <<= 1; /* one bit space for rounding */

	mul64integers(mant1, mant2, &mant1, &mant2);

/* round and return */
	/* FIXME: ugly soulution is to shift whole mant2 >> as in 32bit version
	 * Here is is more slower because we have to shift two numbers with carry
	 * Better is find first nonzero bit and make only one shift
	 * Third version is to shift both numbers a bit to right and result will be then 
	 * placed in higher part of result. Then lower part will be good only for rounding.
	 */
	
	while ((exp < FLOAT64_MAX_EXPONENT) && (mant2 > 0 )) { 
		mant1 >>= 1;
		mant1 &= ((mant2 & 0x1) << 63);
		mant2 >>= 1;
		++exp;
	}
	
	while ((exp < FLOAT64_MAX_EXPONENT) && (mant1 >= ( (__u64)1 << (FLOAT64_MANTISA_SIZE + 2)))) { 
		++exp;
		mant1 >>= 1;
	};

	/* rounding */
	//++mant1; /* FIXME: not works - without it is ok */
	mant1 >>= 1; /* shift off rounding space */
	
	if ((exp < FLOAT64_MAX_EXPONENT) && (mant1 >= ((__u64)1 << (FLOAT64_MANTISA_SIZE + 1)))) {
		++exp;
		mant1 >>= 1;
	};

	if (exp >= FLOAT64_MAX_EXPONENT ) {	
		/* TODO: fix overflow */
		/* return infinity*/
		result.parts.exp = FLOAT64_MAX_EXPONENT;
		result.parts.mantisa = 0x0;
		return result;
	}
	
	exp -= FLOAT64_MANTISA_SIZE;

	if (exp <= FLOAT64_MANTISA_SIZE) { 
		/* denormalized number */
		mant1 >>= 1; /* denormalize */
		while ((mant1 > 0) && (exp < 0)) {
			mant1 >>= 1;
			++exp;
		};
		if (mant1 == 0) {
			/* FIXME : underflow */
		result.parts.exp = 0;
		result.parts.mantisa = 0;
		return result;
		};
	};
	result.parts.exp = exp; 
	result.parts.mantisa = mant1 & ( ((__u64)1 << FLOAT64_MANTISA_SIZE) - 1);
	
	return result;	
	
}

/** Multiply two 64 bit numbers and return result in two parts
 * @param a first operand
 * @param b second operand
 * @param lo lower part from result
 * @param hi higher part of result
 */
void mul64integers(__u64 a,__u64 b, __u64 *lo, __u64 *hi)
{
	__u64 low, high, middle1, middle2;
	__u32 alow, blow;
	
	alow = a & 0xFFFFFFFF;
	blow = b & 0xFFFFFFFF;
	
	a <<= 32;
	b <<= 32;
	
	low = (__u64)alow * blow;
	middle1 = a * blow;
	middle2 = alow * b;
	high = a * b;

	middle1 += middle2;
	high += ((__u64)(middle1 < middle2) << 32) + middle1>>32;
	middle1 << 32;
	low += middle1;
	high += (low < middle1);
	*lo = low;
	*hi = high;
	return;
}


