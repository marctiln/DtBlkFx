#ifndef _DT_FIX_POINT_H_
#define _DT_FIX_POINT_H_
/*
FixPoint template
 run through preprocess.pl to generate actual code
 perl preprocess.pl FixPoint_.h >FixPoint.h

*/

#include <math.h>

template <
	int FRAC_BITS_,		// number of bits to represent fractional part
	class T = long,		// storage type (default 32 bits)
	class F = float		// floating point conversion type
> struct FixPoint
//
// integer with a fixed number of bits representing the fraction
//
{
public: // constants
	enum { FRAC_BITS = FRAC_BITS_ };

	// fraction multiplier
	static T FracMult() { return (T)1 << FRAC_BITS; }

	// fraction mask, all bits set
	static T FracMask() { return FracMult()-1; }

	// most-significant-bit of fraction
	static T FracTopBit() { return (T)1<<(FRAC_BITS-1); }

	// use this for constant shifts to make sure negative shift works
	static T shiftUp(T val, int shift)
	{
		if(shift < 0) return val >> -shift;
		return val << shift;
	}

	// return smallest value
	static FixPoint epsilon() { return Pass(1); }

public: // data
	// fixed point value (fraction is the bottom "FRAC_BITS" bits)
	T val;

public: // conversions back to integer
	// return value rounded to nearest integer
	T getRound() const { return (val + FracTopBit()) >> FRAC_BITS; }
	void doRound() { val = (val + FracTopBit()) & ~FracMask(); }

	// return value rounded up
	T getCeil() const  { return (val + FracMask()) >> FRAC_BITS; }
	void doCeil() { val = (val + FracMask()) & ~FracMask(); }

	// return value rounded down
	T getFloor() const { return val >> FRAC_BITS; }
	void doFloor() { val = val & ~FracMask(); }

public: // conversions to a float or a double
	operator F() const { return (1.0f/(F)FracMult()) * (F)val; }

public: // various types of set
	// set from another FixPoint with fraction rounded to closest value if reducing precision
	template <int FRAC_BITS2>
		void setClosest(FixPoint<FRAC_BITS2, T> src)
		{
			val = shiftUp(
				src.val + shiftUp(1, FRAC_BITS2-FRAC_BITS-1),
				FRAC_BITS-FRAC_BITS2
			);
		}
	void setClosest(float src) { val = (T)(src * (float)FracMult() + 0.5f); }
	void setClosest(double src) { val = (T)(src * (double)FracMult() + 0.5); }

	template <class SRC> static FixPoint closest(const SRC& src) { FixPoint r; r.setClosest(src); return r; }

	// ceil from another fraction
	template <int FRAC_BITS2>
		void setClosestHigher(FixPoint<FRAC_BITS2, T> src)
		{
			if(FRAC_BITS2 > FRAC_BITS)
				val = shiftUp(
					src.val + shiftUp(1, FRAC_BITS2-FRAC_BITS) - 1,
					FRAC_BITS-FRAC_BITS2
				);
			else set(src);
		}
	void setClosestHigher(float src) { val = (T)ceilf(src * (float)FracMult()); }
	void setClosestHigher(double src) { val = (T)ceil(src * (double)FracMult()); }


	// set value with fraction truncated (as opposed to rounded) if reducing precision
	template <int FRAC_BITS2>
		void setTrunc(FixPoint<FRAC_BITS2, T> src)
			{ val = shiftUp(src.val, FRAC_BITS-FRAC_BITS2); }
	void setTrunc(float src) { val = (T)(src * (float)FracMult()); }
	void setTrunc(double src) { val = (T)(src * (double)FracMult()); }

	// "set" is a synonmyn for "setTrunc"
	template <class T2> void set(T2 src) { setTrunc(src); }

public: // access fraction
	// return fraction
	T getFracRaw() const { return val & FracMask(); }

	// return fraction adjusted to n_bits, truncated for less precision or
	// zero extended for more precision
	T getFracRaw(int n_bits) const { return shiftUp(getFracRaw(),  n_bits-FRAC_BITS); }

	// return fractional part
	FixPoint getFrac() const { return Pass(getFracRaw()); }

public: // constructors

	// wrapper class to construct value transparently
	struct Pass {
		T val;
		explicit Pass(T src) { val = src; }
	};

	FixPoint() {}
	FixPoint(Pass p) { val = p.val; }

	// construct from integer type
	FixPoint(int src) { val = (T)src << FRAC_BITS; }
	FixPoint(T src) { val = src << FRAC_BITS; }

	// force number of fraction bits
	FixPoint(T src, int src_frac_bits) { val = shiftUp(src, FRAC_BITS-src_frac_bits); }

	// can construct, assume fraction truncated (rounded down) when reducing precision
	//template <class T2> FixPoint(T2 src) { setTrunc(src); }
	template <int FRAC_BITS2> FixPoint(FixPoint<FRAC_BITS2, T> src) { setTrunc(src); }
	FixPoint(float src) { setTrunc(src); }
	FixPoint(double src) { setTrunc(src); }

public: // operators, note FixPoint is assumed to be left hand argument & precision is always adjusted accordingly

	FixPoint operator -() { return Pass(-val); }

	//
	template <class T2> FixPoint operator + (T2 other) const { return Pass(val + FixPoint(other).val); }
	template <class T2> FixPoint operator - (T2 other) const { return Pass(val - FixPoint(other).val); }
	FixPoint operator << (int s) const { return Pass(val<<s); }
	FixPoint operator >> (int s) const { return Pass(val>>s); }

	// 64 bit precision fixed point division : careful that you don't overflow, division requires FRAC_BITS + FRAC_BITS2*2 fraction bits (+ 1 more bit if rounding)
	template <int FRAC_BITS2> void doDiv64Trunc(FixPoint<FRAC_BITS2> div) { val = (T)(((long long)val<<FRAC_BITS2) / div.val); }
	template <int FRAC_BITS2> void doDiv64Closest(FixPoint<FRAC_BITS2> div) { val = (T)((((long long)val<<(FRAC_BITS2+1))+div.val) / (div.val<<1)); }
	template <int FRAC_BITS2> void doDiv32Trunc(FixPoint<FRAC_BITS2> div) { val = (T)(((long)val<<FRAC_BITS2) / div.val); }
	template <int FRAC_BITS2> void doDiv32Closest(FixPoint<FRAC_BITS2> div) { val = (T)((((long)val<<(FRAC_BITS2+1))+div.val) / (div.val<<1)); }

	// multiply requires FRAC_BITS+FRAC_BITS2
	template <int FRAC_BITS2> void doMul64Trunc(FixPoint<FRAC_BITS2> mul) { val = (T)(((long long)val*(long long)mul.val) >> mul.FRAC_BITS); }
	template <int FRAC_BITS2> void doMul64Closest(FixPoint<FRAC_BITS2> mul) { val = (T)(((long long)val*(long long)mul.val+mul.FracTopBit()) >> mul.FRAC_BITS); }
	template <int FRAC_BITS2> void doMul32Trunc(FixPoint<FRAC_BITS2> mul) { val = (T)(((long)val*(long)mul.val) >> mul.FRAC_BITS); }
	template <int FRAC_BITS2> void doMul32Closest(FixPoint<FRAC_BITS2> mul) { val = (T)(((long)val*(long)mul.val+mul.FracTopBit()) >> mul.FRAC_BITS); }

	// multiply & divide operators on another fixed point frac do 64 bit truncated
	template <int FRAC_BITS2> FixPoint operator / (FixPoint<FRAC_BITS2> div) { FixPoint t = Pass(val); t.doDiv64Trunc(div); return t; }
	template <int FRAC_BITS2> FixPoint operator * (FixPoint<FRAC_BITS2> mul) { FixPoint t = Pass(val); t.doMul64Trunc(mul); return t; }

	// prefix increment/decrement
	FixPoint& operator ++() { val += FracMult(); return *this; }
	FixPoint& operator --() { val -= FracMult(); return *this; }
	// postfix
	FixPoint operator ++(int) { FixPoint t = *this; ++ *this; return t; }
	FixPoint operator --(int) { FixPoint t = *this; -- *this; return t; }

	FixPoint& operator >>= (int s) { val >>= s; return *this; }
	FixPoint& operator <<= (int s) { val <<= s; return *this; }


	//note that the following round down if the result is beyond precision
	//P:for $OP ('*', '/') {
		//P:for $TYPE ('float', 'double') {
			FixPoint operator $OP ($TYPE other) { return Pass((T)(($TYPE)val $OP other)); }
			FixPoint& operator $OP= ($TYPE other) { val = (T)(($TYPE)val $OP other); return *this; }
		//P:}
		//P:for $TYPE ('int', 'T') {
			FixPoint operator $OP ($TYPE other) { return Pass(val $OP other); }
			FixPoint& operator $OP= ($TYPE other) { val $OP= other; return *this; }
		//P:}
	//P:}

	// comparisons, note that the RHS is cast to the same precision as LHS before compare
	//P:for $OP ('>', '>=', '<', '<=', '==', '!=') {
		template<class R_TYPE> bool operator $OP (const R_TYPE& right) { return val $OP FixPoint(right).val; }
	//P:}

	template <class T2> FixPoint& operator += (const T2& other) { val += FixPoint(other).val; return *this; }
	template <class T2> FixPoint& operator -= (const T2& other) { val -= FixPoint(other).val; return *this; }
	template <class T2> FixPoint& operator *= (T2 mul) { return *this = *this * mul; }
	template <class T2> FixPoint& operator /= (T2 div) { return *this = *this / div; }

	bool operator !() { return val == 0; }
};

template <int FRAC_BITS, class T> inline T round(const FixPoint<FRAC_BITS, T>& t) { return t.getRound(); }
template <int FRAC_BITS, class T> inline T ceil(const FixPoint<FRAC_BITS, T>& t) { return t.getCeil(); }
template <int FRAC_BITS, class T> inline T floor(const FixPoint<FRAC_BITS, T>& t) { return t.getFloor(); }
template <int FRAC_BITS, class T> inline T fixRound(const FixPoint<FRAC_BITS, T>& t) { return t.getRound(); }
template <int FRAC_BITS, class T> inline T fixCeil(const FixPoint<FRAC_BITS, T>& t) { return t.getCeil(); }
template <int FRAC_BITS, class T> inline T fixFloor(const FixPoint<FRAC_BITS, T>& t) { return t.getFloor(); }

//P:for $L_TYPE ('int', 'T') {
	//P:for $OP ('-', '/') {
		template <int FRAC_BITS, class T> inline FixPoint<FRAC_BITS, T> operator $OP ($L_TYPE left, const FixPoint<FRAC_BITS, T>& right) { return FixPoint<FRAC_BITS, T>(left) $OP right; }
	//P:}
	//P:for $OP ('+', '*') {
		template <int FRAC_BITS, class T> inline FixPoint<FRAC_BITS, T> operator $OP ($L_TYPE left, const FixPoint<FRAC_BITS, T>& right) { return right $OP left; }
	//P:}
//P:}
//P:for $OP ('>', '>=', '<', '<=', '==', '!=') {
	//P:for $L_TYPE ('int', 'T') {
		template <int FRAC_BITS, class T> inline bool operator $OP ($L_TYPE left, const FixPoint<FRAC_BITS, T>& right) { return FixPoint<FRAC_BITS, T>(left).val $OP right.val; }
	//P:}
	//P:for $L_TYPE ('float', 'double') {
		template <int FRAC_BITS, class T> inline bool operator $OP ($L_TYPE left, const FixPoint<FRAC_BITS, T>& right) { return left $OP ($L_TYPE)right; }
	//P:}
//P:}

#endif
