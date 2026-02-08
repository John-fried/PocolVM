/* common.h -- list of common types, macros, and utility that used in various program */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOL_COMMON_H
#define POCOL_COMMON_H

#ifndef ONE_SOURCE
# define ONE_SOURCE 1
#endif

#if ONE_SOURCE
#define ST_INLN static inline
#define ST_FUNC static
#define ST_DATA static
#else
#define ST_INLN
#define ST_FUNC
#define ST_DATA extern
#endif

#endif /* POCOL_COMMON_H */
