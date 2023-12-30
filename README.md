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
| OUTPUT_CSV_FILE | "fprof" | output csv file name prefix |
| OUTPUT_MD_FILE | "fprof" | output md file name prefix |
| USE_HTML_IN_OUTPUT_FILE | 1 | 1 = use html in md file (colored text), 0 = no html in md file |
| MAX_FUNCTIONS | 10240 | The maximum number of functions that can be processed |

After making and changes to the header file, remember to rebuild the library and reinstall.

    make release
    sudo make install

## Building the fprof library

Steps:

- To build the library:

        make release

- To install the library

        sudo make install

## Using the fprof library

Steps:

- Add the fprof.h to every c file that you want analyzed. No need to modify any of your source code files.

        #define <fprof.h>

- Add the following to the CFLAGS (compiler options) variable in your makefile.

        -finstrument-functions -export-dynamic -Wl,--export-dynamic

- Add the following to the LDFLAGS (linker options) variable in your makefile.

        -L/usr/local/lib -Wl,-R/usr/local/lib -lfprof -ldl
