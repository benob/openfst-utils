#include <fst/fstlib.h>
#include <fst/script/fstscript.h>

using namespace fst;
using namespace fst::script;
int main(int argc, char** argv) {
    FstClass *ifst = FstClass::Read("");
    VectorFstClass ofst(ifst->ArcType());
    RmEpsilon(*ifst, &ofst);
    Encode(&ofst, kEncodeLabels|kEncodeWeights, false, "tmp.encoded");
    Determinize(ofst, &ofst);
    Minimize(&ofst, &ofst);
    Decode(&ofst, "tmp.encoded");
    ofst.Write("");
}
