/* <math.h> must pull ixemul's own math-68881.h even when gcc's private
   include dir, which ships an older math-68881.h without the float
   functions, sits ahead of ixemul's include dir on the search path.
   Before the fix this failed with "implicit declaration of function
   'log10f'". */
#include <math.h>

float order_check(float x)
{
  return log10f(x) + fabsf(x) + sqrtf(x);
}
