CPPFLAGS:=-lfst -g -Wall
all: fstcompile2 add-tags ngram-expand 
%: %.cc
clean: 
	rm -f $(shell grep ^all: Makefile | cut -f2- -d" ")
