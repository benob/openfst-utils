#include <fst/fstlib.h>

using namespace fst;

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cerr << "usage: " << argv[0] << " <fst1> <fst2>\n";
        return 1;
    }
    StdVectorFst *input1 = StdVectorFst::Read(argv[1]);
    StdVectorFst *input2 = StdVectorFst::Read(argv[2]);

    // step 1: relabel symbols so that they match
    bool relabel;
    SymbolTable *symbolMap = MergeSymbolTable(*(input1->OutputSymbols()), *(input2->InputSymbols()), &relabel);
    Relabel(input1, NULL, symbolMap);
    Relabel(input2, symbolMap, NULL);
    
    // step 2: create half-edit fsts
    int64 subSym = symbolMap->AddSymbol("<sub>");
    int64 delSym = symbolMap->AddSymbol("<del>");
    int64 insSym = symbolMap->AddSymbol("<ins>");
    input1->SetOutputSymbols(symbolMap);
    input2->SetInputSymbols(symbolMap);
    StdVectorFst halfEdit1;
    StdVectorFst halfEdit2;
    halfEdit1.SetInputSymbols(symbolMap);
    halfEdit1.SetOutputSymbols(symbolMap);
    halfEdit1.AddState();
    halfEdit1.SetStart(0);
    halfEdit1.SetFinal(0, 0);
    halfEdit1.AddArc(0, StdArc(0, insSym, 0.5, 0));
    halfEdit2.SetInputSymbols(symbolMap);
    halfEdit2.SetOutputSymbols(symbolMap);
    halfEdit2.AddState();
    halfEdit2.SetStart(0);
    halfEdit2.SetFinal(0, 0);
    halfEdit2.AddArc(0, StdArc(delSym, 0, 0.5, 0));
    for(SymbolTableIterator siter(*symbolMap); !siter.Done(); siter.Next()) {
        int64 symbol = siter.Value();
        if(symbol != 0 && symbol != insSym && symbol != delSym && symbol != subSym) {
            halfEdit1.AddArc(0, StdArc(symbol, symbol, 0, 0));
            halfEdit1.AddArc(0, StdArc(symbol, subSym, 0.5, 0));
            halfEdit1.AddArc(0, StdArc(symbol, delSym, 0.5, 0));
            halfEdit2.AddArc(0, StdArc(symbol, symbol, 0, 0));
            halfEdit2.AddArc(0, StdArc(subSym, symbol, 0.5, 0));
            halfEdit2.AddArc(0, StdArc(insSym, symbol, 0.5, 0));
        }
    }

    // step 3: compose
    StdVectorFst halfCompose1;
    Compose(*input1, halfEdit1, &halfCompose1);
    ArcSort(&halfCompose1, StdOLabelCompare());
    StdVectorFst halfCompose2;
    Compose(halfEdit2, *input2, &halfCompose2);
    ArcSort(&halfCompose2, StdILabelCompare());
    StdVectorFst oracle;
    Compose(halfCompose1, halfCompose2, &oracle);
    oracle.Write("");

    delete symbolMap;
    delete input1;
    delete input2;
}
