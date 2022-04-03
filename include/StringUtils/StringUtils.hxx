#ifndef INCLUDED_STRINGUTILS_HXX
#define INCLUDED_STRINGUTILS_HXX

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <cstdarg>
#include <cctype>
#include <cstring>
#include <cstdio>

//#include "StdInt.hxx" // uint16_t, uint32_t
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char byte;

inline std::string lower(const std::string &s)
{
	std::string s2 = s;
	std::transform(s2.begin(), s2.end(), s2.begin(), (int(*)(int))std::tolower);
	return s2;
}

std::string replace(const std::string &s, const std::string &a, const std::string &b); // forward decl
inline std::string lowerLatin1(const std::string &s)
{
	std::string s2 = lower(s);
	s2 = replace(s2, "\xc1", "\xe1"); // "�" -> "�"
	s2 = replace(s2, "\xc9", "\xe9"); // "�" -> "�"
	s2 = replace(s2, "\xcd", "\xed"); // "�" -> "�"
	s2 = replace(s2, "\xd3", "\xf3"); // "�" -> "�"
	s2 = replace(s2, "\xda", "\xfa"); // "�" -> "�"
	s2 = replace(s2, "\xdc", "\xfc"); // "�" -> "�"
	s2 = replace(s2, "\xd1", "\xf1"); // "�" -> "�"
	return s2;
}

inline std::string upper(std::string &s)
{
	std::string s2 = s;
	std::transform(s2.begin(), s2.end(), s2.begin(), (int(*)(int))std::toupper);
	return s2;
}

// replace all occurrences of a in s with b
inline std::string replace(const std::string &s, const std::string &a, const std::string &b)
{
	std::string result;
	std::string::size_type pos = 0;

	while (1)
	{
		std::string::size_type onset = s.find(a, pos);

		if (onset == std::string::npos)
		{
			result += s.substr(pos, s.size()-pos);
			break;
		}

		result += s.substr(pos, onset-pos);
		result += b;
		pos = onset + a.size();
	}

	return result;
}

// replace all occurrences of all characters in string a in s with string b
// e.g. noNumbers = strReplaceForAllCharsInA(input, "0123456789", "")
inline std::string replaceForAllCharsInA(const std::string &s, const std::string &a, const std::string &b)
{
	std::string result = s;
	for (int i = 0; i < (int)a.size(); ++i)
	{
		result = replace(result, a.substr(i, 1), b);
	}
	return result;
}


// -------------------------------------------------------------------------------------------------

// Get length of list or string.
// Equivalent to Python: len(string), len(list)
template<typename T>
inline int len(const T &obj)
{
	return (int)obj.size();
}

// Compute sum of all elements in list.
// Equivalent to Python: sum(list)
template<typename T>
inline typename T::value_type sum(const T &obj)
{
	double s = 0.0;
	for (int i = 0; i < (int)obj.size(); ++i)
	{
		s += obj[i];
	}
	return static_cast<typename T::value_type>(s);
}

// Helper for making lists of strings.
// Equivalent to Python: [i1]
inline std::vector<std::string> mkl(const std::string &i1)
{
	std::vector<std::string> result;
	result.push_back(i1);
	return result;
}

// Helper for making lists of strings.
// Equivalent to Python: [i1, i2]
inline std::vector<std::string> mkl(const std::string &i1, const std::string &i2)
{
	std::vector<std::string> result;
	result.push_back(i1);
	result.push_back(i2);
	return result;
}

// Helper for making lists of strings.
// Equivalent to Python: [i1, i2, i3]
inline std::vector<std::string> mkl(const std::string &i1, const std::string &i2, const std::string &i3)
{
	std::vector<std::string> result;
	result.push_back(i1);
	result.push_back(i2);
	result.push_back(i3);
	return result;
}

// Helper to extend list of of strings.
// Equivalent to Python: toExtend.extend(extendWith)
inline void extend(std::vector<std::string> &toExtend, const std::vector<std::string> &extendWith)
{
	toExtend.insert(toExtend.end(), extendWith.begin(), extendWith.end());
}

// Checks if character occurs in string or not.
// Equivalent to Python: c in s
inline bool charInString(unsigned char c, const std::string &s)
{
	return (s.find_first_of(c) != std::string::npos);
}

// Checks if string occurs in string or not.
// Equivalent to Python: s1 in s2
inline bool stringInString(const std::string &s1, const std::string &s2)
{
	return (s2.find(s1) != std::string::npos);
}

// Checks if string occurs in list of strings or not.
// Equivalent to Python: s in lst
inline bool stringInList(const std::string &s, const std::vector<std::string> &lst)
{
	for (int i = 0; i < (int)lst.size(); ++i)
	{
		if (lst[i] == s)
			return true;
	}
	return false;
}

// Get subset of list of strings.
// Equivalent to Python: list = list[b:e].
inline std::vector<std::string> slicelist(const std::vector<std::string> &list, int b, int e)
{
	if (e < 0)
		e = (int)list.size() + e;

	std::vector<std::string> result;
	for (int i = b; i < e; ++i)
	{
		result.push_back(list[i]);
	}
	return result;
}

// Get substring from string.
// Equivalent to Python: string = string[b:e].
inline std::string substr(const std::string &s, int b, int e)
{
	if (e < 0)
		e = (int)s.size() + e;

	return s.substr(b, e-b);
}

// Split string 'str' on all occurrences of substring 'sep'.
//
// E.g.:
// split("aaa::bbb::ccc", "::") = ["aaa", "bbb", "ccc"]
// split("a; b; c;", ";") = ["a", " b", " c", ""]
// split("abc", "#") = ["abc"]
//
// Same as Python's str.split(sep).
//
// Note that there's an overload of split() with no 'sep' argument which splits on any white space 
// character (and skips all white space).
inline std::vector<std::string> split(std::string str, std::string sep, int maxsplit = -1)
{
	std::vector<std::string> v;
	if (sep.size() == 0)
	{
//		throw std::runtime_error("empty separator"); // Python's split() behavior
		v.push_back(str); // return [str]
	}
	else
	{
		std::string::size_type bpos = 0;
		while (1)
		{
			if (maxsplit >= 0)
			{
				if (v.size() == maxsplit)
				{
					v.push_back(str.substr(bpos, str.size()-bpos));
					break;
				}
			}

			std::string::size_type epos = str.find(sep, bpos);
			if (epos == std::string::npos)
			{
				v.push_back(str.substr(bpos, str.size()-bpos));
				break;
			}
			else
			{
				v.push_back(str.substr(bpos, epos-bpos));
				bpos = epos + sep.size();
			}
		}
	}

	return v;
}

// Split string 'str' on any whitespace character (and strip all whitespace).
//
// E.g.:
// split("    aaa  bbb     ") = ["aaa", "bbb"]
//
// Same as Python's str.split() (with no argument).
inline std::vector<std::string> split(std::string str)
{
	std::istringstream iss(str);
	std::vector<std::string> v;
	std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter<std::vector<std::string> >(v));
	return v;
}

// Joins list of strings to single string.
// Equivalent to Python: s = sep.join(list)
inline std::string join(const std::vector<std::string> &list, const std::string &sep)
{
	std::string result = "";
	for (int i = 0; i < (int)list.size(); ++i)
	{
		result += list[i];
		if (i != list.size()-1)
			result += sep;
	}
	return result;
}

// -------------------------------------------------------------------------------------------------

// strip any leading/trailing character in what (if not specified, any white space)
inline std::string strip(const std::string &s, const std::string &what = " \t\n\r")
{
	std::string::size_type b = s.find_first_not_of(what);
	if (b == std::string::npos)
		return ""; // input string was all white space (or empty)
	std::string::size_type last = s.find_last_not_of(what); // if b != npos, then last is also != npos
	std::string::size_type e = last+1;
	return s.substr(b, e-b);
}

// strip any leading character in what (if not specified, any white space)
inline std::string lstrip(const std::string &s, const std::string &what = " \t\n\r")
{
	std::string::size_type b = s.find_first_not_of(what);
	if (b == std::string::npos)
		return ""; // input string was all white space (or empty)
	std::string::size_type e = s.size();
	return s.substr(b, e-b);
}

// strip any trailing character in what (if not specified, any white space)
inline std::string rstrip(const std::string &s, const std::string &what = " \t\n\r")
{
	std::string::size_type last = s.find_last_not_of(what);
	if (last == std::string::npos)
		return ""; // input string was all white space (or empty)
	std::string::size_type b = 0;
	std::string::size_type e = last+1;
	return s.substr(b, e-b);
}

// stripts whole of sub, not any char in sub
inline std::string lstripSub(const std::string &s, const std::string &sub)
{
	std::string::size_type p = s.find(sub);
	if (p == std::string::npos)
		return s;

	std::string::size_type b = p + sub.size();
	return s.substr(b, s.size() - b);
}

// stripts whole of sub, not any char in sub
inline std::string rstripSub(const std::string &s, const std::string &sub)
{
	std::string::size_type p = s.rfind(sub);
	if (p == std::string::npos)
		return s;

	return s.substr(0, p - 0);
}

// strips everything right of comment
// does not strip whitespace before comment
inline std::string stripComment(const std::string &s, const std::string &commentDelimiter = "#")
{
	// ad-hoc method to avoid considering delimiter within double quoted string as comment:
	// XXX: probably doesn't work in all cases, and not terribly efficient
	std::vector<std::string::size_type> quoteBegins;
	std::vector<std::string::size_type> quoteEnds;
	
	for (std::string::size_type i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		if (c == '\"' && (i == 0 || s[i-1] != '\\')) // ", but not \"
		{
			if (quoteBegins.size() > quoteEnds.size())
			{
				quoteEnds.push_back(i);
			}
			else
			{
				quoteBegins.push_back(i);
			}
		}
	}

	int nStrings = std::min(int(quoteBegins.size()), int(quoteEnds.size()));

	std::string::size_type searchOffset = 0;
	while (searchOffset < s.size())
	{
		// check for comment delimiter:
		std::string::size_type p = s.find(commentDelimiter, searchOffset);
		if (p == std::string::npos) // no comment delimiter found, return whole string
			return s;

		// check if inside double quoted string:
		bool isInString = false;
		for (int k = 0; k < nStrings; ++k)
		{
			if (p >= quoteBegins[k] && p < quoteEnds[k])
			{
				isInString = true;
				break;
			}
		}

		if (!isInString) // comment delimiter found, but not in inside string; return string upto comment
			return s.substr(0, p - 0);
		else // comment delimiter found, but inside string; try again with new search offset
			searchOffset = p+1;
	}

	return s;
}

// -------------------------------------------------------------------------------------------------

inline bool startswith(const std::string &s, const std::string &what)
{
	return (s.find(what) == 0);
}

inline bool endswith(const std::string &s, const std::string &what)
{
	if (what.size() > s.size())
		return false;
	return (s.rfind(what) == s.size() - what.size());
}

// -------------------------------------------------------------------------------------------------

// looks for subOut = "aa bb" in s = "xyz [aa bb] xyz" with bTag = "[" and eTag = "]"
// returns end of search (use as offset argument for subsequent calls), or npos if failed
inline std::string::size_type getSubWithinTags(const std::string &s, const std::string &bTag, const std::string &eTag, std::string &subOut, std::string::size_type offset = 0)
{
	std::string::size_type bFind = std::string::npos; // begin before bTag
	std::string::size_type eFind = std::string::npos; // end after eTag

	bFind = s.find(bTag, offset);
	if (bFind != std::string::npos)
	{
		eFind = s.find(eTag, bFind+bTag.size());
		if (eFind != std::string::npos)
		{
			eFind += eTag.size();

			std::string::size_type b = bFind + bTag.size();
			std::string::size_type e = eFind - eTag.size();
			subOut = s.substr(b, e-b);
		}
	}

	return eFind;
}

// -------------------------------------------------------------------------------------------------

#ifdef _MSC_VER
inline std::string format_arg_list(const char *fmt, va_list args)
{
	if (!fmt)
		return "";

	// NOTE:
	// MSVC's _vsnprintf() is somewhat non-standard in that it returns -1 instead of 
	// the required size for destination buffer, so we have no way of directly determining 
	// the required buffer size for the string
	//
	// As a work-around, we start with an arbitrary initial size (big enough for most cases)
	// and exponentially grow the buffer until it fits

	int result = -1;
	int length = 256; // initial size to allocate
	char *buffer = 0;
	while (result == -1)
	{
		if (buffer)
			delete[] buffer;
		buffer = new char[length + 1]; // allocate
		memset(buffer, 0, length + 1);
		result = _vsnprintf(buffer, length, fmt, args); // will write up to length characters WITHOUT null terminator (so buffer should be at least of size length+1, and manually null terminate)
		length *= 2; // double space in case doesn't fit
	}
	std::string s(buffer); // allocate, copy
	delete[] buffer;
	return s;
}
#else
inline std::string format_arg_list(const char *fmt, va_list args)
{
	if (!fmt)
		return "";

	char *buffer = 0;
	int result = vsnprintf(buffer, 0, fmt, args);
	if (result < 0)
		return ""; // error

	int length = result + 1; // result is required size without null terminator, so here we add one
	buffer = new char[length]; // allocate
	result = vsnprintf(buffer, length, fmt, args); // writes length-1 characters + null terminator
	std::string s(buffer); // allocate, copy
	delete[] buffer;
	return s;
}
#endif

// like sprintf, but without having to manually allocate/deallocate memory
// not so efficient however
inline std::string format(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::string s = format_arg_list(fmt, args);
	va_end(args);
	return s;
}

// -------------------------------------------------------------------------------------------------

inline int convStr2Int(const std::string &s)
{
	int result;
	std::stringstream ss;
	ss << s;
	ss >> result;
	if (!ss)
		throw std::runtime_error("Could not convert string \"" + s + "\" to integer.");
	return result;
}

inline std::string convInt2Str(int v)
{
	std::string result;
	std::stringstream ss;
	ss << v;
	ss >> result;
	if (!ss)
		throw std::runtime_error("Could not convert integer to string.");
	return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline std::string convHexSeq2Str(byte *src, size_t len)
{
	std::string dst = "";
	for (size_t i = 0; i < len; ++i)
	{
		dst += format("%02x", int(src[i]));
	}
	return dst;
	// (should never fail)
}

inline std::string convHexInt162Str(uint16_t src)
{
	return format("%04x", src);
	// (should never fail)
}

inline std::string convHexInt322Str(uint32_t src)
{
	return format("%08x", src);
	// (should never fail)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void convStr2HexSeq(byte *dst, size_t dstLen, std::string s)
{
	char tmp[3];
	tmp[2] = '\0';

	const char *src = s.c_str();
	const size_t srcLen = s.size();
	
	size_t dstIdx = 0;
	for (size_t srcIdx = 0; srcIdx < srcLen; srcIdx += 2)
	{
		tmp[0] = src[srcIdx+0];
		tmp[1] = src[srcIdx+1];
		
		int n;
		if (sscanf(tmp, "%x", &n) != 1)
			throw std::runtime_error("Hex string conversion failed.");
		
		if (dstIdx >= dstLen)
			throw std::runtime_error("Hex string size exceeds target data size.");

		dst[dstIdx] = (byte)n;
		++dstIdx;
	}
}

inline uint32_t convStr2HexInt(std::string s)
{
	uint32_t dst;
	if (sscanf(s.c_str(), "%x", &dst) != 1)
		throw std::runtime_error("Hex string conversion failed.");
	return dst;
}

// -------------------------------------------------------------------------------------------------

inline int convHexStr2Int(const std::string &s)
{
	unsigned int result;
	std::stringstream ss;
	ss << std::hex << s;
	ss >> result;
	if (!ss)
		throw std::runtime_error("Could not convert hex string \"" + s + "\" to integer.");
	return result;
}

inline std::string convInt2HexStr(int v, bool use0xPrefix = true)
{
	std::string result;
	std::stringstream ss;
	ss << std::hex << v;
	ss >> result;
	if (!ss)
		throw std::runtime_error("Could not convert integer to hex string.");
	if (use0xPrefix)
		result = "0x" + result;
	return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline float convStr2Float(const std::string &s)
{
	float result;
	std::stringstream ss;
	ss << s;
	ss >> result;
	if (!ss)
		throw std::runtime_error("Could not convert string \"" + s + "\" to float.");
	return result;
}

inline std::string convFloat2Str(float v)
{
	std::string result;
	std::stringstream ss;
	ss << v;
	ss >> result;
	if (!ss)
		throw std::runtime_error("Could not convert float to string.");
	return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline double convStr2Double(const std::string &s)
{
	double result;
	std::stringstream ss;
	ss << s;
	ss >> result;
	if (!ss)
		throw std::runtime_error("Could not convert string \"" + s + "\" to double.");
	return result;
}

inline std::string convDouble2Str(double v)
{
	std::string result;
	std::stringstream ss;
	ss << v;
	ss >> result;
	if (!ss)
		throw std::runtime_error("Could not convert double to string.");
	return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline bool convStr2Bool(const std::string &s)
{
	bool result;
	if (s == "true")
		return true;
	else if (s == "false")
		return false;
	else
		throw std::runtime_error("Could not convert string \"" + s + "\" to bool.");
	return result;
}

inline const char *convBool2Str(bool b)
{
	if (b)
		return "true";
	else
		return "false";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void convStr2FloatArray(std::string s, float *out, int sizeOut)
{
	s = strip(s);
	s = lstrip(s, "{");
	s = rstrip(s, "}");

	std::vector<std::string> vals = split(s, ",");

	if (len(vals) != sizeOut)
		throw std::runtime_error("array size mismatch");

	for (int i = 0; i < len(vals); ++i)
	{
		std::string v = vals[i];
		v = strip(v);
		out[i] = convStr2Float(v);
	}
}

// -------------------------------------------------------------------------------------------------

inline bool isAsciiPrintable(char c)
{
	return ((c >= 32 && c <= 126) || c == '\t' || c == '\n' || c == '\r');
}

inline bool isAsciiLetter(char c)
{
	return ((c >= 65 && c <= 90) || (c >= 97 && c <= 122));
}

// -------------------------------------------------------------------------------------------------

inline std::string asciiToPrintableAsciiReplace(std::string s)
{
	std::string printable = "";

	for (std::string::size_type i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		if (isAsciiPrintable(c))
			printable += c;
		else
			printable += '?';
	}

	return printable;
}

inline std::string utf8ToAsciiReplace(std::string s, bool printableOnly)
{
	std::string ascii = "";

	// parse UTF-8 (see e.g.: http://en.wikipedia.org/wiki/UTF-8):
	std::string::size_type i = 0;
	while (i < s.size())
	{
		unsigned char c = s[i];

		// 1 byte sequence, compatible with US-ASCII
		if (c >= 0x00 && c <= 0x7F)
		{
			if (printableOnly && !isAsciiPrintable(c)) // note: allows newlines
				ascii += '?';
			else
				ascii += (char)c;
			i += 1;
		}
		// first byte of 2 byte sequence
		else if (c >= 0xC2 && c <= 0xDF)
		{
			ascii += '?';
			i += 2;
		}
		// first byte of 3 byte sequence
		else if (c >= 0xE0 && c <= 0xEF)
		{
			ascii += '?';
			i += 3;
		}
		// first byte of 4 byte sequence
		else if (c >= 0xF0 && c <= 0xF4)
		{
			ascii += '?';
			i += 4;
		}
		// illegal UTF-8 byte sequence, ignore
		else
		{
			i += 1;
		}
	}

	return ascii;
}

#endif
