out : my_print.o main.c
	gcc -o out my_print.o main.c
my_print.o : my_print.asm
	nasm -f elf -o my_print.o my_print.asm
clean : 
	rm out my_print.o
