CFLAGS = -Wall -g  
LDFLAGS =  

OBJS = main.o

all: shell 

shell: $(OBJS)
	$(CC) $(CFLAGS) -o shell $(OBJS) $(LDFLAGS) -lreadline

clean: 
	rm -rf $(OBJS) shell
