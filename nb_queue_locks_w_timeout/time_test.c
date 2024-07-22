/*  Copyright (c) Michael L. Scott, 2001, 2002

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <thread.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "atomic_ops.h"
#include "thread_data.h"
#include "tas.h"
#include "mcs.h"
#include "clh.h"

#ifdef __GNUC__
bool main_uses_cc = false;
#else
bool main_uses_cc = true;
#endif

#define DEFAULT_NTHREADS 4
#define DEFAULT_ITERATIONS 10000
#define DEFAULT_PATIENCE 1000       /* nanoseconds */

#define MAX_THREADS 100

int iterations = DEFAULT_ITERATIONS;
int nthreads = DEFAULT_NTHREADS;
int max_processors = 1000000;
    /* maximum number of threads that can be guaranteed their own
        processor.  Do not run blocking algorithms with more threads
        than this. */
hrtime_t patience = DEFAULT_PATIENCE;
int try_percent = 100;
int non_critical_work = 0;
int critical_work = 0;

int padding8[32];

static volatile int foo = 0;    /* dummy variable for killing time */

int padding0[32];

tatas_lock      tatas_L = 0;
mcs_plain_lock  mcs_PL  = 0;
mcs_try_lock    mcs_TL  = 0;
mcs2_try_lock   mcs2_TL = {0, 0};

int padding1[32];

clh_cc_qnode   initial_clh_plain_qnode          = {available, 0};
clh_numa_qnode initial_clh_plain_numa_qnode     = {available_p, 0};
clh_cc_qnode   initial_clh_try_qnode            = {available, 0};
clh_numa_qnode initial_clh_try_numa_qnode       = {available_p, 0};
clh_cc_qnode   initial_clh_try_swap_qnode       = {available, 0};
clh_numa_qnode initial_clh_try_numa_swap_qnode  = {available_p, 0};

int padding2[32];

clh_cc_lock    clh_PL   = &initial_clh_plain_qnode;
clh_numa_lock  clh_PNL  = &initial_clh_plain_numa_qnode;
clh_cc_lock    clh_TL   = &initial_clh_try_qnode;
clh2_cc_lock   clh2_TL  = {0, 0};
clh_numa_lock  clh_TNL  = &initial_clh_try_numa_qnode;
clh_cc_lock    clh_TSL  = &initial_clh_try_swap_qnode;
clh_numa_lock  clh_TNSL = &initial_clh_try_numa_swap_qnode;

int padding3[32];

static struct {
    clh_cc_qnode clh_cc_qn;
    int padding4[30];
} clh_cc_qnodes[MAX_THREADS];
static struct {
    clh_numa_qnode clh_numa_qn;
    int padding5[30];
} clh_numa_qnodes[MAX_THREADS];

int padding6[32];

static struct {
    int count;
    int padding7[32];
} successes [MAX_THREADS];

static volatile unsigned long counter;
static volatile int winner;
static volatile unsigned long handoffs;    /* cross-thread lock acquisitions */
static hrtime_t start_time, end_time;

volatile unsigned long special_events[NUM_SPECIALS];
char *special_names[] = {
    "FIFO timing windows",
    "NB stack timing windows",
    "calls to malloc",
    0
};

void barrier(int tid)
{
    static volatile unsigned long count = 0;
    static volatile bool sense = false;
    static volatile bool thread_sense[MAX_THREADS] = {false};

    thread_sense[tid] = !thread_sense[tid];
    if (fai(&count) == nthreads-1) {
        count = 0;
        sense = !sense;
    } else {
        while (sense != thread_sense[tid]);     /* spin */
    }
}

void initialize()
{
    int i;

    counter = 0;
    winner = -1;
    handoffs = -1;  /* first one doesn't count */
    for (i = 0; i < NUM_SPECIALS; i++) {
        special_events[i] = 0;
    }
    for (i = 0; i < nthreads; i++) {
        successes[i].count = 0;
    }
    start_time = gethrtime();
}

void report()
{
    int total_successes = 0;
    int i;

    end_time = gethrtime();
    for (i = 0; i < nthreads; i++) {
        total_successes += successes[i].count;
    }
    printf("counter %u (%.1f%%)", counter,
        (double) counter/(nthreads*iterations)*100);
    if (counter != total_successes) {
        printf("; expected %u", total_successes);
        exit(1);
    }
    printf("; %.0f ns per; ",
        ((double) end_time - (double) start_time)
        / (double) iterations / nthreads);
    printf("%u handoffs (%.1f%%)", handoffs,
        (double) handoffs/counter*100);
    for (i = 0; i < NUM_SPECIALS; i++) {
        if (special_events[i]) {
            printf ("; %u %s", special_events[i], special_names[i]);
        }
    }
    printf ("\n");
    fflush(stdout);
}

/* in the following, acquire, timed_acquired, and release must be
    complete C function calls, including parameters */
#define TRY_TEST(name, acquire, timed_acquire, release)         \
    if (!tid) {                                                 \
        printf("%s: ", name);                                 \
        fflush(stdout);                                         \
        initialize();                                           \
    }                                                           \
    barrier(tid);                                               \
    for (i = 1; i <= iterations; i++) {                         \
        if (try_percent == 100                                  \
                || random() % 100 < try_percent) {              \
            asm("! timed_acquire [");                           \
            if (!timed_acquire) {                               \
                asm("! ] timed_acquire");                       \
                continue;                                       \
            }                                                   \
        } else {                                                \
            asm("! acquire [");                                 \
            acquire;                                            \
            asm("! ] acquire");                                 \
        }                                                       \
        for (k = critical_work; k; k--) {                       \
            if (foo) break; /* never happens; just want to make \
                sure the compiler thinks it might, and doesn't  \
                "optimize" it out */                            \
        }                                                       \
        counter++;                                              \
        if (tid != winner) {                                    \
            handoffs++;                                         \
            winner = tid;                                       \
        }                                                       \
        successes[tid].count++;                                 \
        asm("! release [");                                     \
        release;                                                \
        asm("! ] release");                                     \
        for (k = non_critical_work; k; k--) {                   \
            if (foo) break; /* never happens; just want to make \
                sure the compiler thinks it might, and doesn't  \
                "optimize" it out */                            \
        }                                                       \
    }                                                           \
    barrier(tid);                                               \
    if (!tid) {                                                 \
        report();                                               \
    }

#define PLAIN_TEST(name, acquire, release)                      \
    TRY_TEST(name, acquire, (acquire, 1), release)

void *worker(void *arg)
{
    int tid = (int) arg;
    int i, k;
    mcs_plain_qnode     mcs_pqn;
    mcs_try_qnode       mcs_tqn;
    clh_cc_qnode       *clh_cqnp = &clh_cc_qnodes[tid].clh_cc_qn;
    clh_numa_qnode     *clh_nqnp = &clh_numa_qnodes[tid].clh_numa_qn;

    if (nthreads == 1) {
        asm("! bare_loop:");
        PLAIN_TEST("bare_loop", 0, 0);

        asm("! loop_with_gethrtime:");
        PLAIN_TEST("loop_with_gethrtime()", 0,
            (void) gethrtime());
    }

    if (nthreads <= max_processors) {

        asm("! tatas:");
        PLAIN_TEST("tatas",
            tatas_acquire(&tatas_L),
            tatas_release(&tatas_L));

        asm("! mcs_plain:");
        PLAIN_TEST("MCS_plain",
            mcs_plain_acquire(&mcs_PL, &mcs_pqn),
            mcs_plain_release(&mcs_PL, &mcs_pqn));

        asm("! clh_plain:");
        PLAIN_TEST("CLH_plain",
            clh_plain_acquire(&clh_PL, clh_cqnp),
            clh_plain_release(&clh_cqnp));

        asm("! clh_plain_numa:");
        PLAIN_TEST("CLH_plain_NUMA",
            clh_plain_numa_acquire(&clh_PNL, clh_nqnp),
            clh_plain_numa_release(&clh_nqnp));
    }

    asm("! tatas_try:");
    TRY_TEST("tatas_try",
        tatas_acquire(&tatas_L),
        tatas_timed_acquire(&tatas_L, patience),
        tatas_release(&tatas_L));

    asm("! mcs_try:");
    TRY_TEST("MCS_try",
        mcs_try_acquire(&mcs_TL, &mcs_tqn),
        mcs_try_timed_acquire(&mcs_TL, &mcs_tqn, patience),
        mcs_try_release(&mcs_TL, &mcs_tqn));

    asm("! mcs2_try:");
    TRY_TEST("MCS2_try",
        mcs2_try_acquire(&mcs2_TL),
        mcs2_try_timed_acquire(&mcs2_TL, patience),
        mcs2_try_release(&mcs2_TL));

    asm("! clh_try:");
    TRY_TEST("CLH_try",
        clh_try_acquire(&clh_TL, clh_cqnp),
        clh_try_timed_acquire(&clh_TL, clh_cqnp, patience),
        clh_try_release(&clh_cqnp));

    asm("! clh_try_swap:");
    TRY_TEST("CLH_try_swap",
        clh_try_swap_acquire(&clh_TSL, clh_cqnp),
        clh_try_swap_timed_acquire(&clh_TSL, clh_cqnp, patience),
        clh_try_swap_release(&clh_cqnp));

    asm("! clh2_try:");
    TRY_TEST("CLH2_try",
        clh2_try_acquire(&clh2_TL),
        clh2_try_timed_acquire(&clh2_TL, patience),
        clh2_try_release(&clh2_TL));
}

main(int argc, char **argv) {
    extern char *optarg;
    extern int optind, opterr, optopt;
    int c;
    char *options, *value;
    int errflg = 0;
    bool bound = 1;

    static char *backoff_options[] = {
#define BACKOFF_BASE 0
                     "base",
#define BACKOFF_CAP  1
                     "cap",
        0};
    extern int backoff_base;
    extern int backoff_cap;

    while ((c = getopt(argc, argv, "b:Bi:n:c:m:p:r:t:v")) != EOF) {
        switch (c) {
            case 'b':
                options = optarg;
                while (*options != '\0') {
                    switch (getsubopt(&options, backoff_options, &value)) {
                        case BACKOFF_BASE:
                            if (!value) {
                                fprintf(stderr, "unrecognized option: %s\n",
                                    value);
                                errflg++;
                            }
                            else backoff_base = atoi(value);
                            if (backoff_base < 0) {
                                fprintf(stderr, "value too small: %s\n",
                                    value);
                                errflg++;
                            }
                            break;
                        case BACKOFF_CAP:
                            if (!value) {
                                fprintf(stderr, "unrecognized option: %s\n",
                                    value);
                                errflg++;
                            }
                            else backoff_cap = atoi(value);
                            if (backoff_cap < 0) {
                                fprintf(stderr, "value too small: %s\n",
                                    value);
                                errflg++;
                            }
                            break;
                        default:
                            fprintf(stderr, "unrecognized option: %s\n",
                                value);
                            errflg++;
                            break;
                    }
                }
                if (backoff_base > backoff_cap) {
                    fprintf(stderr, "cap (%d) must be larger than base (%d)\n",
                        backoff_cap, backoff_base);
                    errflg++;
                }
                break;
            case 'B':
                bound = 0;
                break;
            case 'i':
                iterations = atoi(optarg);
                break;
            case 'n':
                non_critical_work = atoi(optarg);
                break;
            case 'c':
                critical_work = atoi(optarg);
                break;
            case 'm':
                max_processors = atoi(optarg);
                break;
            case 'p':
                patience = atoi(optarg);
                break;
            case 'r':
                if ((try_percent = atoi(optarg)) > 100 || try_percent < 0) {
                    fprintf(stderr,
                        "try percent (%d) must be between 0 and 100\n",
                        try_percent);
                    errflg++;
                }
                break;
            case 't':
                nthreads = atoi(optarg);
                break;
            default:
                errflg++;
                break;
        }
    }
    if (errflg) {
        fprintf(stderr, "usage: %s \
[-b {base=backoff_base, cap=backoff_cap}] \
[-B]\n\t\
[-i iterations] \
[-c cricital work] \
[-n non-critical work] \
[-p patience] \
[-r %%try] \
[-t nthreads] \
[-m processors] \
\n",
            argv[0]);

        exit(1);
    }

    printf("\nt %d, i %d, p %lld, n %d, c %d, m %d, r %d, b %d:%d\n",
        nthreads, iterations, patience, non_critical_work, critical_work,
        max_processors, try_percent, backoff_base, backoff_cap);

    {
        int i;

        for (i = 0; i < nthreads; i++) {
            thread_t tid;
            thr_create(/* NULL, NULL, */ new_stack(tid), stack_size(),
                worker, (void *) i, (bound ? THR_BOUND : NULL), &tid);
        }
        for (i = 0; i < nthreads; i++) {
            void *dum;
            thr_join(NULL, NULL, &dum);
        }
    }

    exit(0);
}