# A fuzzer for BFFParser
This is a tool aimed to find bugs in bff parsing code by parsing randomly generated files.  
Currently it can find plain old crashes and bugs reported by [Address](https://clang.llvm.org/docs/AddressSanitizer.html) and [Memory](https://clang.llvm.org/docs/MemorySanitizer.html) sanitizers.  
Currently there are build configurations only for Linux, although it can be built on OSX in all configurations and the basic version (without sanitizers) can even be built on Windows.

## Dependencies
*   [libFuzzer](http://llvm.org/docs/LibFuzzer.html) is a part of LLVM (specifically: a part of compiler-rt) and may be installed with LLVM, depending on the distribution. If it isn't included in LLVM distribution, then it needs to be built from source.
*   Clang is required to build this tool.
*   (optional) [libc++](http://libcxx.llvm.org/docs/)  
    To run MSan build all code has to be instrumented with MemorySanitizer, this means that we have to build special versions of libFuzzer and libc++ compiled with MemorySanitizer.

### (optional) Building libFuzzer
This should be needed only if Clang 5 or older is used, as starting from Clang 6 libFuzzer is integrated into Clang via `-fsanitize=fuzzer` flag.
*   Get libFuzzer source code:
    ```bash
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/compiler-rt-12.0.0.src.tar.xz
    tar xJf compiler-rt-12.0.0.src.tar.xz compiler-rt-12.0.0.src/lib/fuzzer
    cd compiler-rt-12.0.0.src/lib/fuzzer
    ```
*   Build libFuzzer: `./build.sh`
*   Copy resulting library to some place known to the linker (e.g. `/usr/local/lib`):
    ```bash
    cp libFuzzer.a /usr/local/lib/
    ```

### (optional) Building libc++ with MemorySanitizer
*   Get libc++ source code corresponding to already installed version (example for version 12.0.0):
    ```bash
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/llvm-project-12.0.0.src.tar.xz
    tar xJf llvm-project-12.0.0.src.tar.xz
    cd cd llvm-project-12.0.0.src/libcxx
    ```
*   Build minimal version of libc++ (static library, no tests, no documentation, etc.)
    ```bash
    mkdir build && cd build
    cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DLLVM_USE_SANITIZER=MemoryWithOrigins -DLIBCXX_USE_COMPILER_RT=ON -DLIBCXX_CXX_ABI=libcxxabi -DLIBCXX_CXX_ABI_INCLUDE_PATHS=/usr/include/c++/v1 -DLIBCXX_ENABLE_SHARED=NO -DLIBCXX_ENABLE_EXPERIMENTAL_LIBRARY=NO -DLIBCXX_INCLUDE_BENCHMARKS=NO -DLIBCXX_INCLUDE_TESTS=NO -DLIBCXX_INCLUDE_DOCS=NO
    make -j$(nproc)
    ```
*   Copy resulting library to `External/MSan/` subdirectory in the FASTBuild repo:
    ```bash
    cp lib64/libc++.a /path/to/fastbuild/External/MSan/
    ```

### (optional) Building libFuzzer with MemorySanitizer
*   Get libFuzzer source code (see above).
*   Build libFuzzer with required flags (using libc++ instead of libstdc++, with MSan instrumentation and ignoring MSan diagnostics from libFuzzer itself):
    ```bash
    echo "fun:_ZN6fuzzer*" > denylist.txt
    echo "fun:_ZNK6fuzzer*" >> denylist.txt
    CXX='clang -stdlib=libc++ -fsanitize=memory -fsanitize-memory-track-origins -fsanitize-blacklist=denylist.txt' ./build.sh
    ```
*   Copy resulting library to `External/MSan/` subdirectory in the FASTBuild repo:
    ```bash
    cp libFuzzer.a /path/to/fastbuild/External/MSan/
    ```

## Using BFFFuzzer
Before using BFFFuzzer it is recommended to read [the official libFuzzer tutorial](https://github.com/google/fuzzer-test-suite/blob/master/tutorial/libFuzzerTutorial.md).  
All examples below assume that current directory is `Code/Tools/FBuild/BFFFuzzer`.

### Running BFFFuzzer
There are some options that can be (and should always be) passed to libFuzzer to speed up the process:
*   `-dict=bff.dict` This provides input mutation engine with the list of all special keywords/tokens used in BFF files.
*   `-max_len=1024` This limit the input to 1024 bytes. It should be possible to achieve good coverage with small inputs and small inputs will generally be parsed quicker.

These options are omitted for brevity in the examples below.

To run BFFFuzzer we need to create a directory to store interesting inputs (corpus): `mkdir corpus`.
Then we can run it on that directory:
```bash
# ASan build
../../../../tmp/x64ClangLinux-ASan/Tools/FBuild/BFFFuzzer/bfffuzzer corpus
# or MSan build
../../../../tmp/x64ClangLinux-MSan/Tools/FBuild/BFFFuzzer/bfffuzzer corpus
```
Generally it is better to do fuzzing with ASan build because it runs faster, and later retest corpus on MSan build:
```bash
../../../../tmp/x64ClangLinux-MSan/Tools/FBuild/BFFFuzzer/bfffuzzer -runs=0 corpus
```

### Creating a seed corpus
To greatly speedup the process (at least initially), corpus can be populated with real valid BFF files:
```bash
mkdir /tmp/seed-corpus
i=0; find ../../../../ -name '*.bff' | while read f; do cp "$f" /tmp/seed-corpus/$i; i=$((i+1)); done
bfffuzzer -merge=1 corpus /tmp/seed-corpus
rm -r /tmp/seed-corpus
```

### Minimizing corpus
To speedup initial corpus load it is useful to periodically remove redundant files from corpus:
```bash
mv corpus corpus-old
mkdir corpus
../../../../tmp/x64ClangLinux-ASan/Tools/FBuild/BFFFuzzer/bfffuzzer -merge=1 corpus corpus-old
../../../../tmp/x64ClangLinux-MSan/Tools/FBuild/BFFFuzzer/bfffuzzer -merge=1 corpus corpus-old
rm -r corpus-old
```

### Minimizing crash reproducers
libFuzzer can try to reduce the size of an input that triggers a crash:
```bash
bfffuzzer -minimize_crash=1 -max_total_time=60 crash-input
```
Results for all steps of this process will be stored in files `minimized-from-*`.

### Removing slow test cases
After some time fuzzing starts to produce various inputs that are very slow to parse:
* deeply nested `ForEach` loops,
* exponentially growing strings or arrays (`.S='hello' .S+.S+.S+.S+.S`).
They don't provide much benefit in terms of code coverage but greatly reduce the speed of fuzzing.
They can be removed from the corpus like that:
```bash
find corpus -type f -exec bfffuzzer {} + 2>&1 | awk '/^Executed [^ ]* in ([0-9])* ms$/ { if ($4 > 20) print $2 }' | xargs rm -v
```

### Generating coverage information for corpus
It is easy to get code coverage for existing corpus from the SanitizerCoverage which is used by libFuzzer to guide mutations:
```bash
bfffuzzer -runs=0 -dump_coverage=1 corpus
sancov -symbolize bfffuzzer bfffuzzer.*.sancov > bfffuzzer.symcov
coverage-report-server.py --symcov bfffuzzer.symcov --srcpath ../../../../
```
Then open http://127.0.0.1:8001 in the browser.
