#include <fst/fstlib.h>

using namespace std;
using namespace fst;

typedef Fst<LogArc> LogFst;
typedef PhiMatcher< SortedMatcher<LogFst> > PM;

int main(int argc, char** argv) {
    if(argc != 2) {
        cerr << "usage: cat <input1> | " << argv[0] << " <phi-symbol> <input2>\n";
        return 1;
    }
    int phiLabel = atoi(argv[1]);
    VectorFst<LogArc>* mutableInput1 = VectorFst<LogArc>::Read(argv[2]);
    VectorFst<LogArc>* mutableInput2 = VectorFst<LogArc>::Read("");
    Fst<LogArc>* input1(mutableInput1);
    ArcSort(*input1, OLabelCompare<LogArc>()); 
    ComposeFstOptions<LogArc, PM> opts;
    opts.gc_limit = 0;
    opts.matcher1 = new PM(input1, MATCH_OUTPUT, phiLabel);
    opts.matcher2 = new PM(input2, MATCH_NONE, kNoLabel);
    // composition
    ComposeFst<LogArc> output(*input1, *input2, opts);
    output.Write("");
}
