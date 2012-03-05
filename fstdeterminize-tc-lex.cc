#include <fst/fstlib.h>
#include <tr1/functional>
#include "categorial-weight.h"

using namespace fst;

typedef CategorialWeight<int,CATEGORIAL_LEFT> StdCategorialWeight;
typedef LexicographicWeight<TropicalWeight,StdCategorialWeight> TCLexWeight;
typedef LexicographicArc<TropicalWeight,StdCategorialWeight> TCLexArc;
typedef VectorFst<TCLexArc> TCLexFst;

/* we need to override the lexicographic weight in order to use a different ordering
 * <w1,w2> + <w3,w4> =
 *     <w1,w2> if w1 < w3 else
 *     <w3,w4> if w1 > w3 else
 *     <w1,w2> if w2 <L w4 else (where <L is the lexicographic order over tag strings)
 *     <w3,w4>
 * here:
 * w + v =
 *     w if w.value1 < v.value1 else
 *     v if w.value1 > v.value1 else
 *     w if w.value2 <L v.value2 else
 *     v
 */
namespace fst {
    template <>
        inline LexicographicWeight<TropicalWeight,StdCategorialWeight> Plus(const LexicographicWeight<TropicalWeight,StdCategorialWeight> &w,
                const LexicographicWeight<TropicalWeight,StdCategorialWeight> &v) {
            NaturalLess<TropicalWeight> less1;
            if (less1(w.Value1(), v.Value1())) return w;
            if (less1(v.Value1(), w.Value1())) return v;
            /*if(w.Value2() < v.Value2()) return w;
              return v;*/
            CategorialWeightIterator<int,CATEGORIAL_LEFT> iter1(w.Value2());
            CategorialWeightIterator<int,CATEGORIAL_LEFT> iter2(v.Value2());
            for (; !iter1.Done() && !iter2.Done(); iter1.Next(), iter2.Next()) {
                if(iter1.Value() < iter2.Value()) return w;
                else if(iter1.Value() > iter2.Value()) return v;
            }
            if(!iter2.Done()) return w;
            return v;
        }
}

void convertToTCLexSemiring(const StdVectorFst& input, TCLexFst &output) {
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

void convertToTCLexSymbols(const TCLexFst &input, StdVectorFst& output, SymbolTable& symbols) {
    symbols.AddSymbol("<eps>");
    for(int64 state = 0; state < input.NumStates(); state++) {
        output.AddState();
        if(input.Start() == state) output.SetStart(state);
        if(input.Final(state) != TCLexWeight::Zero())
            output.SetFinal(state, input.Final(state).Value1()); // use tropical weight
        for(ArcIterator<TCLexFst> aiter(input, state); !aiter.Done(); aiter.Next()) {
            const TCLexArc &arc = aiter.Value();
            std::ostringstream label;
            label << arc.weight.Value2();
            int64 id = symbols.AddSymbol(label.str());
            output.AddArc(state, StdArc(arc.ilabel, id, arc.weight.Value1(), arc.nextstate));
        }
    }
}

bool isSimple(const std::string &input) {
    if(string::npos != input.find_first_of("\\<>")) return false;
    return true;
}

void splitOnUnderscores(const std::string &input, std::vector<std::string>& tokens) {
    size_t start = 0;
    size_t end = 0;
    while(end < input.length()) {
        start = end;
        while(end < input.length() && input[end] != '_') {
            end++;
        }
        tokens.push_back(input.substr(start, end - start));
        end++;
    }
}

void splitOnSlashes(const std::string &input, std::vector<std::string>& tokens) {
    size_t start = 0;
    size_t end = 0;
    while(end < input.length()) {
        start = end;
        if(input[end] == '<') {
            int num = 1;
            while(num > 0 && end < input.length() - 1) {
                end++;
                if(input[end] == '>') num--;
                else if(input[end] == '<') num++;
            }
        }
        while(end < input.length() && input[end] != '\\') {
            end++;
        }
        if(input[start] == '<')
            tokens.push_back(input.substr(start + 1, end - start - 2));
        else
            tokens.push_back(input.substr(start, end - start));
        end++;
    }
}

void buildMapper(StdVectorFst& output, SymbolTable& TCLexSymbols) {
    output.AddState();
    output.SetStart(0);
    output.SetFinal(0, TropicalWeight::One());
    SymbolTableIterator iter(TCLexSymbols);
    for(; !iter.Done(); iter.Next()) {
        if(iter.Value() == 0) continue; // skip epsilon
        std::string symbol = iter.Symbol();
        //cerr << "SYM: " << symbol << endl;
        if(isSimple(symbol)) {
            int64 label = atoi(symbol.c_str()); //tags->Find(symbol);
            output.AddArc(0, StdArc(iter.Value(), label, 0, 0));
        } else {
            std::vector<std::string> arcsInput;
            std::vector<std::string> arcsOutput;
            splitOnSlashes(symbol, arcsInput);
            splitOnUnderscores(arcsInput.back(), arcsOutput);
            arcsInput.pop_back();
            reverse(arcsInput.begin(), arcsInput.end());
            arcsInput.push_back(symbol);
            /*cerr << symbol << " ----> ";
            for(vector<string>::const_iterator i = arcsInput.begin(); i != arcsInput.end(); i++) {
                cerr << *i << " , ";
            }
            cerr << ":::: ";
            for(vector<string>::const_iterator i = arcsOutput.begin(); i != arcsOutput.end(); i++) {
                cerr << *i << " , ";
            }
            cerr << endl;*/
            size_t max = arcsInput.size();
            if(max < arcsOutput.size()) max = arcsOutput.size();
            int fromState = 0, nextState = 0;
            for(size_t i = 0; i < max; i++) {
                int64 ilabel = 0;
                int64 olabel = 0;
                if(i < arcsInput.size()) ilabel = TCLexSymbols.Find(arcsInput[i]);
                if(i < arcsOutput.size()) olabel = atoi(arcsOutput[i].c_str()); //tags->Find(arcsOutput[i]);
                if(i == max - 1) {
                    nextState = 0;
                } else {
                    nextState = output.NumStates();
                    output.AddState();
                }
                //cerr << "ARC: " << fromState << " " << nextState << " " << ilabel << " " << olabel << " " << symbol << endl;
                output.AddArc(fromState, StdArc(ilabel, olabel, 0, nextState));
                fromState = nextState;
            }
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
    StdVectorFst back_to_syms, mapper;
    cerr << "Remove epsilons\n";
    if(input->Properties(kEpsilons, true)) RmEpsilon(input);
    cerr << "Convert to <T,C>-lexicographic semiring\n";
    convertToTCLexSemiring(*input, converted);
    //print(converted, input->InputSymbols(), input->OutputSymbols());
    cerr << "Determinize\n";
    Determinize(converted, &determinized);
    SymbolTable symbols("tclex");
    //symbols.AddTable(*(input->OutputSymbols()));
    cerr << "Get symbol table\n";
    convertToTCLexSymbols(determinized, back_to_syms, symbols);
    cerr << "Build mapper\n";
    buildMapper(mapper, symbols);
    back_to_syms.SetInputSymbols(input->InputSymbols());
    back_to_syms.SetOutputSymbols(&symbols);
    //back_to_syms.Write("");
    mapper.SetInputSymbols(&symbols);
    mapper.SetOutputSymbols(input->OutputSymbols());
    //mapper.Write("");
    //StdComposeFst result(back_to_syms, mapper);
    StdVectorFst result;
    ArcSort(&mapper, ILabelCompare<StdArc>());
    cerr << "Compose with mapper\n";
    Compose(back_to_syms, mapper, &result);
    result.Write("");
    /*back_to_syms.SetInputSymbols(input->InputSymbols());
    back_to_syms.SetOutputSymbols(&symbols);
    back_to_syms.Write("");*/
    //print(determinized, input->InputSymbols(), input->OutputSymbols());
}

