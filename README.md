# Function Level Profiler - fprof


## Revision History

| Date | By | Description |
|------|----|-------------|
| Dec 21 2023 | EDaly | Version 1.0.0 - First release |

## Configuring fprof

Review the following settings int fprof.h file, and make any changes

| Setting | Default | Description |
|---------|---------|-------------|
| OUTPUT_INTERVAL_MIN | 10 | |
| OUTPUT_CSV_FILE | "fprof" | output csv file name |
| OUTPUT_MD_FILE | "fprof" | output md file name |
| USE_HTML_IN_OUTPUT_FILE |	1 | 1 = use html in md file (colored text), 0 = no html in md file |

After making and changes to the header file, remember to rebuild the library and reinstall.

    make release
    make install

## Building the fprof library

Steps:

- To build the library:

        make release

- To install the library

        make install

## Using the fprof library

Steps:

- Add the fprof.h to every c file that you want analyzed

        #define <fprof.h>

- Add the following to the CFLAGS variable in your makefile.

        -finstrument-functions -export-dynamic -Wl,--export-dynamic

- Add the following to the LDFLAGS variable in your makefile.

        -ldl