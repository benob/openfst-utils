CPPFLAGS:=$(CFLAGS) -lfst -g -Wall -ldl --std=c++11
all: fstcompile-nolex add-tags ngram-expand fstminimize-transducer fstdeterminize-tc-lex fstsuperfinal-noepsilon fstcompose-maplex fstoracle fstposteriors fstcompose-specials fstprint-nbest-strings
%: %.cc
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -o $@ $<
clean: 
	rm -f $(shell grep ^all: Makefile | cut -f2- -d" ")
