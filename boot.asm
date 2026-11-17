bits 32
section .multiboot
    align 4
    dd 0x1BADB002
    dd 0x00000007
    dd -(0x1BADB002 + 0x00000007)
    dd 0, 0, 0, 0, 0
    dd 0
    dd 800
    dd 600
    dd 32

section .text
global start
extern kernel_main
start:
    cli
    mov esp, stack_space
    push ebx
    call kernel_main
    hlt

section .bss
resb 16384
stack_space:
