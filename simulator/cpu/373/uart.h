/* Mulator - An extensible {e,si}mulator
 * Copyright 2011-2020 Pat Pannuto <pat.pannuto@gmail.com>
 *
 * Licensed under either of the Apache License, Version 2.0
 * or the MIT license, at your option.
 */

#ifndef UART_H
#define UART_H

#include "core/common.h"

#ifndef PP_STRING
#define PP_STRING "URT"
#include "core/pretty_print.h"
#endif

// CONFIGURATION
#define POLL_UART_PORT 4100
//#define POLL_UART_BUFSIZE 16
//#define POLL_UART_BAUD 1200
#define POLL_UART_BUFSIZE 8192
#define POLL_UART_BAUD 9600

#endif // UART_H
