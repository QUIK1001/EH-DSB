bits 16
org 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    
    mov [boot_drive], dl
    
    mov ax, 0x07E0
    mov es, ax
    mov bx, 0x0000
    mov ah, 0x02
    mov al, 10
    mov ch, 0x00
    mov cl, 0x02
    mov dh, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    
    jmp 0x07E0:0x0000

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

msg_error db 'Boot error!', 0
boot_drive db 0

times 510-($-$$) db 0
dw 0xAA55
