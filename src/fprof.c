// fprof.c
// function profiler

#define _POSIX_C_SOURCE 199309
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include "fprof.h"

// interval timer variables
typedef struct it {
    pthread_attr_t attr;
    struct sigevent sig;
    timer_t timerid;
    struct itimerspec in, out;
} INT_TIMER;

// declare and initialize variables
PSTATS pstats = { .stats_count = 0, .file_count = 0 };

INT_TIMER itmr = {
    .in.it_value.tv_sec = OUTPUT_1ST_INTERVAL_SEC,
    .in.it_value.tv_nsec = 0,
    .in.it_interval.tv_sec = OUTPUT_INTERVAL_SEC,
    .in.it_interval.tv_nsec = 0,
    .sig.sigev_notify = SIGEV_THREAD,
    .sig.sigev_value.sival_int = 1,
    .sig.sigev_notify_attributes = &itmr.attr,
};

pthread_mutex_t fprofmutex = PTHREAD_MUTEX_INITIALIZER;
Dl_info info;

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

	pstats.fp_csv = fopen(pathname, "w");
	if (pstats.fp_csv == NULL) {
		perror("Could not open csv file");
		return;
	}

	for (i = 0; i < pstats.stats_count; i++) {
		dladdr(pstats.stats[i].this_fn, &info);
		strcpy(pstats.stats[i].sfile, info.dli_fname);

		if (i == 0) {
			fprintf(pstats.fp_csv, "file, function_ptr, function, count, min_time_usec, max_time_usec, err_count, errno, err_desc\n");

		}
		else {
			fprintf(pstats.fp_csv, "%s, %p, %s, %lu, %lu, %lu, %d, %d, %s\n",
			pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
			pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max,
			pstats.stats[i].error_count, pstats.stats[i].serror_num, pstats.stats[i].serror_desc);
		}
	}

	fflush(pstats.fp_csv);
	fclose(pstats.fp_csv);
}

// print results in markdown file format
void fprof_stats_md(char *pathname) {

	int i;

	pstats.fp_md = fopen(pathname, "w");
	if (pstats.fp_md == NULL) {
		perror("Could not open md file");
		return;
	}

	for (i = 0; i < pstats.stats_count; i++) {
		dladdr(pstats.stats[i].this_fn, &info);
		strcpy(pstats.stats[i].sfile, info.dli_fname);

		if (i == 0) {
			fprintf(pstats.fp_md, "# %s\n\n", pathname);
			fprintf(pstats.fp_md, "Number of function = %d\n\n", pstats.stats_count > 0 ? pstats.stats_count-1 : 0);

			fprintf(pstats.fp_md, "| file | function_ptr | function | count | min_time_usec | max_time_usec | err_count | errno | err_desc |\n");
			fprintf(pstats.fp_md, "|------|--------------|----------|-------|---------------|---------------|-----------|-------|----------|\n");
		}
		else {
			if (pstats.stats[i].error_count > 0) {
#if USE_HTML_IN_OUTPUT_FILE
				fprintf(pstats.fp_md, "| %s | %p | %s | %lu | %lu | %lu | %s%d%s | %s%d%s | %s%s%s |\n",
				pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
				pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max,
				PRE_HTML_RED, pstats.stats[i].error_count, POST_HTML_RED,
				PRE_HTML_RED, pstats.stats[i].serror_num, POST_HTML_RED,
				PRE_HTML_RED, pstats.stats[i].serror_desc, POST_HTML_RED);
#else
				fprintf(pstats.fp_md, "| %s | %p | %s | %lu | %lu | %lu | %d | %d | %s |\n",
				pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
				pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max,
				pstats.stats[i].error_count, pstats.stats[i].serror_num, pstats.stats[i].serror_desc);
#endif
			}
			else {
				fprintf(pstats.fp_md, "| %s | %p | %s | %lu | %lu | %lu | %d | %d | %s |\n",
				pstats.stats[i].sfile, pstats.stats[i].this_fn, info.dli_sname,
				pstats.stats[i].call_count, pstats.stats[i].time_min, pstats.stats[i].time_max,
				pstats.stats[i].error_count, pstats.stats[i].serror_num, pstats.stats[i].serror_desc);
			}
		}
	}

	fflush(pstats.fp_md);
	fclose(pstats.fp_md);
}

// output status helper function
void fprof_output_stats(void) {

		// generate file names
		snprintf(pstats.file_csv_name, FPROF_MAX_LEN, "%s-%d-%d.csv", OUTPUT_CSV_FILE, (int)getpid(), pstats.file_count);
		snprintf(pstats.file_md_name, FPROF_MAX_LEN, "%s-%d-%d.md", OUTPUT_MD_FILE, (int)getpid(), pstats.file_count);

		// write out status
		pthread_mutex_lock(&fprofmutex);
		fprof_stats_csv(pstats.file_csv_name);
		fprof_stats_md(pstats.file_md_name);
		pthread_mutex_unlock(&fprofmutex);

}

// output status
void fprof_send_status(union sigval val) {

	// service interval timer 1
	if (val.sival_int == 1) {

		fprof_output_stats();

		#ifdef DEBUG
		printf("fprof saving data to %s and %s with %d functions\n", pstats.file_csv_name, pstats.file_md_name, pstats.stats_count);
		#endif

		pstats.file_count++;
	}

}

// create and start a timer
int fprof_interval_timer(INT_TIMER *itmr, void *func) {

    int r;

    pthread_attr_init(&itmr->attr);
    itmr->sig.sigev_notify_function = func;

    r = timer_create(CLOCK_REALTIME, &itmr->sig, &itmr->timerid);
    if (r == 0) {

        // issue the periodic timer request here.
        r = timer_settime(itmr->timerid, 0, &itmr->in, &itmr->out);
        if(r != 0) {
            r = timer_delete(itmr->timerid);
        }
    }
    return r;
}


// pre-function processing
void fprof_update_stats_start(void *this_fn, void *call_site){

	int i;

	if (pstats.stats_count == 0) {

		// start up
		atexit(fprof_output_stats);

		// set up and start an interval timer to periodically output status
		fprof_interval_timer(&itmr, fprof_send_status);

		#ifdef DEBUG
		printf("fprof starting up\n");
		#endif
	}

	for (i = 0; i < MAX_FUNCTIONS; i++) {
		if (this_fn == pstats.stats[i].this_fn) {
			// match, already in table, just update the stuff that changed
			pthread_mutex_lock(&fprofmutex);
			pstats.stats[i].errno_start = errno;
			pstats.stats[i].call_count++;
			pstats.stats[i].time_start = fprof_get_time();
			pthread_mutex_unlock(&fprofmutex);
			return;
		}
	}

	// not in table
	pthread_mutex_lock(&fprofmutex);
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
	pthread_mutex_unlock(&fprofmutex);
	return;
}

// post-function processing
void fprof_update_stats_end(void *this_fn, void *call_site){

	int i;
	unsigned long elapsed;

	for (i = 0; i < MAX_FUNCTIONS; i++) {
		if (this_fn == pstats.stats[i].this_fn) {

			pthread_mutex_lock(&fprofmutex);

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
				snprintf(pstats.stats[i].serror_desc, FPROF_MAX_LEN, "%s", strerror(pstats.stats[i].errno_end));
			}

			pthread_mutex_unlock(&fprofmutex);

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



