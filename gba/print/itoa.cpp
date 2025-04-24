#include "itoa.h"

static const constexpr char ITOA_CHARS[] = "fedcba9876543210123456789abcdef";

char *itoa(uint32_t value, char *result, uint32_t base)
{
	// check that the base if valid
	if (base < 2 || base > 16)
	{
		*result = '\0';
		return result;
	}
	char *ptr = result;
	char *ptr1 = result;
	uint32_t tmp_value;
	uint32_t count = 0;
	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = ITOA_CHARS[15 + (tmp_value - value * base)];
		count++;
	} while (value);
	if (base != 10)
	{
		while (count++ < 8)
		{
			*ptr++ = '0';
		};
	}
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		char tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

char *itoa(int32_t value, char *result, uint32_t base)
{
	// check that the base if valid
	if (base < 2 || base > 16)
	{
		*result = '\0';
		return result;
	}
	char *ptr = result;
	char *ptr1 = result;
	bool sign = value < 0;
	value = value < 0 ? -value : value;
	int32_t tmp_value;
	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = ITOA_CHARS[15 + (tmp_value - value * base)];
	} while (value);
	// Apply negative sign
	if (sign)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		char tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

char *itoa(uint64_t value, char *result, uint32_t base)
{
	// check that the base if valid
	if (base < 2 || base > 16)
	{
		*result = '\0';
		return result;
	}
	char *ptr = result;
	char *ptr1 = result;
	uint64_t tmp_value;
	uint32_t count = 0;
	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = ITOA_CHARS[15 + (tmp_value - value * base)];
		count++;
	} while (value);
	if (base != 10)
	{
		while (count++ < 8)
		{
			*ptr++ = '0';
		};
	}
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		char tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

char *itoa(int64_t value, char *result, uint32_t base)
{
	// check that the base if valid
	if (base < 2 || base > 16)
	{
		*result = '\0';
		return result;
	}
	char *ptr = result;
	char *ptr1 = result;
	bool sign = value < 0;
	value = value < 0 ? -value : value;
	int64_t tmp_value;
	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = ITOA_CHARS[15 + (tmp_value - value * base)];
	} while (value);
	// Apply negative sign
	if (sign)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		char tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

char *btoa(bool value, char *result)
{
	static const char *trueString = "true";
	static const char *falseString = "false";
	auto src = value ? trueString : falseString;
	auto ptr = result;
	while (*src != '\0')
	{
		*ptr++ = *src++;
	}
	*ptr = '\0';
	return result;
}

char *fptoa(int32_t value, char *result, uint32_t BITSF, uint32_t precision)
{
	static const char chars[] = "0123456789----------";
	// make value absolute
	int32_t tmp_value = static_cast<uint32_t>(value < 0 ? -value : value);
	int32_t intPart = tmp_value >> BITSF;
	int32_t fractPart = (tmp_value & ((1 << BITSF) - 1));
	// convert integer part
	char *ptr = result;
	do
	{
		tmp_value = intPart;
		intPart /= 10;
		*ptr++ = chars[tmp_value - intPart * 10];
	} while (intPart);
	// apply negative sign
	if (value < 0)
		*ptr++ = '-';
	// store end of int string for appending fraction
	char *intEnd = ptr--;
	// swap int string back-to-front
	char *ptr1 = result;
	while (ptr1 < ptr)
	{
		char tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	// add decimal point
	*intEnd++ = '.';
	// convert fractional part with rounding
	if (precision > 0)
	{
		const uint32_t BITSQ_MASK = ~((1 << BITSF) - 1);
		const int32_t HALF = int32_t(1) << (BITSF - 1);
		int32_t multiplier = 1;
		for (uint32_t i = 0; i < precision; i++)
		{
			multiplier *= 10;
		}
		fractPart *= multiplier;
		fractPart += (value < 0) ? (-HALF) : HALF; // -0.5 / +0.5
		fractPart &= BITSQ_MASK;				   // trunc
		for (uint32_t i = 0; i < precision; i++)
		{
			multiplier /= 10;
			auto number = (fractPart / multiplier) & BITSQ_MASK;
			*intEnd++ = chars[number >> BITSF];
			fractPart -= number * multiplier;
		}
	}
	// convert fractional part w/o rounding
	else
	{
		do
		{
			fractPart *= 10;
			*intEnd++ = chars[fractPart >> BITSF];
			fractPart &= ((1 << BITSF) - 1);
		} while (fractPart);
	}
	// null-terminate
	*intEnd = '\0';
	return result;
}
