CPPFLAGS:=-lfst -lfstscript -g -Wall
all: fstcompile-nolex add-tags ngram-expand fstminimize-transducer fstdeterminize-lexicographic
%: %.cc
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -o $@ $<
clean: 
	rm -f $(shell grep ^all: Makefile | cut -f2- -d" ")
