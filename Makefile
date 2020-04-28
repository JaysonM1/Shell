sshell : sshell.c sshell.h
	gcc -o sshell sshell.c get_path.c -lpthread
clean :
	rm -f *.exe *~ *.o *.out
	rm sshell

