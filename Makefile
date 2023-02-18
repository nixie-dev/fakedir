libfakedir.dylib: fakedir.o trivial_replacements.o execve.o
	$(CC) $(CFLAGS) -shared $^ -o $@

fakedir.o: fakedir.c common.h trivial_replacements.h execve.h
trivial_replacements.o: trivial_replacements.c common.h
execve.o: execve.c common.h
