// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2012 Aix-Marseille Univ.
// Author: benoit.favre@lif.univ-mrs.fr (Benoit Favre)

#include <fst/fstlib.h>

using namespace std;
using namespace fst;

typedef PhiMatcher< SortedMatcher<StdFst> > PM;

int main(int argc, char** argv) {
    if(argc != 3) {
        cerr << "usage: cat <input1> | " << argv[0] << " <phi-symbol> <input2>\n";
        return 1;
    }
    StdVectorFst* input1 = StdVectorFst::Read(argv[2]);
    StdVectorFst* input2 = StdVectorFst::Read("");
    bool relabel;
    SymbolTable *symbolMap = MergeSymbolTable(*(input1->OutputSymbols()), *(input2->InputSymbols()), &relabel);
    Relabel(input1, NULL, symbolMap);
    Relabel(input2, symbolMap, NULL);
    input1->SetOutputSymbols(symbolMap);
    ArcSort(input1, StdOLabelCompare());
    input2->SetInputSymbols(symbolMap);
    int64 phiLabel = symbolMap->AddSymbol(argv[1]);
    std::cerr << "phi = " << phiLabel << " " << symbolMap->Find(phiLabel) << "\n";
    ArcSort(input1, OLabelCompare<StdArc>()); 
    ComposeFstOptions<StdArc, PM> opts;
    opts.gc_limit = 0;
    opts.matcher1 = new PM(*input1, MATCH_OUTPUT, phiLabel);
    opts.matcher2 = new PM(*input2, MATCH_NONE, kNoLabel);
    // composition
    ComposeFst<StdArc> composed(*input1, *input2, opts);
    StdVectorFst output(composed);
    Connect(&output);
    output.Write("");
    delete symbolMap;
}
