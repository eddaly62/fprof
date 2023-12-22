// fprof.c
// function profiler

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>
#include <errno.h>
#include "fprof.h"

// declare and initialize variables
Dl_info info;
PSTATS pstats = { .stats_count = 0, .file_count = 0 };

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

// print results in csv file format
void fprof_stats_csv(char *pathname) {

	int i;
	FILE *fp;

	fp = fopen(pathname, "w");

	for (i = 0; i < pstats.stats_count; i++) {
		dladdr(pstats.stats[i].this_fn, &info);
		strcpy(pstats.stats[i].sfile, info.dli_fname);

		if (i == 0) {
			fprintf(fp, "file, function_ptr, function, count, min_time_usec, max_time_usec, err_count, errno, err_desc\n");
		}
		else {
			fprintf(fp, "%s, %p, %s, %lu, %lu, %lu, %d, %d, %s\n",
			pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
			pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max,
			pstats.stats[i].error_count, pstats.stats[i].serror_num, pstats.stats[i].serror_desc);
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
			fprintf(fp, "# %s\n\n", pathname);
			fprintf(fp, "| file | function_ptr | function | count | min_time_usec | max_time_usec | err_count | errno | err_desc |\n");
			fprintf(fp, "|------|--------------|----------|-------|---------------|---------------|-----------|-------|----------|\n");
		}
		else {
			if (pstats.stats[i].error_count > 0) {
#if USE_HTML_IN_OUTPUT_FILE
				fprintf(fp, "| %s | %p | %s | %lu | %lu | %lu | %s%d%s | %s%d%s | %s%s%s |\n",
				pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
				pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max,
				PRE_HTML_RED, pstats.stats[i].error_count, POST_HTML_RED,
				PRE_HTML_RED, pstats.stats[i].serror_num, POST_HTML_RED,
				PRE_HTML_RED, pstats.stats[i].serror_desc, POST_HTML_RED);
#else
				fprintf(fp, "| %s | %p | %s | %lu | %lu | %lu | %d | %d | %s |\n",
				pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
				pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max,
				pstats.stats[i].error_count, pstats.stats[i].serror_num, pstats.stats[i].serror_desc);
#endif
			}
			else {
				fprintf(fp, "| %s | %p | %s | %lu | %lu | %lu | %d | %d | %s |\n",
				pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
				pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max,
				pstats.stats[i].error_count, pstats.stats[i].serror_num, pstats.stats[i].serror_desc);
			}
		}
	}

	fclose(fp);
}

// output status
void fprof_send_status(void){

	snprintf(pstats.file_csv_name, MAX_LEN, "%s-%d.csv", OUTPUT_CSV_FILE, pstats.file_count);
	snprintf(pstats.file_md_name, MAX_LEN, "%s-%d.md", OUTPUT_MD_FILE, pstats.file_count);
	fprof_stats_csv(pstats.file_csv_name);
	fprof_stats_md(pstats.file_md_name);
}

// signal handler
void fprof_sig_handler(int signum) {

	if (signum == SIGALRM) {
		fprof_send_status();
		pstats.file_count++;
		alarm(OUTPUT_INTERVAL_SEC);
	}
	else {
		exit(EXIT_SUCCESS);
	}
}

// pre-function processing
void fprof_update_stats_start(void *this_fn, void *call_site){

	int i;

	if (pstats.stats_count == 0) {
		// start up
		signal(SIGINT, fprof_sig_handler);
		signal(SIGALRM, fprof_sig_handler);
		alarm(OUTPUT_INTERVAL_SEC);
		atexit(fprof_send_status);
	}

	for (i = 0; i < MAX_FUNCTIONS; i++) {
		if (this_fn == pstats.stats[i].this_fn) {
			// match, already in table, just update the stuff that changed
			pstats.stats[i].errno_start = errno;
			pstats.stats[i].call_count++;
			pstats.stats[i].time_start = fprof_get_time();
			return;
		}
	}

	// not in table
	i = pstats.stats_count;
	pstats.stats[i].errno_start = errno;
	pstats.stats[i].call_count = 1;
	pstats.stats[i].this_fn = this_fn;
	pstats.stats[i].call_site = call_site;
	pstats.stats[i].time_start = fprof_get_time();
	pstats.stats[i].time_min = pstats.stats[i].time_start;
	pstats.stats[i].time_max = 0;
	pstats.stats[i].error_count = 0;
	pstats.stats[i].serror_num = 0;
	pstats.stats[i].serror_desc[0] = '\0';
	if (pstats.stats_count < MAX_FUNCTIONS) {
		pstats.stats_count++;
	}
	return;
}

// post-function processing
void fprof_update_stats_end(void *this_fn, void *call_site){

	int i;
	unsigned long elapsed;

	for (i = 0; i < MAX_FUNCTIONS; i++) {
		if (this_fn == pstats.stats[i].this_fn) {

			// match found in table
			pstats.stats[i].time_end = fprof_get_time();
			elapsed = fprof_get_time_diff(pstats.stats[i].time_start, pstats.stats[i].time_end);

			if (elapsed < pstats.stats[i].time_min) {
				pstats.stats[i].time_min = elapsed;
			}

			if (elapsed > pstats.stats[i].time_max) {
				pstats.stats[i].time_max = elapsed;
			}

			pstats.stats[i].errno_end = errno;
			if (pstats.stats[i].errno_end != pstats.stats[i].errno_start) {
				// error in the function, update error info.
				// catches last error that occurred, if multiple errors occurred in function
				pstats.stats[i].error_count++;
				pstats.stats[i].serror_num = pstats.stats[i].errno_end;
				snprintf(pstats.stats[i].serror_desc, MAX_LEN, "%s", strerror(pstats.stats[i].errno_end));
			}

			return;
		}
	}

	// not in table
	fprintf(stderr, "table insertion error\n");
	return;
}

// compiler instrumentation function that is inserted before every function call
void __cyg_profile_func_enter(void *this_fn, void *call_site) {
	fprof_update_stats_start(this_fn, call_site);
}

// compiler instrumentation function that is inserted after every function call
void __cyg_profile_func_exit(void *this_fn, void *call_site) {
	fprof_update_stats_end(this_fn, call_site);
}



