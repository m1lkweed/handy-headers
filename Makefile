C_FLAGS += -znoexecstack -O2 --std=gnu2x -lm

.PHONY: clean

prog: *.c
	@gcc $^ $(C_FLAGS) -o $@
	@echo Program compiled!
clean: prog
	@-rm -vfr *~ $^
