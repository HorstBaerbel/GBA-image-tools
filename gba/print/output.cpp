#include "output.h"

#include "itoa.h"

#include <cstdint>
#include <cstring>

namespace Debug
{

	static char PrintBuffer[64] = {0};

#define MGBA_REG_DEBUG_ENABLE (volatile uint16_t *)0x4FFF780
#define MGBA_REG_DEBUG_FLAGS (volatile uint16_t *)0x4FFF700
#define MGBA_REG_DEBUG_STRING (char *)0x4FFF600

	// mGBA debug output through registers
	void print(const char *s)
	{
		*MGBA_REG_DEBUG_ENABLE = 0xC0DE;
		strcpy(MGBA_REG_DEBUG_STRING, s);
		*MGBA_REG_DEBUG_FLAGS = 4 | 0x100;
		//*MGBA_REG_DEBUG_ENABLE = 0;
	}
	/*
		// VisualBoyAdvance debug output through SWI 0xff
		#if defined(__thumb__) || defined(_M_ARMT)
			void print(const char *s) // THUMB code
			{
				asm volatile("mov r0, %0;"
							"swi 0xff;"
							: // no ouput
							: "r" (s)
							: "r0");
			}
		#else
			void print(const char *s) // ARM code
			{
				asm volatile("mov r0, %0;"
							"swi 0xff0000;"
							: // no ouput
							: "r" (s)
							: "r0");
			}
		#endif
	*/

	void printf(const char *fmt...)
	{
		va_list args;
		va_start(args, fmt);
		char *buffer = PrintBuffer;
		buffer[0] = '\0';
		bool expectType = false;
		while (*fmt != '\0')
		{
			if (expectType)
			{
				expectType = false;
				if (*fmt == 'b')
				{
					// note automatic conversion to integral type
					auto c = va_arg(args, int32_t);
					buffer = btoa(c != 0, buffer);
				}
				else if (*fmt == 'c')
				{
					// note automatic conversion to integral type
					auto c = va_arg(args, int32_t);
					buffer = itoa(c, buffer, 10);
				}
				else if (*fmt == 'd')
				{
					auto i = va_arg(args, int32_t);
					buffer = itoa(i, buffer, 10);
				}
				else if (*fmt == 'f')
				{
					auto f = va_arg(args, int32_t);
					buffer = fptoa(f, buffer, 16, 2);
				}
				else if (*fmt == 'x')
				{
					auto i = va_arg(args, int32_t);
					buffer = itoa(i, buffer, 16);
				}
				else
				{
					buffer[0] = *fmt;
					buffer[1] = '\0';
				}
			}
			else if (*fmt == '%')
			{
				expectType = true;
			}
			else
			{
				buffer[0] = *fmt;
				buffer[1] = '\0';
			}
			++fmt;
			// find new end of string
			while (*buffer != '\0' && (buffer - PrintBuffer) < 64)
			{
				buffer++;
			}
		}
		va_end(args);
		print(PrintBuffer);
	}

}
