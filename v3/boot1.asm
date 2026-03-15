bits 16
org 0x7E00

start:
    mov [boot_drive], dl

    mov si, msg_loading
    call print_string

    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov ah, 0x02
    mov al, 128
    mov ch, 0
    mov cl, 12
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    cmp al, 128
    jne disk_error

    mov si, msg_success
    call print_string

    call delay_1sec

    mov si, ascii_art
    call print_string

    mov si, license_text
    call print_string

    call delay_2sec

    cli

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode

disk_error:
    mov si, msg_error
    call print_string
    jmp $

print_string:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

delay_1sec:
    pusha
    mov ah, 0x00
    int 0x1A
    mov bx, dx
    add bx, 36
.wait:
    int 0x1A
    cmp dx, bx
    jl .wait
    popa
    ret

delay_2sec:
    pusha
    mov ah, 0x00
    int 0x1A
    mov bx, dx
    add bx, 36
.wait:
    int 0x1A
    cmp dx, bx
    jl .wait
    popa
    ret

bits 32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000

    jmp 0x08:0x10000

gdt_start:
    dq 0x0000000000000000

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

msg_loading db 'Loading EH-dsb...', 0
msg_success db ' OK', 0x0D, 0x0A, 0
msg_error db ' Error!', 0
boot_drive db 0

ascii_art db 0x0D, 0x0A
          db '__________  ______________', 0x0D, 0x0A
          db '| __| || |  |   \/ __|| _ )  ', 0x0D, 0x0A
          db '| _|| __ |  | |) \__ \| _ \  ', 0x0D, 0x0A
          db '|___|_||_|  |___/|___/|___/ ', 0x0D, 0x0A, 0

license_text db 0x0D, 0x0A
             db 'This is free and unencumbered software released into the public domain.', 0x0D, 0x0A
             db 'Anyone is free to copy, modify, publish, use, compile, sell, or', 0x0D, 0x0A
             db 'distribute this software, either in source code form or as a compiled', 0x0D, 0x0A
             db 'binary, for any purpose, commercial or non-commercial, and by any', 0x0D, 0x0A
             db 'means.', 0x0D, 0x0A
             db 'In jurisdictions that recognize copyright laws, the author or authors', 0x0D, 0x0A
             db 'of this software dedicate any and all copyright interest in the', 0x0D, 0x0A
             db 'software to the public domain. We make this dedication for the benefit', 0x0D, 0x0A
             db 'of the public at large and to the detriment of our heirs and', 0x0D, 0x0A
             db 'successors. We intend this dedication to be an overt act of', 0x0D, 0x0A
             db 'relinquishment in perpetuity of all present and future rights to this', 0x0D, 0x0A
             db 'software under copyright law.', 0x0D, 0x0A
             db 'THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,', 0x0D, 0x0A
             db 'EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF', 0x0D, 0x0A
             db 'MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.', 0x0D, 0x0A
             db 'IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR', 0x0D, 0x0A
             db 'OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,', 0x0D, 0x0A
             db 'ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR', 0x0D, 0x0A
             db 'OTHER DEALINGS IN THE SOFTWARE.', 0x0D, 0x0A
             db 'For more information, please refer to <https://unlicense.org>', 0x0D, 0x0A, 0

times 5120-($-$$) db 0
