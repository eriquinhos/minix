#ifndef PROC_H
#define PROC_H

/* Here is the declaration of the process table.  It contains all process
 * data, including registers, flags, scheduling priority, memory map, 
 * accounting, message passing (IPC) information, etc.
 * 
 * Many assembly code routines reference fields in it.  The offsets to these
 * fields are defined in the assembler include file sconst.h.  When changing
 * struct proc, be sure to change sconst.h to match.
 */
#include <minix/com.h>
#include <minix/portio.h>
#include <machine/archtypes.h>
#include "archconst.h"
#include "config.h"

struct proc {
  struct stackframe_s p_reg;	/* process' registers saved in stack frame */

#if (CHIP == INTEL)
  reg_t p_ldt_sel;		/* selector in gdt with ldt base and limit */
  struct segdesc_s p_ldt[2+NR_REMOTE_SEGS]; /* CS, DS and remote segments */
#endif

  proc_nr_t p_nr;		/* number of this process (for fast access) */
  struct priv *p_priv;		/* system privileges structure */
  short p_rts_flags;		/* SENDING, RECEIVING, etc. */

  /* Guaranteed Scheduling fields - FIXED: Using integer instead of double */
  u32_t justica;		/* Justice/fairness metric (scaled by 1000) */

  char p_priority;		/* current scheduling priority */
  char p_max_priority;		/* maximum scheduling priority */
  char p_ticks_left;		/* number of scheduling ticks left */
  char p_quantum_size;		/* quantum size in ticks */

  struct mem_map p_memmap[NR_LOCAL_SEGS];   /* memory map (T, D, S) */

  clock_t p_user_time;		/* user time in ticks */
  clock_t p_sys_time;		/* sys time in ticks */

  struct proc *p_nextready;	/* pointer to next ready process */
  struct proc *p_caller_q;	/* head of list of procs wishing to send */
  struct proc *p_q_link;	/* link to next proc wishing to send */
  message *p_messbuf;		/* pointer to passed message buffer */
  proc_nr_t p_getfrom_e;	/* from whom does process want to receive? */
  proc_nr_t p_sendto_e;		/* to whom does process want to send? */

  sigset_t p_pending;		/* bit map for pending kernel signals */

  char p_name[P_NAME_LEN];	/* name of the process, including \0 */

  endpoint_t p_endpoint;	/* endpoint number, generation-aware */

#if DEBUG_ENABLE_IPC_WARNINGS
  int p_magic;			/* magic number to check validity of proc */
#endif

  /* Accounting fields */
  struct {
    u64_t enter_queue;		/* time when entered queue */
    u64_t time_in_queue;	/* total time in queue */
    unsigned long dequeues;	/* number of times dequeued */
    unsigned long ipc_sync;	/* number of sync IPC calls */
    unsigned long ipc_async;	/* number of async IPC calls */
    unsigned long preempted;	/* number of times preempted */
  } p_accounting;

  int p_schedules;
};

/* Bits for the runtime flags. A process is runnable iff p_rts_flags == 0. */
#define RTS_SLOT_FREE	0x01	/* process slot is free */
#define RTS_PROC_STOP	0x02	/* process has been stopped */
#define RTS_SENDING	0x04	/* process blocked trying to send */
#define RTS_RECEIVING	0x08	/* process blocked trying to receive */
#define RTS_SIGNALED	0x10	/* set when new kernel signal arrives */
#define RTS_SIG_PENDING	0x20	/* unready while signal being processed */
#define RTS_P_STOP	0x40	/* set when process is being traced */
#define RTS_NO_PRIV	0x80	/* keep process from running */
#define RTS_NO_ENDPOINT	0x100	/* process cannot send or receive messages */
#define RTS_VMINHIBIT	0x200	/* not scheduled until pagetable set by VM */
#define RTS_PAGEFAULT	0x400	/* process has unhandled pagefault */
#define RTS_VMREQUEST	0x800	/* originator of vm memory request */
#define RTS_VMREQTARGET 0x1000	/* target of vm memory request */
#define RTS_SYS_LOCK	0x2000	/* temporary process lock flag */
#define RTS_PREEMPTED	0x4000	/* this process was preempted by enqueue() */
#define RTS_BOOTINHIBIT	0x8000	/* this process cannot be scheduled until
				 * boot completes
				 */

/* Misc flags */
#define MF_REPLY_PEND	0x001	/* reply to IPC_REQUEST is pending */
#define MF_VIRT_TIMER	0x002	/* process-virtual timer is running */
#define MF_PROF_TIMER	0x004	/* process-virtual profile timer is running */
#define MF_ASYNMSG	0x010	/* Asynchonous message pending */
#define MF_FULLVM	0x020
#define MF_DELIVERMSG   0x040	/* Copy message for him before running */
#define MF_SIG_DELAY	0x080	/* Send signal when no longer sending */
#define MF_SC_ACTIVE	0x100	/* Syscall tracing: in a system call */
#define MF_SC_DEFER	0x200	/* Syscall tracing: deferred system call */
#define MF_SC_TRACE	0x400	/* Syscall tracing: trigger syscall events */
#define MF_SENDING_FROM_KERNEL 0x800 /* send() call from kernel on behalf of
				      * this process
				      */
#define MF_CONTEXT_SET	0x1000	/* Don't clobber context until next kernel
				 * entry
				 */
#define MF_SENDA_VM_MISS 0x2000	/* Pending senda() failed due to VM issues */
#define MF_MSGFAILED	0x4000	/* Pending message failed to be delivered */

/* Magic process table addresses. */
#define BEG_PROC_ADDR ((struct proc *) proc)
#define END_PROC_ADDR ((struct proc *) (proc + NR_TASKS + NR_PROCS))

#define proc_addr(n)      ((struct proc *) &proc[NR_TASKS + (n)])
#define proc_nr(p) 	  ((p)->p_nr)

#define isokprocn(n)      ((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS)
#define isemptyn(n)       isemptyp(proc_addr(n)) 
#define isemptyp(p)       ((p)->p_rts_flags == RTS_SLOT_FREE)
#define iskernelp(p)	  ((p) < BEG_USER_ADDR)
#define iskerneln(n)	  ((n) < 0)
#define isuserp(p)        isusern((p) - proc)
#define isusern(n)        ((n) >= 0)

/* The process table and pointers to process table slots. The pointers allow
 * faster access because now a process entry can be found by indexing the
 * pproc_addr array, while accessing an element of the proc array requires a
 * multiplication.
 */
EXTERN struct proc proc[NR_TASKS + NR_PROCS];	/* process table */
EXTERN struct proc *rdy_head[NR_SCHED_QUEUES]; /* ptrs to ready list headers */
EXTERN struct proc *rdy_tail[NR_SCHED_QUEUES]; /* ptrs to ready list tails */

#endif /* PROC_H */
