CPPFLAGS:=-lfst -g -Wall
all: fstcompile2 add-tags ngram-expand 
%: %.cc
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -o $@ $<
clean: 
	rm -f $(shell grep ^all: Makefile | cut -f2- -d" ")
