#pragma once

#define ULONG_MAX __UINT64_MAX__
#define LONG_MAX  __INT64_MAX__
#define LONG_MIN  (-LONG_MAX - 1L)

/* Minimum and maximum values a `signed int' can hold.  */
#undef INT_MAX
#define INT_MAX __INT_MAX__
#undef INT_MIN
#define INT_MIN (-INT_MAX - 1)

/* Maximum value an `unsigned int' can hold.  (Minimum is 0).  */
#undef UINT_MAX
#define UINT_MAX (INT_MAX * 2U + 1U)