// fprof.c
// function profiler source code

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>
#include "fprof.h"


Dl_info info;
PSTATS pstats = { .stats_count = 0 };

// return time of day
unsigned long fprof_get_time(void) {

	struct timeval time;

	gettimeofday(&time, NULL);
	return ((time.tv_sec * 1000000) + time.tv_usec);
}

// get time difference
unsigned long fprof_get_time_diff(unsigned long start, unsigned long end) {
	return (end - start);
}

// print stats
void fprof_print_stats(void) {

	int i;

	for (i = 0; i < pstats.stats_count; i++) {
		dladdr(pstats.stats[i].this_fn, &info);
		strcpy(pstats.stats[i].sfile, info.dli_fname);

		printf("file(%s), this_fn(%p, %s), count(%lu), min time(%lu), max time(%lu)\n",
		pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
		pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max);
	}
}

// print results in csv file format
void fprof_stats_csv(char *pathname) {

	int i;
	FILE *fp;

	fp = fopen(pathname, "w");

	for (i = 0; i < pstats.stats_count; i++) {
		dladdr(pstats.stats[i].this_fn, &info);
		strcpy(pstats.stats[i].sfile, info.dli_fname);

		if (i == 0) {
			fprintf(fp, "file, function_ptr, function, count, min_time, max_time\n");
		}
		else {
			fprintf(fp, "%s, %p, %s, %lu, %lu, %lu\n",
			pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
			pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max);
		}
	}

	fclose(fp);
}

// print results in markdown file format
void fprof_stats_md(char *pathname) {

	int i;
	FILE *fp;

	fp = fopen(pathname, "w");

	for (i = 0; i < pstats.stats_count; i++) {
		dladdr(pstats.stats[i].this_fn, &info);
		strcpy(pstats.stats[i].sfile, info.dli_fname);

		if (i == 0) {
			//fprintf(fp, "| file | function_ptr | function | count | min_time | max_time |\n");
			fprintf(fp, "# %s\n\n", pathname);
			fprintf(fp, "| function | count | min_time (usec) | max_time (usec) |\n");
			fprintf(fp, "|----------|-------|-----------------|-----------------|\n");
		}
		else {
			//fprintf(fp, "| %s | %p | %s | %lu | %lu | %lu |\n",
			//pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
			//pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max);

			fprintf(fp, "| %s | %lu | %lu | %lu |\n",
			info.dli_sname, pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max);
		}
	}

	fclose(fp);
}

// exit function
void fprof_fini(void){
	fprof_print_stats();
	fprof_stats_csv("fprof.csv");
	fprof_stats_md("fprof.md");
}

// exit signal handler
void fprof_sig_handler(int signum) {
	//print_stats();
	exit(EXIT_SUCCESS);
}

void fprof_update_stats_start(void *this_fn, void *call_site){

	int i;

	if (pstats.stats_count == 0) {
		signal(SIGINT, fprof_sig_handler);
		atexit(fprof_fini);
	}

	for (i = 0; i < MAX_FUNCTIONS; i++) {
		if (this_fn == pstats.stats[i].this_fn) {
			// match, already in table, just update the stuff that changed
			pstats.stats[i].call_count++;
			pstats.stats[i].time_start = fprof_get_time();
			return;
		}
	}

	// not in table
	i = pstats.stats_count;
	pstats.stats[i].call_count = 1;
	pstats.stats[i].this_fn = this_fn;
	pstats.stats[i].call_site = call_site;
	pstats.stats[i].time_start = fprof_get_time();
	pstats.stats[i].time_min = pstats.stats[i].time_start;
	pstats.stats[i].time_max = 0;
	if (pstats.stats_count < MAX_FUNCTIONS) {
		pstats.stats_count++;
	}
	return;
}

void fprof_update_stats_end(void *this_fn, void *call_site){

	int i;
	unsigned long elapsed;

	for (i = 0; i < MAX_FUNCTIONS; i++) {
		if (this_fn == pstats.stats[i].this_fn) {

			// match
			pstats.stats[i].time_end = fprof_get_time();
			elapsed = fprof_get_time_diff(pstats.stats[i].time_start, pstats.stats[i].time_end);

			if (elapsed < pstats.stats[i].time_min) {
				pstats.stats[i].time_min = elapsed;
			}

			if (elapsed > pstats.stats[i].time_max) {
				pstats.stats[i].time_max = elapsed;
			}
			return;
		}
	}

	// not in table
	fprintf(stderr, "table insertion error\n");
	return;
}

void __cyg_profile_func_enter(void *this_fn, void *call_site) {
	fprof_update_stats_start(this_fn, call_site);
}

void __cyg_profile_func_exit(void *this_fn, void *call_site) {
	fprof_update_stats_end(this_fn, call_site);
}



