# AntiDump

This Kernel module will implement some methods to prevent memory dumping in a stable, silent manner.

## Why?

Some well known Linux rootkits are written as kernel modules.
Threat actors often use those rootkits to hide artifacts and indicators of compromise such as processes, files and network connections.
While those techniques are effective against live forensic analysis or security software, they are mostly useless against [modern offline memory analysis](https://github.com/volatilityfoundation/volatility).

## Preventing Memory Dumps
It's very often that modern Linux distributions comes with a kernel that is configured in a way that doesn't allows usage of built-in interfaces to dump memory ([Example](https://www.kernelconfig.io/config_strict_devmem)), and even if those interfaces aren't blocked, It's supposed to be trivial for a rootkit to prevent access to a file/device from user mode using any form of hooking (DKOM, `kprobe`, inline hooking, etc...)
That leaves us with one reliable way to take a memory dump from a Linux machine - A kernel module.

both [LiME](https://github.com/504ensicsLabs/LiME) and [PMEM](https://github.com/google/rekall/tree/master/tools/linux) (Two widely used kernel modules that accomplish this) rely on their ability to enumerate the system memory to find RAM ranges in the IO memory by iterating the kernel resource tree starting at `iomem_resource`.

I came up with two methods to prevent such attempts to enumerate system RAM:

#### Blocking the module
 A notifier callback can be registered to notify an attacker when a new kernel module is loaded, and the callback has the ability to block it before it's `init` function is executed (and a memory dump is taken).
 But, instead of blocking every module (including innocent ones), I'll block just the ones that use the `iomem_resource` symbol.
 That will probably raise less suspicion among the system maintainers.

#### Manipulating the resource tree
Each entry in the resource tree has a name, and memory analysts care about `System RAM` entries.
The field that holds the resource name can be maliciously modified to disable automatic detection of memory ranges (because the tools mentioned above look for entries named `System RAM` specifically).
The resource tree can be seen by usermode applications via `/proc/iomem`, so it can be arranged that the modification won't be visible from there by hooking the 'show' operation of the `/proc/iomem` interface.

#### Maybe I'll add more methods in the future. Make a pull request if you have an idea!
## Usage
I DON'T TAKE ANY RESPONSIBILITY OF WHAT HAPPENS WHEN YOU RUN THIS ON A MACHINE.

This was tested on my Ubuntu 21.10 with 5.13.0-41-generic kernel.

To use this modify `anti_dump.h` and select the method that you want to use (don't enable both).
Once you are done, `cd` into the project repository and:
```
make
insmod anti_dump.ko
```