# Granota

Alternative fuzzing engine for the [american-fuzzy-lop](http://lcamtuf.coredump.cx/afl/) (AFL) instrumentation system.

## Movitation

I started this project in 2013 to experiment some naive ideas in the early development days of the AFL fuzzer.

## Features

Remarkable features are:

* The fuzzing engine is based on genetic algorithms. The objective function depends on the coverage information provided by AFL`s instrumentation.

* Uses [crit-bit trees](http://cr.yp.to/critbit.html) to sort test cases. It allows different types of files to be fuzzed at once.

* Hooks I/O syscalls to fuzz daemons and standalone programs continuously without restarting them.

* Hooks memory comparison functions (e.g. strcmp) to potencially guest **magic headers** and defeact **checksums**.

* Week integration between the fuzzer and the fork server makes possible to target any child in the process tree.

* Child process forks late to bypass all the initialization overhead.

* Reads and writes fuzzed files direcly in shared memory (SHM) without touching the filesystem.

* Performs well even with large files (>100 KB).

* Generates unique crash logs based on the target's stack trace signature.

* Automatically classifies crash logs at runtime by severity using the [exploitable](https://github.com/jfoote/exploitable) library.

## Status

Some vulnerabilities were found in popular free software using this fuzzer. Bad news are that the genetic fuzzing engine offers limited coverage.

## Installation

Granota is built with autotools. Try typing:

```sh
./configure && make && sudo make install
```

## Usage

Compile the target program with **granota-gcc** just as you would do with the original AFL.

Run granota binary. It requires a file to fuzz and the program's executable to test. 

```
granota 0.27.afl built Mar 10 2016 20:28:12

Usage: granota [OPTION]... EXE-PATH

Target Selection:
  -f FILE       path to target FILE

Directories:
  -i DIR        input DIR for test cases
  -o DIR        output DIR for test cases
  -a DIR        crash DIR for captured crashes
  -e DIR        temporary DIR (/dev/shm)

Execution Control:
  -t MSEC       timeout for each input (1000 MSEC)
  -m MB         memory limit for children process (512 MB)
  -q            null-ify children's stdin, stdout, stderr

Fuzzing Options:
  -l BYTES      maximum size for each input file (1048576 BYTES)
  -n FILES      backlog of the input queue (4 FILES)
  -s SECONDS    stage duration (2 SECONDS)
```

Finally launch the program's executable. Do not forget to inject the granota's helper library into the binary with **LD_PRELOAD**.

```sh
LD_PRELOAD=/usr/local/lib/libgranota.so /path/to/tested_binary ...
```

The input directory with initial test cases is optional. If it not specified, the fuzzer starts with random inputs. 

## Tutorial

The directory `tests/clamav` contains a framework to fuzz the [ClamAV](https://www.clamav.net) antivirus and the [zlib](http://www.zlib.net) and [bzip2](http://www.bzip.org) libraries. It illustates the use of the fuzzer on a relatively complex piece of software. 

To run the test you will need an empty directory on your home directory called `$HOME/fuzz`. All files will be installed under that directory.

The build process is automated by the provided **Makefile**, so no further intervention should be necessary. Just go to the directory `tests/clamav` and run **make**. 

The command line scanner `clamscan` is our target.

```sh
# Temporarily disable ASLR
sudo sysctl -w kernel.randomize_va_space=0

# Allow ptrace processes
sudo sysctl -w kernel.yama.ptrace_scope=0

# Disable piping core dumps
sudo sysctl -w kernel.core_pattern=core

# Every 60 seconds clean up temporary files left by clamav (requires tmpwatch tool)
( while tmpwatch -cqa -X /tmp/foobar.exe 1m /tmp; do sleep 60; done ) &

# Start the fuzzer
granota -i ~/fuzz/root/seeds -o ~/fuzz/root/testcases -a ~/fuzz/root/crashes -f ~/fuzz/root/foobar.exe ~/fuzz/root/bin/clamscan
```

Switch to another terminal and run `clamscan`:

```
LD_PRELOAD=/usr/local/lib/libgranota.so \
 ~/fuzz/root/bin/clamscan --quiet -d ~/fuzz/root/test.ndb ~/fuzz/root/foobar.exe 
```

The fuzzing process will continue until you press Ctrl-C. There are three subdirectories created within the fuzz directory and updated in real time: seeds, testcases and crashes.

## Status Screen

```
   _oo__      granota 0.27.afl up 0+08:34:18
  _\-   \__   Usage: 0m17.72s user, 0m28.43s sys, 990492 vctxsw
 _\-)--)-,/_  Mem: 83864k used, 19309 pagefts

Session: R (running), 106060 tpid, 36 evals/s
Crash: 0 exploitable, 0 probably, 1 probably-not
       173 seen, 1 unknown, 0 error
Stage: #   cycles  evaluations    nodes   refines   collns
    => 3     9063       748441      796   3888116    22306
       2     1424       227602      365    323765     3081
       1        0            0        0         0        0
       0        1           52       32         7        0
Pivot: 254/1193 seq, 11 children, 0.0000465389 score (in 171.89 secs)
       422 bytes, 1739/3376 critbit
  0000: 504b 0304 1500 0200 0900 5914 fd38 fd3c  PK........Y..8.<
  0010: 07ef 1001 0000 2002 0000 0800 0000 636c  ...... .......cl
  0020: 616d 2e65 7865 bd51 314b 0331 187d 8987  am.exe.Q1K.1.}..
  0030: 60ad 9a4d 7450 6f74 516b 7f80 7757 3399  `..MtPotQk..wW3.
```

## Crashes

Hopefully after several minutes the fuzzer will find some crashes.

Here is an example of a crash log found in the current version of ClamAV (0.98.4) at the time of writing this.

**SourceAvNearNull.7zIn.c.1110.7e3eda3.1736ccb.log**
```
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/usr/lib/libthread_db.so.1".
0x00007ffff4d10c39 in raise () from /usr/lib/libpthread.so.0

Program received signal SIGSTOP, Stopped (signal).
0x00007ffff4d10c39 in raise () from /usr/lib/libpthread.so.0

Program received signal SIGSEGV, Segmentation fault.
SzReadHeader2 (allocTemp=0x7ffff7bbfe30 <allocTempImp>, allocMain=0x7ffff7bbfe40 <allocImp>, lwtVector=0x7fffffff76e0, emptyFileVector=0x7fffffff76d8, emptyStreamVector=0x7fffffff76d0, digests=0x7fffffff76c8, digestsDefined=0x7fffffff76c0, unpackSizes=0x7fffffff76b8, sd=0x7fffffff7700, p=0x7fffffff7a80) at 7z/7zIn.c:1110
1110          file->Size = (*unpackSizes)[sizeIndex];
__main__:104: UserWarning: GDB v7.11 may not support required Python API
'exploitable' version 1.31 (granota)
Linux arch 4.4.1-2-ARCH #1 SMP PREEMPT Wed Feb 3 13:12:33 UTC 2016 x86_64
Signal si_signo: 11 Signal si_addr: 0x0
Nearby code:
Registers:
rax            0x0  0
rbx            0x7fffffff7a80 140737488321152
rcx            0x1  1
rdx            0x0  0
rsi            0x0  0
rdi            0x0  0
rbp            0x7ffff7bbfe30 0x7ffff7bbfe30 <allocTempImp>
rsp            0x7fffffff7650 0x7fffffff7650
r8             0xffff7a7f 4294933119
r9             0x0  0
r10            0x0  0
r11            0x0  0
r12            0x7ffff7bbfe40 140737349680704
r13            0x7fffee3d8029 140737190395945
r14            0x1df631 1963569
r15            0x0  0
rip            0x7ffff532f727 0x7ffff532f727 <SzArEx_Open2+12703>
eflags         0x10202  [ IF RF ]
cs             0x33 51
ss             0x2b 43
ds             0x0  0
es             0x0  0
fs             0x0  0
gs             0x0  0
Stack trace:
#0  SzReadHeader2 (allocTemp=0x7ffff7bbfe30 <allocTempImp>, allocMain=0x7ffff7bbfe40 <allocImp>, lwtVector=0x7fffffff76e0, emptyFileVector=0x7fffffff76d8, emptyStreamVector=0x7fffffff76d0, digests=0x7fffffff76c8, digestsDefined=0x7fffffff76c0, unpackSizes=0x7fffffff76b8, sd=0x7fffffff7700, p=0x7fffffff7a80) at 7z/7zIn.c:1110
#1  SzReadHeader (allocTemp=0x7ffff7bbfe30 <allocTempImp>, allocMain=0x7ffff7bbfe40 <allocImp>, sd=0x7fffffff7700, p=0x7fffffff7a80) at 7z/7zIn.c:1143
#2  SzArEx_Open2 (p=p@entry=0x7fffffff7a80, inStream=<optimized out>, allocMain=allocMain@entry=0x7ffff7bbfe40 <allocImp>, allocTemp=0x7ffff7bbfe30 <allocTempImp>) at 7z/7zIn.c:1338
#3  0x00007ffff5332c19 in SzArEx_Open (p=0x7fffffff7a80, inStream=<optimized out>, allocMain=0x7ffff7bbfe40 <allocImp>, allocTemp=<optimized out>) at 7z/7zIn.c:1350
#4  0x00007ffff5321289 in cli_7unz (ctx=0x7fffffffc120, offset=<optimized out>) at 7z_iface.c:110
#5  0x00007ffff5073baa in magic_scandesc (ctx=ctx@entry=0x7fffffffc120, type=CL_TYPE_7Z, type@entry=CL_TYPE_ANY) at scanners.c:2725
#6  0x00007ffff506355b in cli_base_scandesc (type=CL_TYPE_ANY, ctx=0x7fffffffc120, desc=7) at scanners.c:3007
#7  cli_magic_scandesc (desc=desc@entry=7, ctx=ctx@entry=0x7fffffffc120) at scanners.c:3016
#8  0x00007ffff5079fdf in scan_common (context=<optimized out>, scanoptions=<optimized out>, engine=<optimized out>, scanned=<optimized out>, virname=0x7fffffffc418, map=0x0, desc=7) at scanners.c:3233
#9  cl_scandesc_callback (desc=7, virname=0x7fffffffc418, scanned=<optimized out>, engine=<optimized out>, scanoptions=<optimized out>, context=<optimized out>) at scanners.c:3252
#10 0x000000000041c1d4 in scanfile (filename=filename@entry=0x67ef40 "foobar.exe", engine=engine@entry=0x677d40, opts=opts@entry=0x673010, options=options@entry=4219447) at manager.c:303
#11 0x000000000042473d in scanmanager (opts=0x673010) at manager.c:1005
#12 0x0000000000402f72 in main (argc=<optimized out>, argv=0x7fffffffea38) at clamscan.c:164
#13 0x00007ffff497c610 in __libc_start_main () from /usr/lib/libc.so.6
#14 0x0000000000403c49 in _start ()
Title: SourceAvNearNull.7zIn.c.1110.7e3eda3.1736ccb
Description: Access violation near NULL on source operand
Short description: SourceAvNearNull (16/22)
Hash: 7e3eda35e167c5e06f14d897b82dc32f.1736ccb784c993920a7c90afbf417621
Exploitability Classification: PROBABLY_NOT_EXPLOITABLE
Explanation: The target crashed on an access violation at an address matching the source operand of the current instruction. This likely indicates a read access violation, which may mean the application crashed on a simple NULL dereference to data structure that has no immediate effect on control of the processor.
Other tags: AccessViolation (21/22)
```

Please note that when you cannot reproduce a crash found by granota, the most likely cause is that you are not setting the same memory limit as used by the tool. Try:

```sh
LIMIT_MB=512
( ulimit -Sv $[LIMIT_MB << 10]; /path/to/tested_binary ... )
```

## License

Licensed under the Apache License Version 2.0. See COPYING file.
