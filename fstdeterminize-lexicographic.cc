#include <fst/fstlib.h>
#include <tr1/functional>
#include "categorial-weight.h"

using namespace fst;

typedef CategorialWeight<int,CATEGORIAL_LEFT> StdCategorialWeight;
typedef LexicographicWeight<TropicalWeight,StdCategorialWeight> TCLexWeight;
typedef LexicographicArc<TropicalWeight,StdCategorialWeight> TCLexArc;
typedef VectorFst<TCLexArc> TCLexFst;

void convert(const StdVectorFst& input, TCLexFst &output) {
    for(int64 state = 0; state < input.NumStates(); state++) {
        output.AddState();
        if(input.Start() == state) output.SetStart(state);
        if(input.Final(state) != TropicalWeight::Zero())
            output.SetFinal(state, TCLexWeight(input.Final(state), StdCategorialWeight::One()));
        for(ArcIterator<StdVectorFst> aiter(input, state); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            output.AddArc(state, 
                    TCLexArc(arc.ilabel, arc.ilabel, 
                        TCLexWeight(arc.weight, StdCategorialWeight(arc.olabel)), 
                    arc.nextstate));
        }
    }
}

void print(const TCLexFst &input, const SymbolTable *isyms, const SymbolTable *osyms) {
    for(int64 state = 0; state < input.NumStates(); state++) {
        if(input.Final(state) != TCLexWeight::Zero()) cout << state << endl;
        for(ArcIterator<TCLexFst> aiter(input, state); !aiter.Done(); aiter.Next()) {
            const TCLexArc &arc = aiter.Value();
            cout << state << " " << arc.nextstate << " " << isyms->Find(arc.ilabel) << " " << arc.weight.Value2() << " " << arc.weight.Value1() << endl;
        }
    }
}

int main(int argc, char** argv) {
    StdVectorFst *input = StdVectorFst::Read("");
    TCLexFst converted, determinized;
    convert(*input, converted);
    //print(converted, input->InputSymbols(), input->OutputSymbols());
    Determinize(converted, &determinized);
    print(determinized, input->InputSymbols(), input->OutputSymbols());
}

