CC=g++
CFLAGS=-I. -libverbs -lpthread


APP      = rdma_probe

SRCEXT   = c
INCEXT   = h
SRCDIR   = src
OBJDIR   = obj

SRCS    := $(shell find $(SRCDIR) -name '*.$(SRCEXT)')
DEPS    := $(shell find $(SRCDIR) -name '*.$(INCEXT)')
OBJS    := $(patsubst $(SRCDIR)/%.$(SRCEXT),$(OBJDIR)/%.o,$(SRCS))

all: mk_dir $(APP)


$(APP): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.$(SRCEXT) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


mk_dir:
	mkdir -p $(OBJDIR)

.PHONY : clean
clean :
		rm -r $(OBJDIR)
		rm $(APP)