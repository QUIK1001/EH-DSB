bits 16
org 0x7E00

start:
    mov [boot_drive], dl
    
    mov si, msg_loading
    call print_string
    
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000
    
    mov ah, 0x02
    mov al, 128
    mov ch, 0x00
    mov cl, 0x0C
    mov dh, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    
    cmp al, 128
    jne disk_error
    
    mov si, msg_success
    call print_string
    
    mov si, ascii_art
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
    db 0x9A
    db 0xCF
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
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

times 5120-($-$$) db 0
