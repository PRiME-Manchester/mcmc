#include "spin1_api.h"
#include <limits.h>
#include <math.h>

// 1ms timer tick
#define TIMER_TICK_PERIOD 1000
#define CORES 64
#define N     50000
#define THIN  1000
#define RAND_NUMS 100000
#define NUM_CHIPS 4
#define LOG10_2 0.301029995663981

// If you *must* statically initialize it, here's one.
#define PCG32_INITIALIZER   { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

typedef unsigned long long uint64_t;
typedef uint uint32_t;

typedef struct
{
  uint64_t state;
  uint64_t inc;
} pcg32_random_t;

// state for global RNGs
static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

// PCG prototype definitions
void pcg32_srandom_r(pcg32_random_t *rng, uint64_t initstate, uint64_t initseq);
void pcg32_srandom(uint64_t seed, uint64_t seq);
uint32_t pcg32_random_r(pcg32_random_t *rng);
float pcg32_rng_f(pcg32_random_t *rng);
float pcg32_rng_f_pos(pcg32_random_t *rng);
uint32_t pcg32_random();

float pcg_ran_gamma (pcg32_random_t *r, float a, float b);
float pcg_ran_gamma_int (pcg32_random_t *r, unsigned int a);
float gamma_large (pcg32_random_t *r, float a);
float gamma_frac (pcg32_random_t *r, float a);
float pcg_ran_gaussian(pcg32_random_t *r, float sigma);

void radix_convert(float num, char *s);
void count_ticks(uint ticks, uint null);
uint32_t mcmc(void);
void pcg_test();
void app_done();

// void reverse(char *s, int len);
// uint itoa(uint num, char s[], uint len);
// void ftoa(float n, char *res, int precision);

// chip and core IDs
uint32_t coreID, chipID, chipID_l, r_min, r_max;
uint64_t jobs_per_core, jobs_per_chip;
uint32_t t1, t2;

typedef uint uint32_t;
typedef int  int32_t;

void c_main()
{
  // Get core and chip IDs
  coreID = spin1_get_core_id();
  chipID = spin1_get_chip_id();

  jobs_per_core = N/NUM_CHIPS/16;
  jobs_per_chip = N/NUM_CHIPS;

  switch(chipID)
  {
    case 0:   chipID_l = 0; break;
    case 1:   chipID_l = 1; break;
    case 256: chipID_l = 2; break;
    case 257: chipID_l = 3; break;
  }

  io_printf (IO_BUF, ">> Markov chain Monte Carlo - Chip %d (%d, %d) Core %d\n",
       chipID, chipID >> 8, chipID & 255, coreID);
  r_min = (coreID-1)*jobs_per_core + chipID_l*jobs_per_chip + 1;
  r_max = r_min + jobs_per_core - 1;
  // io_printf(IO_BUF, "Range %d:%d\n", r_min, r_max);
  io_printf(IO_BUF, "Generating random numbers...\n");
  
  // Set timer tick value (in microseconds)
  spin1_set_timer_tick(TIMER_TICK_PERIOD);

  // Register callbacks
  //spin1_callback_on(TIMER_TICK, count_ticks, 0);
  
  spin1_schedule_callback(pcg_test, 0, 0, 1);

  // Go
  spin1_start(SYNC_NOWAIT);

  app_done();
}

void app_done(void)
{
  uint64_t comp_per_sec;

  comp_per_sec = (uint64_t)jobs_per_core*THIN*1000/(t2-t1);
  //rng_per_sec = (uint64_t)RAND_NUMS*1000/(t2-t1);

  //io_printf(IO_BUF, "t:%d", t2-t1);
  io_printf(IO_BUF, "Time taken to generate %d computations: %d ms (%d computations/s)\n", (uint)jobs_per_core*THIN, t2-t1, (uint)comp_per_sec);
}

void pcg_test(void)
{
  uint32_t i, j;
  float x=0, y=0;
  double tmp;
  char s1[50], s2[50];
  pcg32_random_t rng;

  // Seed random number generator
  pcg32_srandom_r(&rng, 42u, 54u);

  // for(i=0; i<1000; i++)
  // {
  //   //x=pcg_ran_gaussian(&rng,1.0);
  //   x=pcg_ran_gamma(&rng, 3.0, 1.0);
  //   radix_convert(x, s1);
  //   io_printf(IO_BUF, "%s\n", s1);
  //   // radix_convert(pcg32_rng_f(&rng), s2);
  //   // io_printf(IO_BUF, "%u %s %s\n", pcg32_random_r(&rng), s1, s2);
  // }

  t1 = sv->clock_ms;

  for(i=r_min; i<r_max; i++)
  //for(i=0; i<100; i++)
  {
    for(j=0; j<THIN; j++)
    {
      x = pcg_ran_gamma(&rng, 3.0, 1.0/(y*y+4.0));
      y = 1.0/(x+1) + pcg_ran_gaussian(&rng, 1.0/sqrt(2.0*x+2.0));
    }

    radix_convert(x, s1);
    radix_convert(y, s2);
    io_printf(IO_BUF, "%d %s %s\n", i, s1, s2);
  }
  t2 = sv->clock_ms;

  spin1_exit(0);
}


// int mcmc(void)
// {
//   int i,j;
//   double x=0, y=0;

//   gsl_rng *r = gsl_rng_alloc(gsl_rng_mt19937);

//   io_printf(IO_BUF, "Iter x y\n");
//   for (i=0; i<N; i++)
//   {
//     for (j=0; j<THIN; j++)
//     {
//       x = gsl_ran_gamma(r, 3.0, 1.0/(y*y+4));
//       y = 1.0 / (x+1) + gsl_ran_gaussian(r,1.0/sqrt(2*x+2));
//     }
//     io_printf(IO_BUF, "%d %f %f\n",i,x,y);
//   }

//   return 0;
// }

float pcg32_rng_f(pcg32_random_t *rng)
{
    return (float)pcg32_random_r(rng)/(UINT_MAX+1.0);
}

float pcg32_rng_f_pos(pcg32_random_t *rng)
{
    uint32_t r;
    do {
      r = pcg32_random_r(rng);
    } while(r==0);

    return (float)r/(UINT_MAX+1.0);
}

// Generate a uniformly distributed 32-bit random number
uint32_t pcg32_random_r(pcg32_random_t *rng)
{
    uint64_t oldstate = rng->state;
    uint32_t xorshifted, rot;

    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);

    // Calculate output function (XSH RR), uses old state for max ILP
    xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    rot = oldstate >> 59u;

    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t pcg32_random()
{
    return pcg32_random_r(&pcg32_global);
}

// Seed the rng.  Specified in two parts, state initializer and a
// sequence selection constant (a.k.a. stream id)
void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

void pcg32_srandom(uint64_t seed, uint64_t seq)
{
    pcg32_srandom_r(&pcg32_global, seed, seq);
}

float pcg_ran_gamma (pcg32_random_t *r, float a, float b)
{
  /* assume a > 0 */
  unsigned int na = floor (a);

  if(a >= UINT_MAX) 
  {
    return b * (gamma_large (r, floor (a)) + gamma_frac (r, a - floor (a)));
  }
  else if (a == na)
  {
    return b * pcg_ran_gamma_int (r, na);
  }
  else if (na == 0)
  {
    return b * gamma_frac (r, a);
  }
  else
  {
    return b * (pcg_ran_gamma_int (r, na) + gamma_frac (r, a - na)) ;
  }
}

float pcg_ran_gamma_int (pcg32_random_t *r, unsigned int a)
{
  if (a < 12)
  {
    unsigned int i;
    double prod = 1;

    for (i = 0; i < a; i++)
      prod *= pcg32_rng_f_pos(r);

    /* Note: for 12 iterations we are safe against underflow, since
 the smallest positive random number is O(2^-32). This means
 the smallest possible product is 2^(-12*32) = 10^-116 which
 is within the range of double precision. */

    return -log (prod);
  }
  else
  {
    return gamma_large (r, (double)a);
  }
}

float gamma_large (pcg32_random_t *r, float a)
{
  /* Works only if a > 1, and is most efficient if a is large

     This algorithm, reported in Knuth, is attributed to Ahrens.  A
     faster one, we are told, can be found in: J. H. Ahrens and
     U. Dieter, Computing 12 (1974) 223-246.  */

  float sqa, x, y, v;
  sqa = sqrt (2 * a - 1);
  do
  {
    do
    {
      y = tan (M_PI * pcg32_rng_f(r));
      x = sqa * y + a - 1;
    }
    while (x <= 0);
    v = pcg32_rng_f(r);
  }
  while (v > (1 + y * y) * exp ((a - 1) * log (x / (a - 1)) - sqa * y));

  return x;
}

float gamma_frac (pcg32_random_t *r, float a)
{
  /* This is exercise 16 from Knuth; see page 135, and the solution is
     on page 551.  */

  float p, q, x, u, v;
  p = M_E / (a + M_E);
  do
  {
    u = pcg32_rng_f (r);
    v = pcg32_rng_f (r);

    if (u < p)
    {
      x = exp ((1 / a) * log (v));
      q = exp (-x);
    }
    else
    {
      x = 1 - log (v);
      q = exp ((a - 1) * log (x));
    }
  }
  while (pcg32_rng_f (r) >= q);

  return x;
}

float pcg_ran_gaussian(pcg32_random_t *r, float sigma)
{
  float x, y, r2; //, result;
  char s1[50], s2[50], s3[50], s4[50];

  do
  {
    /* choose x,y in uniform square (-1,-1) to (+1,+1) */
    x = -1.0 + 2.0 * pcg32_rng_f(r);
    y = -1.0 + 2.0 * pcg32_rng_f(r);

    /* see if it is in the unit circle */
    r2 = x * x + y * y;
  }
  while (r2 > 1.0 || r2 == 0);

  radix_convert(x, s1);
  radix_convert(y, s2);
  radix_convert(r2, s3);
  radix_convert(-3.2654,s4);
  //radix_convert(log(r2),s4);
  //io_printf(IO_BUF, "x:%s y:%s r2:%s log(r2)/r2:%s ", s1, s2, s3, s4);

  /* Box-Muller transform */
  // result = sigma * y * sqrt (-2.0*log(r2)/r2);
  // radix_convert(result,s1);
  // io_printf(IO_BUF, "%s\n", s1);

  return sigma * y * sqrt (-2.0*log(r2)/r2);
}


// double gsl_ran_gamma(const gsl_rng *r, const double a, const double b)
// {
//   /* assume a > 0 */
//   if (a < 1)
//   {
//     double u = gsl_rng_uniform_pos(r);
//     return gsl_ran_gamma(r, 1.0+a, b) * pow(u, 1.0/a);
//   }


//   double x, v, u;
//   double d = a - 1.0 / 3.0;
//   double c = (1.0/3.0)/sqrt(d);

//   while (1)
//   {
//     do
//     {
//       x = gsl_ran_gaussian_ziggurat(r, 1.0);
//       v = 1.0 + c*x;
//     }
//     while (v <= 0);

//     v = v*v*v;
//     u = gsl_rng_uniform_pos(r);

//     if (u < 1 - 0.0331*x*x*x*x) 
//       break;

//     if (log(u) < 0.5*x*x + d*(1 - v + log (v)))
//       break;
//   }
  
//   return b*d*v;
// }

// double gsl_ran_gaussian_ziggurat (const gsl_rng *r, const double sigma)
// {
//   uint64_t i, j;
//   int sign;
//   double x, y;

//   const uint64_t range = r->type->max - r->type->min;
//   const uint64_t offset = r->type->min;

//   while (1)
//   {
//     if (range >= 0xFFFFFFFF)
//     {
//       uint64_t k = gsl_rng_get(r) - offset;
//       i = (k & 0xFF);
//       j = (k >> 8) & 0xFFFFFF;
//     }
//     else if (range >= 0x00FFFFFF)
//     {
//       uint64_t k1 = gsl_rng_get(r) - offset;
//       uint64_t k2 = gsl_rng_get(r) - offset;
//       i = (k1 & 0xFF);
//       j = (k2 & 0x00FFFFFF);
//     }
//     else
//     {
//       i = gsl_rng_uniform_int (r, 256); /*  choose the step */
//       j = gsl_rng_uniform_int (r, 16777216);  /* sample from 2^24 */
//     }

//     sign = (i & 0x80) ? +1 : -1;
//     i &= 0x7f;

//     x = j * wtab[i];

//     if (j < ktab[i])
//       break;

//     if (i < 127)
//     {
//       double y0, y1, U1;
//       y0 = ytab[i];
//       y1 = ytab[i + 1];
//       U1 = gsl_rng_uniform (r);
//       y = y1 + (y0 - y1) * U1;
//     }
//     else
//     {
//       double U1, U2;
//       U1 = 1.0 - gsl_rng_uniform (r);
//       U2 = gsl_rng_uniform (r);
//       x = PARAM_R - log (U1) / PARAM_R;
//       y = exp (-PARAM_R * (x - 0.5 * PARAM_R)) * U2;
//     }

//     if (y < exp (-0.5 * x * x))
//       break;
//   }

//   return sign * sigma * x;
// }

// // reverses a string 's' of length 'len'
// void reverse(char *s, int len)
// {
//   int i=0, j=len-1;
//   char temp;
//   while (i<j)
//   {
//     temp = s[i];
//     s[i] = s[j];
//     s[j] = temp;
//     i++;
//     j--;
//   }
// }

// // Converts a given integer num to string str[].  len is the number
// // of digits required in output. If len is more than the number
// // of digits in num, then 0s are added at the beginning.
// uint itoa(uint num, char s[], uint len)
// {
//   uint i = 0;

//   do {
//     s[i++] = '0' + num%10;
//     num /= 10;
//   } while (num>0);

//   // If number of digits required is more, then
//   // add 0s at the beginning
//   while (i < len)
//     s[i++] = '0';

//   reverse(s, i);
//   s[i] = '\0';
//   return i;
// }


// // Converts a floating point number to string.
// void ftoa(float n, char *res, int precision)
// {
//   // Extract integer part
//   int ipart = (int)n;

//   // Extract floating part
//   float fpart = n - (int)n;

//   // convert integer part to string
//   uint i = itoa(ipart, res, 0);

//   // check for display option after point
//   if (precision != 0)
//   {
//     res[i] = '.';  // add dot

//     // This computes pow(10,precision)
//     uint m=1;
//     if (precision>0)
//     for (int i=0; i<precision; i++)
//       m*=10;

//     // Get the value of fraction part upto given no.
//     // of points after dot. The third parameter is needed
//     // to handle cases like 233.007
//     fpart = fpart * m;

//     itoa((int)fpart, res+i+1, precision);
//   }
// }

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

  if ((int)dec.m==0)
  {
    dec.m*=10;
    dec.e--;
  }

  if (dec.e)
    io_printf(s, "%d.%de%d", dec.sign*(int)dec.m, (int)((dec.m-(int)dec.m)*1000000), dec.e);
  else
    io_printf(s, "%d.%d", dec.sign*(int)dec.m, (int)((dec.m-(int)dec.m)*1000000));
}

