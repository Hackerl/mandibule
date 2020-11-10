# mandibule: linux elf injector

## intro
Mandibule is a program that allows to inject an ELF file into a remote process.

Both static & dynamically linked programs can be targetted.
Supported archs:

- x86
- x86_64
- arm
- aarch64

Example usage: https://asciinema.org/a/KkOHP2Jef0E6wViPCglkXLRcV

@ixty 2018


## installation
```shell
git clone https://github.com/ixty/mandibule
make
```


## new feature
- support containers
- shellcode for malloc/free memory in remote process
- fix memory leak bug
- python inject sample like "pyrasite/pylane"
- fake stack aligned 16-byte

## usage
```shell
usage: ./mandibule <elf> [-a arg]* [-e env]* [-m addr] <pid>

loads an ELF binary into a remote process.

arguments:
    - elf: path of binary to inject into <pid>
    - pid: pid of process to inject into

options:
    -a arg: argument to send to injected program - can be repeated
    -e env: environment value sent to injected program - can be repeated
    -m mem: base address at which program is loaded in remote process, default=AUTO

Note: order of arguments must be respected (no getopt sry)
```


## example run
```shell
$ make x86_64

# in shell 1
$ ./target
> started.
......

# in shell 2
$ ./mandibule ./toinject `pidof target`
> target pid: 2255143
> arg[0]: ./toinject
> args size: 51
> set namespace pid: 2255143
> success attaching to pid 2255143
> backed up registers
> shellcode injection addr: 0x563786d75000 size: 0x1ea (available: 0x1000)
> backed up memory
> injected shellcode at 0x563786d75000
> running shellcode..
> shellcode executed!
> restored memory
> malloc heap: 0x7f81d388c010
> shellcode injection addr: 0x7f81d388d000 size: 0x6000 (available: 0x6000)
> backed up memory
> injected shellcode at 0x7f81d388d000
> running shellcode..
> cancel exit syscall
> break exit syscall
> shellcode executed!
> restored memory
> free heap: 0x7f81d388c010
> shellcode injection addr: 0x563786d75000 size: 0xc9 (available: 0x1000)
> backed up memory
> injected shellcode at 0x563786d75000
> running shellcode..
> shellcode executed!
> restored memory
> restored registers
> success detach from pid 2255143
> successfully injected shellcode into pid 22551436

# back to shell 1
...
> arg[0]: ./toinject
> args size: 51
> auxv len: 320
> auto-detected manual mapping address 0x563788000000
> mapping './toinject' into memory at 0x563788000000
> reading elf file './toinject'
> loading elf at: 0x563788000000
> load segment addr 0x563788000000 len 0x1000 => 0x563788000000
> load segment addr 0x563788200dd8 len 0x1000 => 0x563788200000
> max vaddr 0x563788212000
> loading interp '/lib64/ld-linux-x86-64.so.2'
> reading elf file '/lib64/ld-linux-x86-64.so.2'
> loading elf at: 0x563788212000
> load segment addr 0x563788212000 len 0x23000 => 0x563788212000
> load segment addr 0x563788435bc0 len 0x2000 => 0x563788435000
> max vaddr 0x563788448000
> eop 0x563788212c20
> setting auxv
> set auxv[3] to 0x563788000040
> set auxv[5] to 0x9
> set auxv[9] to 0x5637880006a0
> set auxv[7] to 0x563788000000
> eop 0x563788212c20
> align stack ptr: 7ffd1ee72888
> stack ptr: 7ffd1ee72880
> starting ...

# oh hai from pid 6266
# arg[0]: ./toinject
# :)
# :)
# :)
# bye!
...........


```


## injection proces
mandibule has no dependency (not even libc) and is compiled with pie and fpie in order to make it fully relocatable.

This way we can copy mandibule's code into any process and it will be able to run as if it were a totally independant shellcode.

Here is how mandibule works:

- find an executable section in target process with enough space (~500b)
- attach to process with ptrace
- backup register state
- backup executable section
- inject malloc shellcode into executable section
- malloc enough space in remote process for inject mandibule
- inject mandibule code into memory allocated in the previous step
- let the execution resume on our own injected code
- wait until exit() is called by the remote process
- inject free shellcode into executable section
- free previously allocated memory
- restore registers & memory
- detach from process

In the remote process, mandibule does the following:

- read arguments, environment variables and other options from its own memory
- find a suitable memory address to load the target elf file if needed
- manually load & map the elf file into memory using only syscalls
- load the ld-linux interpreter if needed
- call the main function of the manually loaded binary


## tested on

- __x86__:      Linux debian 4.9.0-3-amd64 #1 SMP Debian 4.9.30-2+deb9u5 (2017-09-19) x86_64 GNU/Linux
- __x86_64__:   Linux debian 4.9.0-3-amd64 #1 SMP Debian 4.9.30-2+deb9u5 (2017-09-19) x86_64 GNU/Linux
- __arm64__:    Linux buildroot 4.13.6 #1 SMP Sat Mar 3 16:40:18 UTC 2018 aarch64 GNU/Linux
- __arm__:      Linux buildroot 4.11.3 #1 SMP Sun Mar 4 02:36:56 UTC 2018 armv7l GNU/Linux

arm & arm64 where tested using [arm_now](https://github.com/nongiach/arm_now) by [@chaignc](https://twitter.com/chaignc) to easily spawn qemu vms with the desired arch.
