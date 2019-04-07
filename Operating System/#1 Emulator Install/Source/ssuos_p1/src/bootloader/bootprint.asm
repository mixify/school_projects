[BITS 16]

org 0x7C00

;video memory
mov ax, 0xB800;
mov es, ax; 

;clear screen
mov ax, [Background]
mov bx, 0
mov cx, 80*25*2
CLS:
mov [es:bx], ax
add bx, 1
loop CLS

;print
START:
mov si, msg
call PrintString
jmp $

PrintString:
mov bx, 0
next_character:
mov al, [si]
inc si
or al,al
jz exit_function
call PrintCharacter
add bx, 2
jmp next_character
exit_function:
ret

PrintCharacter:
mov ah, 0x09
mov [es:bx], ax
;int 0x10
ret


;data
Background db 0x00
msg db "Hello Sunmoo's World!", 0


times 510-($-$$) db 0x00
dw 0xaa55

