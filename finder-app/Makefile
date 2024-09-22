SHELL:= /bin/sh

#When I type make all, it will run the steps to compile writer
all: writer


#The $(var_name) essentially says hey, if CROSS_COMPILE is define when calling make, put it there
#-g specifies we want debugging information
#-Wall specifies we want all the warning messaages that GCC can produce
#-c specifies that we want to generate a .o file
#-o specifies the name of the output file
# -I/ specifies the directories to be searched

#This line is telling us that how to build writer. But  in order to do that it needs writer.o (which is generated below)
writer: writer.o
	$(CROSS_COMPILE)gcc -g -Wall -I/ writer.o -o writer

#This line is telling us that how to build writer.o. But  in order to do that it needs writer.c (which we have written)
writer.o: writer.c
	$(CROSS_COMPILE)gcc -g -Wall -c -o writer.o writer.c

# -f specifies that we want to run the remove command without asking for confirmation from the user
# This command is removing any .o files, .elf files, .map files, and the writer file
clean:
	rm -f *.o *.elf *.map writer