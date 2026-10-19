/* Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 *
 * fpret_test: an FPU program (-m68881) gets the right double back from
 * ixemul's double-returning functions (fpret.py), from libc.a's rand48 and
 * from libixcompat/libm. Before the FPU archives every one of these read a
 * stale fp0. Run on the Amiga; prints [OK] or [FAIL] per call, exits 1 on
 * any failure.
 *
 * No <math.h>: with -m68881 it inlines most functions (math-68881.h), and
 * the point is the library call. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double pow(double, double);
double ldexp(double, int);
double frexp(double, int *);
double modf(double, double *);
double sqrt(double);
double round(double);
double log2(double);

static int fails;

static void expect(const char *what, double got, double want)
{
  int ok = got == want;
  printf("[%s] %s = %.17g (want %.17g)\n", ok ? "OK" : "FAIL", what, got, want);
  if (!ok)
    fails++;
}

static void expect_unit(const char *what, double got)
{
  int ok = got >= 0.0 && got < 1.0;
  printf("[%s] %s = %.17g (want [0, 1))\n", ok ? "OK" : "FAIL", what, got);
  if (!ok)
    fails++;
}

int main(void)
{
  double ip;
  int e;
  unsigned short xs[3] = { 1, 2, 3 };
  /* a value in fp0 that none of the results equals: a stale fp0 fails */
  volatile double poison = 123456.75;

  ip = poison * 2.0;
  expect("strtod(\"2.5\")", strtod("2.5", NULL), 2.5);
  ip = poison * 2.0;
  expect("atof(\"-1.25\")", atof("-1.25"), -1.25);
  ip = poison * 2.0;
  expect("pow(2, 10)", pow(2.0, 10.0), 1024.0);
  ip = poison * 2.0;
  expect("ldexp(0.75, 4)", ldexp(0.75, 4), 12.0);
  ip = poison * 2.0;
  expect("frexp(12)", frexp(12.0, &e), 0.75);
  expect("frexp(12) exponent", e, 4);
  ip = poison * 2.0;
  expect("modf(3.25)", modf(3.25, &ip), 0.25);
  expect("modf(3.25) integral", ip, 3.0);
  ip = poison * 2.0;
  expect("difftime(10, 4)", difftime((time_t)10, (time_t)4), 6.0);
  ip = poison * 2.0;
  expect("sqrt(16)", sqrt(16.0), 4.0);
  ip = poison * 2.0;
  expect("round(2.5)", round(2.5), 3.0);
  ip = poison * 2.0;
  expect("log2(8)", log2(8.0), 3.0);
  srand48(1);
  ip = poison * 2.0;
  expect_unit("drand48()", drand48());
  ip = poison * 2.0;
  expect_unit("erand48()", erand48(xs));
  (void)ip;
  printf("fpret_test: %s\n", fails ? "FAIL" : "PASS");
  return fails ? 1 : 0;
}
