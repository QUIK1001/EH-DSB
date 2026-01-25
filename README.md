<h1 align="center">EHDSB</h1>
<h2 align="center">>Experimental project!</h2>
<h3 align="center">by quik/QUIK1001</h3>
  
<p align="center">
  <br>
  <img src="https://img.shields.io/badge/EH-DSB*beta_1-green" alt="Version:">
  <img src="https://img.shields.io/badge/License-Unlicense-yellow" alt="License:">
  <img src="https://img.shields.io/badge/Linux-black" alt="Platform">
</p>

## Event Horizon Dual-Stage Bootloader

# This project is in beta!

### Written from scratch in Assembly/C++.

# To run:
## install base-devel, nasm, and qemu-desktop on your Linux distribution and run in the project folder:
```bash
make all run3
```
Or follow the instructions:
```bash
make help
```
# 
## Main components:
Two-stage bootloader (boot.asm, boot1.asm) - loads the kernel into memory and switches to protected mode

C++ microkernel (kernel3.cpp) - main functionality

## Functionality:

- VGA-terminal (80×25)
- keyboard support
- Real-time clock

### File system in RAM:
- Simple in-memory file system
- Support for up to 16 files (up to 2 KB each)
- Standard files: README.TXT, HELP.TXT, SYSTEM.CFG

### Applications:

1. Text Editor
 - Saving/loading files
 - F4 support for saving

2. Calculator
 - Basic arithmetic

3. Brainfuck "IDE"
 - a "simple" development environment for Brainfuck
   
4. simple File manager
 - view files

<img width="720" height="404" alt="изображение" src="https://github.com/user-attachments/assets/54b083a0-8e9f-4827-acf5-0a3b346f886b" />
<img width="718" height="397" alt="изображение" src="https://github.com/user-attachments/assets/db7a0fcf-fc52-4dc6-96d6-a67db3379ed1" />

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/QUIK1001/EH-DSB)
