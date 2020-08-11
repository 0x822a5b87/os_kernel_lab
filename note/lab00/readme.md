# lab00

## 实验目的

- 了解操作系统开发实验环境
- 熟悉命令行方式的编译、调试工程
- 掌握基于硬件模拟器的调试技术
- 熟悉C语言编程和指针的概念
- 了解X86汇编语言

## 了解OS实验

![ucore 系统结构图](../pic/ucore 系统结构图.png)

那我们准备如何一步一步来实现ucore呢？根据一个操作系统的设计实现过程，我们可以有如下的实验步骤：

1. 启动操作系统的 `bootloader`，用于了解操作系统启动前的状态和要做的准备工作，了解运行操作系统的硬件支持，操作系统如何加载到内存中，理解两类中断--“外设中断”，“陷阱中断”等；
2. 物理内存管理子系统，用于理解x86分段/分页模式，了解操作系统如何管理物理内存；
3. 虚拟内存管理子系统，通过页表机制和换入换出（swap）机制，以及中断-“故障中断”、缺页故障处理等，实现基于页的内存替换算法；
4. 内核线程子系统，用于了解如何创建相对与用户进程更加简单的内核态线程，如果对内核线程进行动态管理等；
5. 用户进程管理子系统，用于了解用户态进程创建、执行、切换和结束的动态管理过程，了解在用户态通过系统调用得到内核态的内核服务的过程；
6. 处理器调度子系统，用于理解操作系统的调度过程和调度算法；
7. 同步互斥与进程间通信子系统，了解进程间如何进行信息交换和共享，并了解同步互斥的具体实现以及对系统性能的影响，研究死锁产生的原因，以及如何避免死锁；
8. 文件系统，了解文件系统的具体实现，与进程管理等的关系，了解缓存对操作系统IO访问的性能改进，了解虚拟文件系统（VFS）、buffer cache和disk driver之间的关系。

## 了解编程开发调试的基本工具

### `AT&T` 汇编基本语法

基于 x86 架构 的处理器所使用的汇编指令一般有两种格式 : 

- Intel 汇编
	- DOS(8086处理器), Windows
	- Windows 派系 -> VC 编译器
- AT&T汇编
	- Linux, Unix, Mac OS, iOS(模拟器)
	- Unix派系 -> GCC编译器

![两种汇编语言简要说明](../pic/两种汇编语言简要说明.png)

### 寄存器和汇编的一些总结

e 表示 extend 的意思是扩展到了 32 位

eax/rax		accumulator register 返回值
ebx/rbx		base register 基址寄存器
ecx/rcx		counter register 计数器
edx/rdx		data register 数据寄存器

esi/rsi		source index 源地址指针寄存器
edi/rdi		destination index 目的地址指针寄存器
ebp/rbp		base pointer 基址指针寄存器
esp/rsp		stack pointer 堆栈指针寄存器

>其实现在大部分的寄存器都已经没有特殊功能了，只不过还保留了原始的命名方式。 `%rbp` 指向栈底， `%rsp` 指向栈顶， `%rip` 指向程序当前执行的地址。

## 

https://bbs.csdn.net/topics/392552998
http://www.ruanyifeng.com/blog/2018/01/assembly-language-primer.html

一般函数调用的开始都会执行 `pushq %rbp`

假设我们的代码如下

```cpp
void base();
void invoked();

void base()
{
    invoked();
}
```

在函数开始的时候，有一个基准地址，这个地址就是 invoked(); 这一行代码的地址，这个地址存放在 %rbp 中。

程序执行到 invoked() 函数内部时，首先我们需要将 %rbp 地址存起来，否则我们不知道当 invoked() 函数执行结束时下一步应该执行哪里的代码。

pushq %rbp 等同于以下两条指令

subq	$8, %rsp
moveq	%rbp, %rsp

意思就是，首先将栈指针 -8，这样 %rsp 指向的位置就是我们的被调用函数的栈开始的地址；将 %rbp 压入到函数栈开始的地方，这样当我们退出函数的时候我们就知道跳到哪里执行了

### 程序 DEBUG

```cpp
#include "stdio.h"

int add(int a, int b, int c)
{
	return a + b + c;
}

void invoke()
{
	add(1, 2, 3);
}

int main(void)
{
	invoke();
	return 0;
}
```

>在 ubuntu 下使用 `gcc` 编译后， gdb 执行

```
>>> p main
$1 = {<text variable, no debug info>} `0x400523` <main>
>>> p invoke 
$2 = {<text variable, no debug info>} `0x400509` <invoke>
>>> p add 
$3 = {<text variable, no debug info>} `0x4004ed` <add>
```

### 在 main 函数中

rbp 0x0000000000000000
rsp 0x00007fffffffded0

### 在 invoke 函数中

>在 invoke() 调用之前有一条指令 `mov %rsp %rbp` 修改了 %rsp 的值

rbp 0x00007fffffffded0
rsp 0x00007fffffffdec8

### 在 add 调用之前

rbp 0x00007fffffffdec0
rsp 0x00007fffffffdeb0

rdx 0x0000000000000003
rsi 0x0000000000000002
rdi 0x0000000000000001

### 在 add 调用之后

rbp 0x00007fffffffdec0
rsp 0x00007fffffffdea8

### 总结

结合我们之前提到的： `%rsp` 指向栈顶， `%rbp` 指向栈底。

每一个函数都拥有自己的栈，并且这个栈顶的位置存放在 `%rbp` 中，在函数开始执行的时候， **`%rsp` 的值会被自动的修改到指向新的栈的值**，而我们的程序则需要做如下操作：

1. 存放 `%rbp` 的值，因为我们待会需要修改它
2. 修改 `%rsp` 的值 == `%rbp`，因为现在的 %rbp 指向的是父函数的 base pointer，我们需要保存这个值，以便于当我们调用其他的函数时其他函数也能正常返回。
3. 对于每一个局部变量，我们使用 `%rsp` 来作为栈顶指针存放数据，每存放一个数据都需要修改 %rsp 的值。

在退出函数的时候则比较简单，只需要恢复刚才我们存储的 `%rbp` 地址即可。

所以我们的函数调用代码典型的是如下所示：

```asm
pushq	%rbp				# 保存上一个函数的 base pointer 到栈（注意， %rsp 的不需要被调用者修改）
mov 	%rsp %rbp			# 保存自身的 base pointer 以便于我们调用其他函数的时候能正常返回
subq 	$16, %rsp			# 开辟栈空间

# statement
# 记住一个问题，在访问局部变量的时候，是通过 %rbp 来访问的而不是 %rsp，因为 %rsp 有可能被它调用的函数改变

popq	%rbp				# 恢复上一个函数的 base pointer
ret							# 返回
```

## GCC基本内联汇编

### 基本内联汇编语句

```asm
    asm("nop"); asm("cli");
```

"asm" 和 "__asm__" 的含义是完全一样的。如果有多行汇编，则每一行都要加上 "\n\t"。其中的 “\n” 是换行符，"\t” 是 tab 符，在每条命令的 结束加这两个符号，是为了让 gcc 把内联汇编代码翻译成一般的汇编代码时能够保证换行和留有一定的空格。对于基本asm语句，GCC编译出来的汇编代码就是双引号里的内容。例如：

```asm
        asm( "pushl %eax\n\t"
             "movl $0,%eax\n\t"
             "popl %eax"
        );
```

实际上gcc在处理汇编时，是要把asm(...)的内容"打印"到汇编文件中，所以格式控制字符是必要的。再例如：

```asm
    asm("movl %eax, %ebx");
    asm("xorl %ebx, %edx");
    asm("movl $0, _boo);
```

在上面的例子中，由于我们在内联汇编中改变了 edx 和 ebx 的值，但是由于 gcc 的特殊的处理方法，即先形成汇编文件，再交给 GAS 去汇编，所以 GAS 并不知道我们已经改变了 edx和 ebx 的值，如果程序的上下文需要 edx 或 ebx 作其他内存单元或变量的暂存，就会产生没有预料的多次赋值，引起严重的后果。对于变量 `_boo` 也存在一样的问题。为了解决这个问题，就要用到扩展 GCC 内联汇编语法。

### GCC扩展内联汇编

```asm
#define read_cr0() ({ \
unsigned int __dummy; \
__asm__( \
    "movl %%cr0,%0\n\t" \
	    :"=r" (__dummy)); \
		__dummy; \
		})
```

它代表什么含义呢？这需要从其基本格式讲起。GCC扩展内联汇编的基本格式是： 

```asm
asm [volatile] ( Assembler Template
   : Output Operands
      [ : Input Operands
	     [ : Clobbers  ] ])
```

## 使用 qemu 模拟

```bash
# 在gdb命令行界面下，使用下面的命令连接到qemu
target remote 127.0.0.1:1234
# 为了让gdb获知符号信息，需要指定调试目标文件，gdb中使用file命令：
file ./bin/kernel
```

## 加载调试目标

在进行gdb本地应用程序调试的时候，因为在指定了执行文件时就已经加载了文件中包含的调试信息，因此不用再使用gdb命令专门加载了。

但是在使用qemu进行远程调试的时候，我们必须手动加载符号表，也就是在gdb中用file命令。

这样加载调试信息都是按照elf文件中制定的虚拟地址进行加载的，这在静态连接的代码中没有任何问题。但是在调试含有动态链接库的代码时，动态链接库的ELF执行文件头中指定的加载虚拟地址都是0，这个地址实际上是不正确的。从操作系统角度来看，用户态的动态链接库的加载地址都是由操作系统动态分配的，没有一个固定值。然后操作系统再把动态链接库加载到这个地址，并由用户态的库链接器（linker）把动态链接库中的地址信息重新设置，自此动态链接库才可正常运行。

```gdb
(gdb) add-symbol-file android_test/system/bin/linker 0x6fee6180
```

## 了解处理器硬件

### Intel 80386运行模式

80386处理器有四种运行模式：实模式、保护模式、SMM模式和虚拟8086模式。这里对涉及ucore的实模式、保护模式做一个简要介绍。

`实模式`：这是个人计算机早期的8086处理器采用的一种简单运行模式，当时微软的MS-DOS操作系统主要就是运行在8086的实模式下。80386加电启动后处于实模式运行状态，在这种状态下软件可访问的物理内存空间不能超过1MB，且无法发挥Intel 80386以上级别的32位CPU的4GB内存管理能力。

**实模式将整个物理内存看成分段的区域，程序代码和数据位于不同区域，操作系统和用户程序并没有区别对待，而且每一个指针都是指向实际的物理地址。这样用户程序的一个指针如果指向了操作系统区域或其他用户程序区域，并修改了内容，那么其后果就很可能是灾难性的。**

`保护模式`： **保护模式的一个主要目标是确保应用程序无法对操作系统进行破坏**，

实际上，80386就是通过在实模式下初始化控制寄存器（如GDTR，LDTR，IDTR与TR等管理寄存器）以及页表，然后再通过设置CR0寄存器使其中的保护模式使能位置位，从而进入到80386的保护模式。

当80386工作在保护模式下的时候，其所有的32根地址线都可供寻址，物理寻址空间高达4GB。在保护模式下，支持内存分页机制，提供了对虚拟内存的良好支持。保护模式下80386支持多任务，还支持优先级机制，不同的程序可以运行在不同的特权级上。特权级一共分0～3四个级别，操作系统运行在最高的特权级0上，应用程序则运行在比较低的级别上；配合良好的检查机制后，既可以在任务间实现数据的安全共享也可以很好地隔离各个任务。

### Intel 80386内存架构

一般而言，内存地址有两个：一个是CPU通过总线访问物理内存用到的物理地址，一个是我们编写的应用程序所用到的逻辑地址（也有人称为虚拟地址）

```c
int boo=1;
int *foo=&foo;
```

物理内存地址空间是处理器提交到总线上用于访问计算机系统中的内存和外设的最终地址。一个计算机系统中只有一个物理地址空间。

线性地址空间是80386处理器通过段（Segment）机制控制下的形成的地址空间。在操作系统的管理下，**每个运行的应用程序有相对独立的一个或多个内存空间段**，每个段有各自的起始地址和长度属性，大小不固定，这样可让多个运行的应用程序之间相互隔离，实现对地址空间的保护。

80386处理器的段管理功能单元负责把虚拟地址转换成线性地址，在没有下面介绍的页机制启动的情况下，这个线性地址就是物理地址

相对而言，**段机制对大量应用程序分散地使用大内存的支持能力较弱**。所以Intel公司又加入了页机制，每个页的大小是固定的（一般为4KB），也可完成对内存单元的安全保护，隔离，且可有效支持大量应用程序分散地使用大内存的情况。

>分段机制和分页机制都启动：逻辑地址---> **段机制处理** --->线性地址---> **页机制处理** --->物理地址

### 内存地址总结

操作系统会为每个进程分配一个独立的虚拟内存片，每个虚拟内存的地址都是从 `[0, max_memory]` 区间的一个值。

当我们程序去访问一个虚拟内粗地址（逻辑地址）的时候，首先会经过段处理机制将虚拟地址转换为一个线性地;

由于页的存在，每个线性地址会被分为很多页，而在实际运行过程中，内存的页有可能是在实际的内存中，有可能在硬盘上，这些都需要去进行计算才能得到最后的物理地址。

### Intel 80386寄存器

80386的寄存器可以分为8组：通用寄存器，段寄存器，指令指针寄存器，标志寄存器，系统地址寄存器，控制寄存器，调试寄存器，测试寄存器，它们的宽度都是32位。

**一般程序员看到的寄存器包括通用寄存器，段寄存器，指令指针寄存器，标志寄存器。**

## 了解ucore编程方法和通用数据结构

uCore的面向对象编程方法，目前主要是采用了类似C++的接口（interface）概念，即是让实现细节不同的某类内核子系统（比如物理内存分配器、调度器，文件系统等）有共同的操作方式，这样虽然内存子系统的实现千差万别，但它的访问接口是不变的。这样不同的内核子系统之间就可以灵活组合在一起，实现风格各异，功能不同的操作系统。

**接口在 C 语言中，表现为一组函数指针的集合。放在 C++ 中，即为虚表。**

接口设计的难点是如果找出各种内核子系统的共性访问/操作模式，从而可以根据访问模式提取出函数指针列表。

比如对于uCore内核中的物理内存管理子系统， **首先通过分析内核中其他子系统可能对物理内存管理子系统** ，明确物理内存管理子系统的访问/操作模式，然后我们定义了pmm_manager数据结构（位于lab2/kern/mm/pmm.h）如下： 

```c
// pmm_manager is a physical memory management class. A special pmm manager - XXX_pmm_manager
// only needs to implement the methods in pmm_manager class, then XXX_pmm_manager can be used
// by ucore to manage the total physical memory space.
struct pmm_manager {
    // XXX_pmm_manager's name
    const char *name;  
    // initialize internal description&management data structure
    // (free block list, number of free block) of XXX_pmm_manager 
    void (*init)(void); 
    // setup description&management data structcure according to
    // the initial free physical memory space 
    void (*init_memmap)(struct Page *base, size_t n); 
    // allocate >=n pages, depend on the allocation algorithm 
    struct Page *(*alloc_pages)(size_t n);  
    // free >=n pages with "base" addr of Page descriptor structures(memlayout.h)
    void (*free_pages)(struct Page *base, size_t n);   
    // return the number of free pages 
    size_t (*nr_free_pages)(void);                     
    // check the correctness of XXX_pmm_manager
    void (*check)(void);                               
};
```

这样基于此数据结构，我们可以实现不同连续内存分配算法的物理内存管理子系统，而这些物理内存管理子系统需要编写算法，把算法实现在此结构中定义的init（初始化）、init_memmap（分析空闲物理内存并初始化管理）、alloc_pages（分配物理页）、free_pages（释放物理页）函数指针所对应的函数中。而其他内存子系统需要与物理内存管理子系统交互时，只需调用特定物理内存管理子系统所采用的pmm_manager数据结构变量中的函数指针即可

## 通用数据结构

### 双向循环链表

```c
// 常用的数据结构
typedef struct foo {
    ElemType data;
	struct foo *prev;
	struct foo *next;		
} foo_t;
```

这种双向循环链表数据结构的一个潜在问题是，虽然链表的基本操作是一致的，但由于每种特定数据结构的类型不一致，需要为每种特定数据结构类型定义针对这个数据结构的特定链表插入、删除等各种操作，会导致代码冗余。

```c
// 假设有如下结构题
typedef struct bar {
    ElemType2 data;
	struct foo *prev;
	struct foo *next;		
} bar_t;

// 就必须定义两个不同的 insert 方法
void insert(foo *f ElemType data);
void insert(foo *f ElemType2 data);
```

uCore的双向链表结构定义为：

```c
struct list_entry
{
	struct list_entry *prev, next;
};
```

**需要注意uCore内核的链表节点list_entry没有包含传统的data数据域，，而是在具体的数据结构中包含链表节点。**

```c
/* free_area_t - maintains a doubly linked list to record free (unused) pages */
typedef struct {
    list_entry_t free_list;         // the list header
    unsigned int nr_free;           // # of free pages in this free list
} free_area_t;
```

而每一个空闲块链表节点定义（位于lab2/kern/mm/memlayout）为：

```c
/* *
 * struct Page - Page descriptor structures. Each Page describes one
 * physical page. In kern/mm/pmm.h, you can find lots of useful functions
 * that convert Page to other data types, such as phyical address.
 * */
struct Page {
    atomic_t ref;          // page frame's reference counter
    ……
    list_entry_t page_link;         // free list link
};
```

### 双线链表使用的宏

```c
// 在 c 语言没有模板的情况下通用的双向链表

#include "iostream"

// convert list entry to page
#define le2page(le, member) to_struct((le), struct page, member)

// Return the offset of 'member' relative to the beginning of a struct type
#define offset_of(type, member) \
((size_t)(&((type *)0)->member))

/**
 * to_struct - get a struct from a ptr
 *
 * @ptr: a struct pointer of member
 * @type: the type of the struct this is embedded in
 * @member: the name of the member within the struct
 */
#define to_struct(ptr, type, member) \
((type *)((char *)(ptr) - offset_of(type, member)))

struct list_entry
{
	list_entry *next, *prev;
};

struct page
{
	int        value;
	list_entry page_link;
};

int main(int argc, char **argv)
{
	page p{1024, nullptr};
	std::cout << offset_of(page, page_link) << std::endl;
	// 8
	page *pt = to_struct(&p.page_link, page, page_link);
	std::cout << pt->value << std::endl;
	// 1024
}
```

### offset_of

```c
// Return the offset of 'member' relative to the beginning of a struct type
#define offset_of(type, member) \
((size_t)(&((type *)0)->member))
```

这个宏很简单，就是将0转换为一个 `type` 类型的指针，然后对得到的指针取 member 变量，那么就得到了 member 变量的相对于 0 的地址，也就是这个在整个 struct 中的偏移量。

### to_struct

```c
#define to_struct(ptr, type, member) \
((type *)((char *)(ptr) - offset_of(type, member)))
```

这个宏的参数是一个指针，这个指针指向了结构体中的某一个元素，type 则表示这个结构体的类型，member 则表示这个指针指向的元素的成员变量名。

首先使用 type 和 member 求出成员在结构体中的偏移量。将指向结构体中元素的指针减去这个偏移量就得到 `结构体的指针地址`，再将这个指针地址转换为 `(type *)` 就得到了我们想要的元素的指针。

要注意的地方是，在修改指针的值的时候，必须先将指针转换为 `(char *)`，否则实际得到的结果应该是 - sizeof(type) * offset_of(type, member) 的结果

### 其他

修改宏到如下也可以得到正确的结果：

```c
#define offset_of(type, member) \
((size_t)(&((type *)100)->member))

#define to_struct(ptr, type, member) \
((type *)((char *)(ptr) - (offset_of(type, member) - 100)))
```
