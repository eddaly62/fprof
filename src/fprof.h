#ifndef _FPROF_H
#define _FPROF_H

#ifdef __cplusplus
extern "C" {
#endif

// fprof.h
// header file for the function profiler

// Steps
// 1) add this file, fprof.h to every c file that you want analyzed

// 2) add the following to the CFLAGS variable in your makefile
// -finstrument-functions -export-dynamic -Wl,--export-dynamic

// 3) add the following to the LDFLAGS variable in your makefile
// -ldl

//#define _POSIX_C_SOURCE 199309
#define _GNU_SOURCE
#include <dlfcn.h>
#include <limits.h>
//#include <time.h>

// user settings
#define OUTPUT_1ST_INTERVAL_MIN	1
#define OUTPUT_INTERVAL_MIN	1
#define OUTPUT_CSV_FILE	"fprof"				// output csv file name
#define OUTPUT_MD_FILE	"fprof"				// output md file name
#define USE_HTML_IN_OUTPUT_FILE	1
#define MAX_FUNCTIONS	10240

// user fuction to exclude from analysis
// more can be added here...

// setting that should not be changed
#if MAX_FUNCTIONS > INT_MAX
#error "MAX_FUNCTION is too large"
#endif

#define FPROF_MAX_LEN	512
#define OUTPUT_1ST_INTERVAL_SEC	(OUTPUT_1ST_INTERVAL_MIN*60)	// report 1st interval in seconds
#define OUTPUT_INTERVAL_SEC	(OUTPUT_INTERVAL_MIN*60)			// report interval in seconds

#define PRE_HTML_RED "<span style=\"color:red;\">"
#define POST_HTML_RED "</span>"

// function stats
typedef struct st {
	void *this_fn;
	void *call_site;
	char sfile[FPROF_MAX_LEN];
	int errno_start;
	int errno_end;
	int error_count;
	int serror_num;
	char serror_desc[FPROF_MAX_LEN];
	unsigned long call_count;
	unsigned long time_start;
	unsigned long time_end;
	unsigned long time_max;
	unsigned long time_min;
} STATS;

// program stats
typedef struct ps {
	int file_count;
	char file_md_name[FPROF_MAX_LEN];
	char file_csv_name[FPROF_MAX_LEN];
	FILE *fp_md;
	FILE *fp_csv;
	int stats_count;
	STATS stats[MAX_FUNCTIONS];
} PSTATS;

#if 0
// interval timer variables
typedef struct it {
    pthread_attr_t attr;
    struct sigevent sig;
    timer_t timerid;
    struct itimerspec in, out;
} INT_TIMER;
#endif

// pre-function hook
void fprof_update_stats_start(void *this_fn, void *call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void *this_fn, void *call_site) __attribute__((no_instrument_function));

// post function hook
void fprof_update_stats_end(void *this_fn, void *call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *this_fn, void *call_site) __attribute__((no_instrument_function));

// print results in csv file format
void fprof_stats_csv(char *pathname) __attribute__((no_instrument_function));

// print results in markdown file format
void fprof_stats_md(char *pathname) __attribute__((no_instrument_function));

// execution time functions
unsigned long fprof_get_time(void) __attribute__((no_instrument_function));
unsigned long fprof_get_time_diff(unsigned long start, unsigned long end) __attribute__((no_instrument_function));

// exit signal handler
void fprof_sig_handler(int signum) __attribute__((no_instrument_function));

// output/exit function
void fprof_send_status(union sigval val) __attribute__((no_instrument_function));


#ifdef __cplusplus
}
#endif

#endif