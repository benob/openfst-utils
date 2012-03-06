#include <fst/fstlib.h>

using namespace fst;

int main(int argc, char** argv) {
    EncodeMapper<StdArc> mapper(kEncodeLabels, ENCODE);
    StdVectorFst *ifst = StdVectorFst::Read("");
    RmEpsilon(ifst);
    Encode(ifst, &mapper);
    Determinize(*ifst, ifst);
    Minimize(ifst);
    Decode(ifst, mapper);
    ifst->Write("");
}
