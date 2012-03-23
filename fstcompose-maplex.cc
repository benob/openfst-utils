#include <fst/fstlib.h>

using namespace fst;

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cerr << "usage: " << argv[0] << " <fst1> <fst2>\n";
        return 1;
    }
    StdVectorFst *input1 = StdVectorFst::Read(argv[1]);
    StdVectorFst *input2 = StdVectorFst::Read(argv[2]);
    bool relabel;
    SymbolTable *symbolMap = MergeSymbolTable(*(input1->OutputSymbols()), *(input2->InputSymbols()), &relabel);
    Relabel(input1, NULL, symbolMap);
    Relabel(input2, symbolMap, NULL);
    input1->SetOutputSymbols(symbolMap);
    input2->SetInputSymbols(symbolMap);
    delete symbolMap;
    StdVectorFst composed;
    Compose(*input1, *input2, &composed);
    composed.Write("");
    delete input1;
    delete input2;
}
