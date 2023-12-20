#ifndef _FPROF_H
#define _FPROF_H

#ifdef __cplusplus
extern "C" {
#endif

// fprof.h
// header file for function profiler

// Steps
// 1) add this file, fprof.h to every c file that you want analyzed

// 2) add the following to the CFLAGS variable in your makefile
// -finstrument-functions -export-dynamic -Wl,--export-dynamic

// 3) add the following to the LDFLAGS variable in your makefile
// -ldl

#define _GNU_SOURCE
#include <dlfcn.h>

// user settings
#define OUTPUT_INTERVAL_SEC	(1*60*60)
#define OUTPUT_CSV_FILE	"fprof"
#define OUTPUT_MD_FILE	"fprof"

#define MAX_LEN	512
#define MAX_FUNCTIONS	4096

// function stats
typedef struct st {
	void *this_fn;
	void *call_site;
	char sfile[MAX_LEN];
	int errno_start;
	int errno_end;
	int error_count;
	int serror_num;
	char serror_desc[MAX_LEN];
	unsigned long call_count;
	unsigned long time_start;
	unsigned long time_end;
	unsigned long time_max;
	unsigned long time_min;
} STATS;

// program stats
typedef struct ps {
	int file_count;
	char file_md_name[MAX_LEN];
	char file_csv_name[MAX_LEN];
	int stats_count;
	STATS stats[MAX_FUNCTIONS];
} PSTATS;

void __cyg_profile_func_enter(void *this_fn, void *call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *this_fn, void *call_site) __attribute__((no_instrument_function));
void fprof_update_stats_start(void *this_fn, void *call_site) __attribute__((no_instrument_function));
void fprof_update_stats_end(void *this_fn, void *call_site) __attribute__((no_instrument_function));

// prints status to stdout
void fprof_print_stats(void) __attribute__((no_instrument_function));

// print results in csv file format
void fprof_stats_csv(char *pathname) __attribute__((no_instrument_function));

// print results in markdown file format
void fprof_stats_md(char *pathname) __attribute__((no_instrument_function));

unsigned long fprof_get_time(void) __attribute__((no_instrument_function));
unsigned long fprof_get_time_diff(unsigned long start, unsigned long end) __attribute__((no_instrument_function));

// todo remove?
//int main(int argc, char *argv[]) __attribute__((no_instrument_function));

// exit signal handler
void fprof_sig_handler(int signum) __attribute__((no_instrument_function));

// exit function
void fprof_fini(void) __attribute__((no_instrument_function));

#ifdef __cplusplus
}
#endif

#endif