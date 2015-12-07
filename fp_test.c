#include "spin1_api.h"
#include <math.h>

// 1ms timer tick
#define TIMER_TICK_PERIOD 1000
#define LOG10_2 0.301029995663981

void radix_convert(float num, char *s);

typedef uint uint32_t;
typedef int  int32_t;

uint32_t coreID, chipID;

int c_main(void)
{
  char s[100];

  coreID = spin1_get_core_id();
  chipID = spin1_get_chip_id();

  io_printf (IO_BUF, ">> Floating-point test - Chip %d (%d, %d) Core %d\n", chipID, chipID >> 8, chipID & 255, coreID);

  radix_convert(342.1365, s);
  io_printf(IO_BUF, "%s\n", s);

  radix_convert(-342.1365, s);
  io_printf(IO_BUF, "%s\n", s);

  radix_convert(6.42e23, s);
  io_printf(IO_BUF, "%s\n", s);

  radix_convert(-6.42e23, s);
  io_printf(IO_BUF, "%s\n", s);

//  io_printf(IO_BUF, "Hello: %f %s\n", num, s);

  // Set timer tick value (in microseconds)
  spin1_set_timer_tick(TIMER_TICK_PERIOD);

  // Go
  spin1_start(SYNC_NOWAIT);

  return 0;
}

void radix_convert(float num, char *s)
{
	typedef struct {
		int32_t  sign;
		double   m;
	    int32_t  e;
	} format;

	format bin,dec;

	uint32_t num_int = *(unsigned int *)&num;
	uint32_t m_hex, mask=0x00400000;
	double pow2=0.5, pow10;

	bin.m = 1.0;

	dec.sign = 1;
	if (num_int&0x80000000)
		dec.sign = -1;

	bin.e = ((num_int&0x7f800000)>>23) - 127;

	m_hex = num_int&0x007fffff;
	for(uint32_t i=0; i<23; i++)
	{
		if (mask&m_hex)
			bin.m += pow2;

		mask >>= 1;
		pow2 /= 2.0;
	}

	pow10 = bin.e * LOG10_2;
	dec.e = (int)pow10;
	dec.m = bin.m*pow(10, pow10-dec.e);

	//io_printf(IO_BUF, "%d %d %d\n", dec.sign*(int)dec.m, (int)((dec.m-(int)dec.m)*1000000), dec.e);
	//io_printf(IO_BUF, "%d.%de%d\n", dec.sign*(int)dec.m, (int)((dec.m-(int)dec.m)*1000000), dec.e);
	io_printf(IO_BUF, "%x\n", num_int);
	io_printf(s, "%d.%de%d", dec.sign*(int)dec.m, (int)((dec.m-(int)dec.m)*1000000), dec.e);
}



