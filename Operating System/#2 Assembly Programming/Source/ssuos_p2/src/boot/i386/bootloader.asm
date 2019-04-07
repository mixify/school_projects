org	0x7c00   

[BITS 16]

START:   
		jmp		BOOT1_LOAD;BOOT1_LOAD로 점프

BOOT1_LOAD:
	mov     ax, 0x0900 
        mov     es, ax
        mov     bx, 0x0

        mov     ah, 2	
        mov     al, 0x4		
        mov     ch, 0	
        mov     cl, 2	
        mov     dh, 0		
        mov     dl, 0x80

        int     0x13	
        jc      BOOT1_LOAD

mov cl, 0
CLS:
mov al, 0x02
mov ah, 0x00
int 0x10

PRINT:
mov dl, 0

call print_selected
mov si, ssuos_1
call PrintString
mov dl, 1

call print_selected
mov si, ssuos_2
call PrintString
mov dl, 2

call print_selected
mov si, ssuos_3
call PrintString

get_key:
mov ah, 0
int 0x16

cmp al, 0x0d
je ENTER
cmp ah, 0x48
je UP
cmp ah, 0x50
je DOWN
jmp get_key
UP:
cmp cl, 0
je CLS
dec cl
jmp CLS
DOWN:
cmp cl, 2
je CLS
inc cl
jmp CLS
ENTER:
cmp cl, 0
je KERNEL_LOAD1
cmp cl, 1
je KERNEL_LOAD2
cmp cl, 2
je KERNEL_LOAD3
;jmp $


print_selected:
cmp cl, dl
jne exit_print_selected
mov si, select
call PrintString
mov dl, 16
exit_print_selected:
ret

PrintString:
call skip_select
next_character:
mov al, [si]
inc si
or al,al
jz exit_function
call PrintCharacter
jmp next_character
skip_select:
cmp dl, 16
jne exit_skip 
add si, 3
exit_skip:
ret
exit_function:
cmp cl, dl
je skip_next_line
mov al, 0x0a
call PrintCharacter
mov al, 0x0d
call PrintCharacter
skip_next_line:
ret

PrintCharacter:
mov ah, 0x0e
mov bh, 0x00
mov bl, 0x07
int 0x10
ret



KERNEL_LOAD1:
	mov     ax, 0x1000 
        mov     es, ax
        mov     bx, 0x0

        mov     ah, 2	
        mov     al, 0x3f		
        mov     ch, 0	
        mov     cl, 0x6	
        mov     dh, 0		
        mov     dl, 0x80

        int     0x13	
        jc      KERNEL_LOAD1

jmp		0x0900:0x0000


KERNEL_LOAD2:
	mov     ax, 0x1000
        mov     es, ax
        mov     bx, 0x0

        mov     ah, 2
        mov     al, 0x3f
        mov     ch, 0x09;cylinder num
        mov     cl, 0x2f;sector num
        mov     dh, 0x0e;head num 
        mov     dl, 0x80  

        int     0x13
        jc      KERNEL_LOAD2

jmp		0x0900:0x0000

KERNEL_LOAD3:
	mov     ax, 0x1000
        mov     es, ax
        mov     bx, 0x0

        mov     ah, 2
        mov     al, 0x3f
        mov     ch, 0x0e;cylinder num
        mov     cl, 0x07;sector num
        mov     dh, 0x0e;head num
        mov     dl, 0x80  

        int     0x13
        jc      KERNEL_LOAD3


jmp		0x0900:0x0000




select db "[O]",0
ssuos_1 db "[ ] SSUOS_1",0
ssuos_2 db "[ ] SSUOS_2",0
ssuos_3 db "[ ] SSUOS_3",0
ssuos_4 db "[ ] SSUOS_4",0
partition_num : resw 1

times   446-($-$$) db 0x00

PTE:
partition1 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition2 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition3 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x98, 0x3a, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition4 db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
times 	510-($-$$) db 0x00
dw	0xaa55
