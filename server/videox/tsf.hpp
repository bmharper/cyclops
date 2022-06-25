#pragma once
#ifndef TSF_HPP_INCLUDED
#define TSF_HPP_INCLUDED

/*

tsf: A small, typesafe, cross-platform printf replacement (tested on Windows & Linux).

We use snprintf as a backend, so all of the regular formatting
commands that you expect from the printf family of functions work.
This makes the code much smaller than other implementations.

We do however implement some of the common operations ourselves,
such as emitting integers or plain strings, because most snprintf
implementations are actually very slow, and we can gain a lot of
speed by doing these common operations ourselves.

Usage:

tsf::fmt("%v %v", "abc", 123)        -->  "abc 123"     <== Use %v as a generic value type
tsf::fmt("%s %d", "abc", 123)        -->  "abc 123"     <== Specific value types are fine too, unless they conflict with the provided type, in which case they are overridden
tsf::fmt("%v", std::string("abc"))   -->  "abc"         <== std::string
tsf::fmt("%v", std::wstring("abc"))  -->  "abc"         <== std::wstring
tsf::fmt("%.3f", 25.5)               -->  "25.500"      <== Use format strings as usual
tsf::print("%v", "Hello world")      -->  "Hello world" <== Print to stdout
tsf::print(stderr, "err %v", 5)      -->  "err 5"       <== Print to stderr (or any other FILE*)

Known unsupported features:
* Positional arguments
* %*s (integer width parameter)	-- wouldn't be hard to add. Presently ignored.

API:

fmt           returns std::string.
fmt_buf       is useful if you want to provide your own buffer to avoid memory allocations.
print         prints to stdout
print(FILE*)  prints to any FILE*

By providing a cast operator to fmtarg, you can get an arbitrary type
supported as an argument, provided it fits into one of the molds of the
printf family of arguments.

We also support two custom types: %Q and %q. In order to use these, you need to provide your own
implementation that wraps one of the lower level functions, and provides a 'context' object with
custom functions defined for Escape_Q and Escape_q. These were originally added in order to provide
quoting and escaping for SQL identifiers and SQL string literals.

*/

#if defined(_MSC_VER)
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#ifndef TSF_FMT_API
#define TSF_FMT_API
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <string>

namespace tsf {

class fmtarg {
public:
	enum Types {
		TNull, // Used as a sentinel to indicate that no parameter was passed
		TPtr,
		TCStr,
		TWStr,
		TI32,
		TU32,
		TI64,
		TU64,
		TDbl,
	};
	union {
		const void*    Ptr;
		const char*    CStr;
		const wchar_t* WStr;
		int32_t        I32;
		uint32_t       UI32;
		int64_t        I64;
		uint64_t       UI64;
		double         Dbl;
	};
	Types Type;

	fmtarg() : Type(TNull), CStr(NULL) {}
	fmtarg(const void* v) : Type(TPtr), Ptr(v) {}
	fmtarg(const char* v) : Type(TCStr), CStr(v) {}
	fmtarg(const wchar_t* v) : Type(TWStr), WStr(v) {}
	fmtarg(const std::string& v) : Type(TCStr), CStr(v.c_str()) {}
	fmtarg(const std::wstring& v) : Type(TWStr), WStr(v.c_str()) {}
#ifdef _MSC_VER
	fmtarg(long v) : Type(TI32), I32(v) {
	}
	fmtarg(unsigned long v) : Type(TU32), UI32(v) {}
	fmtarg(__int32 v) : Type(TI32), I32(v) {}
	fmtarg(unsigned __int32 v) : Type(TU32), UI32(v) {}
	fmtarg(__int64 v) : Type(TI64), I64(v) {}
	fmtarg(unsigned __int64 v) : Type(TU64), UI64(v) {}
#else
	// clang/gcc
	fmtarg(int v) : Type(TI32), I32(v) {}
	fmtarg(unsigned int v) : Type(TU32), UI32(v) {}
#if LONG_MAX == 0x7fffffff
	// 32-bit arch
	fmtarg(long v) : Type(TI32), I32(v) {}
	fmtarg(unsigned long v) : Type(TU32), UI32(v) {}
#else
	// 64-bit arch
	fmtarg(long v) : Type(TI64), I64(v) {}
	fmtarg(unsigned long v) : Type(TU64), UI64(v) {}
#endif
	fmtarg(long long v) : Type(TI64), I64(v) {
	}
	fmtarg(unsigned long long v) : Type(TU64), UI64(v) {}
#endif
	fmtarg(double v) : Type(TDbl), Dbl(v) {
	}
};

/* This can be used to add custom formatting tokens.
	The only supported characters that you can use are "Q" and "q".
	These were added for escaping SQL identifiers and SQL strings.
	*/
struct context {
	// Return the number of characters written, or -1 if outBufSize is not large enough to hold
	// the number of characters that you need to write. Do not write a null terminator.
	typedef size_t (*WriteSpecialFunc)(char* outBuf, size_t outBufSize, const fmtarg& val);

	WriteSpecialFunc Escape_Q = nullptr;
	WriteSpecialFunc Escape_q = nullptr;
};

struct StrLenPair {
	char*  Str;
	size_t Len;
};

TSF_FMT_API std::string fmt_core(const context& context, const char* fmt, ssize_t nargs, const fmtarg* args);
TSF_FMT_API StrLenPair  fmt_core(const context& context, const char* fmt, ssize_t nargs, const fmtarg* args, char* staticbuf, size_t staticbuf_size);

namespace internal {

inline void fmt_pack(fmtarg* pack) {
}

inline void fmt_pack(fmtarg* pack, const fmtarg& arg) {
	*pack = arg;
}

template <typename... Args>
void fmt_pack(fmtarg* pack, const fmtarg& arg, const Args&... args) {
	*pack = arg;
	fmt_pack(pack + 1, args...);
}

} // namespace internal

// Format and return std::string
template <typename... Args>
std::string fmt(const char* fs, const Args&... args) {
	const auto num_args = sizeof...(Args);
	fmtarg     pack_array[num_args + 1]; // +1 for zero args case
	internal::fmt_pack(pack_array, args...);
	context cx;
	return fmt_core(cx, fs, (ssize_t) num_args, pack_array);
}

// If the formatted string, with null terminator, fits inside staticbuf_len, then the returned pointer is staticbuf,
// and no memory allocation takes place.
// However, if the formatted string is too large to fit inside staticbuf_len, then the returned pointer must
// be deleted with "delete[] ptr".
template <typename... Args>
StrLenPair fmt_buf(const context& cx, char* buf, size_t buf_len, const char* fs, const Args&... args) {
	const auto num_args = sizeof...(Args);
	fmtarg     pack_array[num_args + 1]; // +1 for zero args case
	internal::fmt_pack(pack_array, args...);
	return fmt_core(cx, fs, (ssize_t) num_args, pack_array, buf, buf_len);
}

// If the formatted string, with null terminator, fits inside staticbuf_len, then the returned pointer is staticbuf,
// and no memory allocation takes place.
// However, if the formatted string is too large to fit inside staticbuf_len, then the returned pointer must
// be deleted with "delete[] ptr".
template <typename... Args>
StrLenPair fmt_buf(char* buf, size_t buf_len, const char* fs, const Args&... args) {
	context cx;
	return fmt_buf(cx, buf, buf_len, fs, args...);
}

// Format and write to FILE*
template <typename... Args>
size_t print(FILE* file, const char* fs, const Args&... args) {
	auto res = fmt(fs, args...);
	if (res.size() == 0)
		return 0;
	return fwrite(res.c_str(), 1, res.length(), file);
}

// Format and write to stdout
template <typename... Args>
size_t print(const char* fs, const Args&... args) {
	return print(stdout, fs, args...);
}

/*  cross-platform "snprintf"

		destination			Destination buffer
		count				Number of characters available in 'destination'. This must include space for the null terminating character.
		format_str			The format string
		return
			-1				Not enough space
			0..count-1		Number of characters written, excluding the null terminator. The null terminator was written though.
	*/
TSF_FMT_API int fmt_snprintf(char* destination, size_t count, const char* format_str, ...);

} // namespace tsf

namespace tsf {

static const size_t argbuf_arraysize = 16;

#ifdef _WIN32
static const char* i64Prefix   = "I64";
static const char* wcharPrefix = "";
static const char  wcharType   = 'S';
#else
static const char* i64Prefix = "ll";
static const char* wcharPrefix = "l";
static const char wcharType = 's';
#endif

class StackBuffer {
public:
	char*  Buffer;    // The buffer
	size_t Pos;       // The number of bytes appended
	size_t Capacity;  // Capacity of 'Buffer'
	bool   OwnBuffer; // True if we have allocated the buffer

	StackBuffer(char* staticbuf, size_t staticbuf_size) {
		OwnBuffer = false;
		Pos       = 0;
		Buffer    = staticbuf;
		Capacity  = staticbuf_size;
	}

	void Reserve(size_t bytes) {
		if (Pos + bytes > Capacity) {
			size_t ncap = Capacity * 2;
			if (ncap < Pos + bytes)
				ncap = Pos + bytes;
			char* nbuf = new char[ncap];
			memcpy(nbuf, Buffer, Pos);
			Capacity = ncap;
			if (OwnBuffer)
				delete[] Buffer;
			OwnBuffer = true;
			Buffer    = nbuf;
		}
	}

	void MoveCurrentPos(size_t bytes) {
		Pos += bytes;
		assert(Pos <= Capacity);
	}

	char* AddUninitialized(size_t bytes) {
		Reserve(bytes);
		char* p = Buffer + Pos;
		Pos += bytes;
		return p;
	}

	void Add(char c) {
		char* p = AddUninitialized(1);
		*p      = c;
	}

	size_t RemainingSpace() const { return Capacity - Pos; }
};

inline int format_string(char* destination, size_t count, const char* format_str, const char* s) {
	if (format_str[0] == '%' && format_str[1] == 's') {
		size_t i = 0;
		for (; i < count; i++) {
			if (!s[i])
				return (int) i;
			destination[i] = s[i];
		}
		return -1;
	}
	return fmt_snprintf(destination, count, format_str, s);
}

template <typename TInt, int tbase, bool upcase>
int format_integer(char* destination, TInt value) {
	// we could theoretically do a lower base than 10, but then our static buffer would need to be bigger.
	static_assert(tbase >= 10 && tbase <= 36, "base invalid");
	TInt base = (TInt) tbase;
	char buf[20];

	const char* lut = upcase ? "ZYXWVUTSRQPONMLKJIHGFEDCBA9876543210123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

	size_t i = 0;
	TInt   tmp_value;
	do {
		tmp_value = value;
		value /= base;
		buf[i++] = lut[35 + (tmp_value - value * base)];
	} while (value);

	if (tmp_value < 0)
		buf[i++] = '-';

	size_t n = i;
	i--;
	for (size_t j = 0; j < n; j++, i--)
		destination[j] = buf[i];
	return (int) n;
}

inline int format_int32(char* destination, size_t count, const char* format_str, int32_t v) {
	switch (format_str[1]) {
	case 'd':
	case 'i':
		if (count >= 11)
			return format_integer<int32_t, 10, false>(destination, v);
		break;
	case 'u':
		if (count >= 11)
			return format_integer<uint32_t, 10, false>(destination, v);
		break;
	case 'x':
		if (count >= 8)
			return format_integer<uint32_t, 16, false>(destination, v);
		break;
	case 'X':
		if (count >= 8)
			return format_integer<uint32_t, 16, true>(destination, v);
		break;
	}
	return fmt_snprintf(destination, count, format_str, v);
}

inline int format_int64(char* destination, size_t count, const char* format_str, int64_t v) {
#ifdef _WIN32
	bool isPlain = format_str[1] == i64Prefix[0] && format_str[2] == i64Prefix[1] && format_str[3] == i64Prefix[2];
#else
	bool isPlain = format_str[1] == i64Prefix[0] && format_str[2] == i64Prefix[1];
#endif
	if (isPlain) {
		switch (format_str[4]) {
		case 'd':
		case 'i':
			if (count >= 20)
				return format_integer<int64_t, 10, false>(destination, v);
			break;
		case 'u':
			if (count >= 20)
				return format_integer<uint64_t, 10, false>(destination, v);
			break;
		case 'x':
			if (count >= 16)
				return format_integer<uint64_t, 16, false>(destination, v);
			break;
		case 'X':
			if (count >= 16)
				return format_integer<uint64_t, 16, true>(destination, v);
			break;
		}
	}
	return fmt_snprintf(destination, count, format_str, v);
}

inline void fmt_settype(char argbuf[argbuf_arraysize], size_t pos, const char* width, char type) {
	if (width != nullptr) {
		// set the type and the width specifier
		switch (argbuf[pos - 1]) {
		case 'l':
		case 'h':
		case 'w':
			pos--;
			break;
		}

		for (; *width; width++, pos++)
			argbuf[pos] = *width;

		argbuf[pos++] = type;
		argbuf[pos++] = 0;
	} else {
		// only set the type, not the width specifier
		argbuf[pos++] = type;
		argbuf[pos++] = 0;
	}
}

inline int fmt_output_with_snprintf(char* outbuf, char fmt_type, char argbuf[argbuf_arraysize], size_t argbufsize, size_t outputSize, const fmtarg* arg) {
#define SETTYPE1(type) fmt_settype(argbuf, argbufsize, nullptr, type)
#define SETTYPE2(width, type) fmt_settype(argbuf, argbufsize, width, type)

	bool tokenint  = false;
	bool tokenreal = false;

	switch (fmt_type) {
	case 'd':
	case 'i':
	case 'o':
	case 'u':
	case 'x':
	case 'X':
		tokenint = true;
	}

	switch (fmt_type) {
	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G':
	case 'a':
	case 'A':
		tokenreal = true;
	}

	switch (arg->Type) {
	case fmtarg::TNull:
		return 0;
	case fmtarg::TPtr:
		SETTYPE1('p');
		return fmt_snprintf(outbuf, outputSize, argbuf, arg->Ptr);
	case fmtarg::TCStr:
		SETTYPE2("", 's');
		return format_string(outbuf, outputSize, argbuf, arg->CStr);
	case fmtarg::TWStr:
		SETTYPE2(wcharPrefix, wcharType);
		return fmt_snprintf(outbuf, outputSize, argbuf, arg->WStr);
	case fmtarg::TI32:
		if (fmt_type == 'c') {
			SETTYPE2("", 'c');
		} else if (tokenint) {
			SETTYPE2("", fmt_type);
		} else {
			SETTYPE2("", 'd');
		}
		return format_int32(outbuf, outputSize, argbuf, arg->I32);
	case fmtarg::TU32:
		if (tokenint) {
			SETTYPE2("", fmt_type);
		} else {
			SETTYPE2("", 'u');
		}
		return format_int32(outbuf, outputSize, argbuf, arg->UI32);
	case fmtarg::TI64:
		if (tokenint) {
			SETTYPE2(i64Prefix, fmt_type);
		} else {
			SETTYPE2(i64Prefix, 'd');
		}
		return format_int64(outbuf, outputSize, argbuf, arg->I64);
		// return fmt_snprintf(outbuf, outputSize, argbuf, arg->UI64);
	case fmtarg::TU64:
		if (tokenint) {
			SETTYPE2(i64Prefix, fmt_type);
		} else {
			SETTYPE2(i64Prefix, 'u');
		}
		return format_int64(outbuf, outputSize, argbuf, arg->UI64);
	case fmtarg::TDbl:
		if (tokenreal) {
			SETTYPE1(fmt_type);
		} else {
			SETTYPE1('g');
		}
		return fmt_snprintf(outbuf, outputSize, argbuf, arg->Dbl);
	}

#undef SETTYPE1
#undef SETTYPE2

	return 0;
}

inline TSF_FMT_API std::string fmt_core(const context& context, const char* fmt, ssize_t nargs, const fmtarg* args) {
	static const size_t bufsize = 256;
	char                staticbuf[bufsize];
	StrLenPair          res = fmt_core(context, fmt, nargs, args, staticbuf, bufsize);
	std::string         str(res.Str, res.Len);
	if (res.Str != staticbuf)
		delete[] res.Str;
	return str;
}

inline TSF_FMT_API StrLenPair fmt_core(const context& context, const char* fmt, ssize_t nargs, const fmtarg* args, char* staticbuf, size_t staticbuf_size) {
	if (nargs == 0) {
		// This is a common case worth optimizing. Unfortunately we cannot return 'fmt' directly, because it may be a temporary object.
		size_t len = strlen(fmt);
		if (staticbuf_size != 0 && len <= staticbuf_size + 1) {
			memcpy(staticbuf, fmt, len + 1);
			return StrLenPair{staticbuf, len};
		}
		StrLenPair r;
		r.Str = new char[len + 1];
		r.Len = len;
		memcpy(r.Str, fmt, len + 1);
		return r;
	}

	ssize_t       tokenstart = -1; // true if we have passed a %, and are looking for the end of the token
	ssize_t       iarg       = 0;
	bool          no_args_remaining;
	bool          spec_too_long;
	bool          disallowed;
	const ssize_t MaxOutputSize = 1 * 1024 * 1024;

	size_t      initial_sprintf_guessed_size = staticbuf_size >> 2; // must be less than staticbuf_size
	StackBuffer output(staticbuf, staticbuf_size);

	char argbuf[argbuf_arraysize];

	// we can always safely look one ahead, because 'fmt' is by definition zero terminated
	for (ssize_t i = 0; fmt[i]; i++) {
		if (tokenstart != -1) {
			bool tokenint  = false;
			bool tokenreal = false;
			bool is_q      = fmt[i] == 'q';
			bool is_Q      = fmt[i] == 'Q';

			switch (fmt[i]) {
			case 'a':
			case 'A':
			case 'c':
			case 'C':
			case 'd':
			case 'i':
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
			case 'H':
			case 'o':
			case 's':
			case 'S':
			case 'u':
			case 'x':
			case 'X':
			case 'p':
			case 'n':
			case 'v':
			case 'q':
			case 'Q':
				no_args_remaining = iarg >= nargs;                          // more tokens than arguments
				spec_too_long     = i - tokenstart >= argbuf_arraysize - 1; // %_____too much data____v
				disallowed        = fmt[i] == 'n';

				if (is_q && context.Escape_q == nullptr)
					disallowed = true;

				if (is_Q && context.Escape_Q == nullptr)
					disallowed = true;

				if (no_args_remaining || spec_too_long || disallowed) {
					for (ssize_t j = tokenstart; j <= i; j++)
						output.Add(fmt[j]);
				} else {
					// prepare the single formatting token that we will send to snprintf
					ssize_t argbufsize = 0;
					for (ssize_t j = tokenstart; j < i; j++) {
						if (fmt[j] == '*')
							continue; // ignore
						argbuf[argbufsize++] = fmt[j];
					}

					// grow output buffer size until we don't overflow
					const fmtarg* arg = &args[iarg];
					iarg++;
					ssize_t outputSize = initial_sprintf_guessed_size;
					while (true) {
						char*   outbuf  = (char*) output.AddUninitialized(outputSize);
						bool    done    = false;
						ssize_t written = 0;
						if (is_q)
							written = context.Escape_q(outbuf, outputSize, *arg);
						else if (is_Q)
							written = context.Escape_Q(outbuf, outputSize, *arg);
						else
							written = fmt_output_with_snprintf(outbuf, fmt[i], argbuf, argbufsize, outputSize, arg);

						if (written >= 0 && written < outputSize) {
							output.MoveCurrentPos(written - outputSize);
							break;
						} else if (outputSize >= MaxOutputSize) {
							// give up. I first saw this on the Microsoft CRT when trying to write the "mu" symbol to an ascii string.
							break;
						}
						// discard and try again with a larger buffer
						output.MoveCurrentPos(-outputSize);
						outputSize = outputSize * 2;
					}
				}
				tokenstart = -1;
				break;
			case '%':
				output.Add('%');
				tokenstart = -1;
				break;
			default:
				break;
			}
		} else {
			// Look ahead to find the next % token. Most of our time is spend just
			// scanning through regular text, so it pays to make that fast.
			// In order to do that, we determine up front how much space is left in
			// our buffer, and then fill it up without checking at each character,
			// whether we have enough space. This turns out to be a big win.
			ssize_t stopAt = i + output.RemainingSpace();
			for (; i < stopAt && fmt[i] != '%' && fmt[i] != 0; i++)
				output.Buffer[output.Pos++] = fmt[i];

			if (fmt[i] == '%')
				tokenstart = i;
			else if (fmt[i] == 0)
				break;
			else {
				// need more buffer space; come around for another pass
				output.Reserve(1);
				i--;
			}
		}
	}
	output.Add('\0');
	return {output.Buffer, output.Pos - 1};
}

inline int fmt_translate_snprintf_return_value(int r, size_t count) {
	if (r < 0 || (size_t) r >= count)
		return -1;
	else
		return r;
}

inline TSF_FMT_API int fmt_snprintf(char* destination, size_t count, const char* format_str, ...) {
	va_list va;
	va_start(va, format_str);
	int r = vsnprintf(destination, count, format_str, va);
	va_end(va);
	return fmt_translate_snprintf_return_value(r, count);
}

} // namespace tsf

#endif // TSF_HPP_INCLUDED