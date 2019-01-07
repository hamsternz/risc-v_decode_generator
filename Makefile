all : riscv-decode riscv-decode.in
	./riscv-decode riscv-decode.in

riscv-decode : riscv-decode.c
	gcc -o riscv-decode riscv-decode.c -Wall -pedantic -O4
