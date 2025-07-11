/* This table has one slot per process.  It contains scheduling information
 * for each process.
 */
#include <limits.h>

#include <minix/bitmap.h>

/* EXTERN should be extern except in main.c, where we want to keep the struct */
#ifdef _MAIN
#undef EXTERN
#define EXTERN
#endif

#ifndef CONFIG_SMP
#define CONFIG_MAX_CPUS 1
#endif

/**
 * We might later want to add more information to this table, such as the
 * process owner, process group or cpumask.
 */

EXTERN struct schedproc {
	endpoint_t endpoint;	/* process endpoint id */
	endpoint_t parent;	/* parent endpoint id */
	unsigned flags;		/* flag bits */

	/* User space scheduling */
	unsigned max_priority;	/* this process' highest allowed priority */
	unsigned priority;		/* the process' current priority */
	unsigned time_slice;		/* this process's time slice */
	unsigned cpu;		/* what CPU is the process running on */
	bitchunk_t cpu_mask[BITMAP_CHUNKS(CONFIG_MAX_CPUS)]; /* what CPUs is the
								process allowed
								to run on */
	
	/* Guaranteed Scheduling specific fields */
	double cpu_share;		/* guaranteed CPU share (0.0 to 1.0) */
	clock_t cpu_time_used;		/* total CPU time used by process */
	clock_t start_time;		/* when process started */
	double fairness_ratio;		/* current fairness ratio */
	unsigned total_quanta;		/* total number of quanta received */
	clock_t last_update;		/* last time statistics were updated */
} schedproc[NR_PROCS];

/* Flag values */
#define IN_USE		0x00001	/* set when 'schedproc' slot in use */
