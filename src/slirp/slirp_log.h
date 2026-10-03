/*
 * slirp_log.h - gate slirp/RPC informational output behind the 1989
 * "Debugging" toggle (Advanced tab).
 *
 * The slirp networking stack and its NFS/RPC/NetInfo servers print a lot of
 * diagnostic output directly with printf()/perror(). Route it through these
 * helpers so the terminal stays quiet unless debugging is enabled.
 */
#ifndef SLIRP_LOG_H
#define SLIRP_LOG_H

#include <stdio.h>
#include <stdbool.h>

/* Provided by the emulator's log module (src/debug/log.c). The ditool
 * console tool links a stub that always returns true. */
extern bool Log_DebugEnabled(void);

#define slirp_printf(...) \
	do { if (Log_DebugEnabled()) printf(__VA_ARGS__); } while (0)
#define slirp_perror(msg) \
	do { if (Log_DebugEnabled()) perror(msg); } while (0)

#endif /* SLIRP_LOG_H */
