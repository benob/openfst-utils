CPPFLAGS:=-lfst -g -Wall
all: fstcompile2 add-tags ngram-expand
ngram-expand: ngram-expand.cc
fstcompile2: fstcompile2.cc
add-tags: add-tags.cc
