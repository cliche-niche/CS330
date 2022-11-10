#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
//#define TIMER_INTERVAL 1000000
#define TIMER_INTERVAL 100000
#define SCHED_NPREEMPT_FCFS 0
#define SCHED_NPREEMPT_SJF 1
#define SCHED_PREEMPT_RR 2
#define SCHED_PREEMPT_UNIX 3
#define SCHED_PARAM_SJF_A_NUMER 1
#define SCHED_PARAM_SJF_A_DENOM 2
#define SCHED_PARAM_CPU_USAGE 200
