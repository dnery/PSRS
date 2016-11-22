# EDIT THESE ONLY
SRCDIR = ./src/
OBJDIR = ./obj/
PARENT = parent
WORKER = worker
OBJS   = qsort.o utils.o

# Path resolution
OBJP   = $(addprefix $(OBJDIR), $(OBJS))

# Comiler and flags
CCOMP  = mpicc
CFLAGS = -Wall -Wextra --pedantic-errors -O0 -g -fopenmp

all: $(OBJDIR) $(OBJP)
	$(CCOMP) $(CFLAGS) $(SRCDIR)$(PARENT).c $(OBJP) -o $(PARENT)
	$(CCOMP) $(CFLAGS) $(SRCDIR)$(WORKER).c $(OBJP) -o $(WORKER)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJP): $(OBJDIR)%.o: $(SRCDIR)%.c $(SRCDIR)%.h
	$(CCOMP) $(CFLAGS) -c $< -o $@

# If the first argument is "run":
ifeq (run, $(firstword $(MAKECMDGOALS)))
  # Use the rest as arguments for "run"...
  ARGLIST := $(wordlist 2, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(ARGLIST):;@:)
endif

$(PARENT):
	# do-nothing

.PHONY: run clean

run:
	./$(PARENT) $(ARGLIST)

clean:
	rm -f $(PARENT) $(WORKER) $(OBJP)
