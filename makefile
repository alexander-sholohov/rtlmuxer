CC        =g++
CFLAGS    =-c -Wall
LDFLAGS   =
INCLUDE   = 
OBJDIR    = ./
OBJLIST   = muxer_main.o buffer.o tcp_sink.o sink_list.o tcp_source.o
OBJECTS   = $(addprefix $(OBJDIR), $(OBJLIST) )


GOAL=rtlmuxer

all:$(GOAL)

$(GOAL): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJECTS): %.o: %.cpp
	$(CC) $(CFLAGS) $? -o $@ $(INCLUDE)

clean:
	rm -f $(GOAL)
	rm -rf *.o
