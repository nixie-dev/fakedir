libfakedir.dylib: fakedir.o trivial_replacements.o
	$(CC) -shared $^ -o $@

fakedir.o: fakedir.c common.h trivial_replacements.h
trivial_replacements.o: trivial_replacements.c common.h
