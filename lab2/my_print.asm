[section .data]

setcolor db 1Bh, '[49;34m', 0 
    .len equ $ - setcolor
basecolor db 1Bh, '[0;0m', 0 
    .len equ $ - basecolor

[section .text]
global my_print
global change_color
global ret_color
my_print:
	mov	edx,[esp+8]//length
	mov	ecx,[esp+4]//ptr
	mov	ebx,1//console
	mov	eax,4//write
	int	0x80
	ret

change_color:
    mov eax, 4
    mov ebx, 1
    mov ecx, setcolor
    mov edx, setcolor.len
    int 80h
    ret

ret_color:
	mov eax, 4
    mov ebx, 1
    mov ecx, basecolor
    mov edx, basecolor.len
    int 80h
    ret
