
sendfile:	read-write.c
	clang -o read-write read-write.c;
run:
	./read-write

clean:
	rm -f  ./read-write ./read-write.o