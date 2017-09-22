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

namespace fst {

    /* <Tropical,Categorial>-Lexicographic determinization: find the best
     * tagging for each path of the input word lattice.
     *
     * Implementation follows the paper "Efficient Determinization of Tagged
     * Word Lattices usingCategorial and Lexicographic Semirings", by I.
     * Shafran et al, ASRU 2011
     *
     * We use string weights as a base class and override plus and divide
     * operator (should not be used with other string weight operations)
     */
    typedef StringWeight<int, STRING_LEFT> CategorialWeight;
    typedef StringWeightIterator<CategorialWeight> CategorialWeightIterator;
    typedef LexicographicWeight<TropicalWeight, CategorialWeight> TCLexWeight;
    typedef LexicographicArc<TropicalWeight, CategorialWeight> TCLexArc;
    typedef VectorFst<TCLexArc> TCLexFst;

    /* define extra negative symbols to encode categorial weight special symbols */
    const int kCategorialLeftBracket = -3;
    const int kCategorialRightBracket = -4;
    const int kCategorialLeftDiv = -5;

    /* override the plus operator
     * w1 + w2 = 
     *      w1 if w1 <L w2 (lexicographic order on strings)
     *      w2 else
     */
    template <> inline CategorialWeight Plus(const CategorialWeight &w1, const CategorialWeight &w2) {
        if (w1 == CategorialWeight::Zero())
            return w2;
        if (w2 == CategorialWeight::Zero())
            return w1;
        CategorialWeightIterator iter1(w1);
        CategorialWeightIterator iter2(w2);
        for (; !iter1.Done() && !iter2.Done(); iter1.Next(), iter2.Next()) {
            if(iter1.Value() < iter2.Value()) return w1;
            else if(iter1.Value() > iter2.Value()) return w2;
        }
        if(!iter2.Done()) return w1;
        return w2;
    }

    /* override the divide operator
     * w1 / w2 = w2 "/" w1 (concatenation with "backslash" symbol)
     * note: 
     *      w1 / 0 = bad
     *      0 / w2 = 0
     *      w1 / w1 = One
     *      do we need: (w1 "/" w2) * w1 = w2 ???
     */
    template <> inline CategorialWeight Divide(const CategorialWeight &w1, const CategorialWeight &w2, DivideType typ) {
        if (typ != DIVIDE_LEFT)
            LOG(FATAL) << "CategorialWeight::Divide: only left division is defined "
                << "for the " << CategorialWeight::Type() << " semiring";

        if (w2 == CategorialWeight::Zero())
            return CategorialWeight(kStringBad);
        else if (w1 == CategorialWeight::Zero())
            return CategorialWeight::Zero();

        if(w1 == w2) return CategorialWeight::One();
        CategorialWeight div;
        CategorialWeightIterator iter1(w1);
        CategorialWeightIterator iter2(w2);
        bool needsBrackets = false;
        for (; !iter2.Done(); iter2.Next()) {
            if(iter2.Value() == kCategorialLeftDiv) {
                needsBrackets = true;
                break;
            }
        }
        iter2.Reset();
        if(needsBrackets) div.PushBack(kCategorialLeftBracket);
        for (; !iter2.Done(); iter2.Next()) {
            div.PushBack(iter2.Value());
        }
        if(needsBrackets) div.PushBack(kCategorialRightBracket);
        div.PushBack(kCategorialLeftDiv);
        for (; !iter1.Done(); iter1.Next()) {
            div.PushBack(iter1.Value());
        }
        return div;
    }

    /* Categorial semiring has the path property
     */
    template <> uint64 CategorialWeight::Properties() {
        return kLeftSemiring | kIdempotent | kPath;
    }

    /* override the lexicographic weight in order to use a different ordering
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
    template <> inline TCLexWeight Plus(const TCLexWeight &w, const TCLexWeight &v) {
        NaturalLess<TropicalWeight> less1;
        if (less1(w.Value1(), v.Value1())) return w;
        if (less1(v.Value1(), w.Value1())) return v;
        CategorialWeightIterator iter1(w.Value2());
        CategorialWeightIterator iter2(v.Value2());
        for (; !iter1.Done() && !iter2.Done(); iter1.Next(), iter2.Next()) {
            if(iter1.Value() < iter2.Value()) return w;
            else if(iter1.Value() > iter2.Value()) return v;
        }
        if(!iter2.Done()) return w;
        return v;
    }

    /* map a standard transducer to the TCLex semiring
    */
    struct ToTCLexMapper {
        typedef StdArc FromArc;
        typedef TCLexArc ToArc;
        TCLexArc operator()(const StdArc &arc) {
            if(arc.weight == TropicalWeight::Zero()) {
                return TCLexArc(arc.ilabel, arc.ilabel, TCLexWeight::Zero(), arc.nextstate);
            }
            return TCLexArc(arc.ilabel, arc.ilabel, TCLexWeight(arc.weight, CategorialWeight(arc.olabel)), arc.nextstate);
        }
        MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }
        MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
        MapSymbolsAction OutputSymbolsAction() const { return MAP_CLEAR_SYMBOLS; }
        uint64 Properties(uint64 props) const { return props; }
    };

    /* map TCLex fst to tropical semiring and generate symbol table
     */
    class FromTCLexMapper {
        SymbolTable &symbols;
        public:
        typedef TCLexArc FromArc;
        typedef StdArc ToArc;
        FromTCLexMapper(SymbolTable& syms) : symbols(syms) {
            symbols.AddSymbol("<eps>", 0);
        }
        StdArc operator()(const TCLexArc &arc) {
            int64 id = 0; // by default, it's a final state, so output label must be 0
            if(arc.nextstate != kNoStateId) { // else
                CategorialWeightIterator iter(arc.weight.Value2()); // serialize categorial weight
                std::ostringstream label;
                bool needSeparator = false;
                for(; !iter.Done(); iter.Next()) {
                    int value = iter.Value();
                    if (value == kCategorialLeftBracket) {
                        label << '<';
                        needSeparator = false;
                    } else if (value == kCategorialRightBracket) {
                        label << '>';
                        needSeparator = false;
                    } else if (value == kCategorialLeftDiv) {
                        label << '\\';
                        needSeparator = false;
                    } else {
                        if(needSeparator) label << "_";
                        label << value;
                        needSeparator = true;
                    }
                }
                id = symbols.AddSymbol(label.str());
            }
            return StdArc(arc.ilabel, id, arc.weight.Value1(), arc.nextstate);
        }
        MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }
        MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
        MapSymbolsAction OutputSymbolsAction() const { return MAP_NOOP_SYMBOLS; }
        uint64 Properties(uint64 props) const { return props; }
    };

    /* Utility methods to tackle serialized TCLex weights
     */
    bool IsSimple(const std::string &input) {
        if(string::npos != input.find_first_of("\\<>")) return false;
        return true;
    }

    void SplitOnUnderscores(const std::string &input, std::vector<std::string>& tokens) {
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

    void SplitOnSlashes(const std::string &input, std::vector<std::string>& tokens) {
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

    /* Generate a transducer that when composed with the determinized automaton will
     * result in the final output (by decomposing the weights)
     */
    void BuildPathDecoder(StdVectorFst& output, const SymbolTable& TCLexSymbols) {
        output.AddState();
        output.SetStart(0);
        output.SetFinal(0, TropicalWeight::One());
        SymbolTableIterator iter(TCLexSymbols);
        for(; !iter.Done(); iter.Next()) {
            if(iter.Value() == 0) continue; // skip epsilon
            std::string symbol = iter.Symbol();
            if(IsSimple(symbol)) {
                int64 label = atoi(symbol.c_str());
                output.AddArc(0, StdArc(iter.Value(), label, 0, 0));
            } else {
                std::vector<std::string> arcsInput;
                std::vector<std::string> arcsOutput;
                SplitOnSlashes(symbol, arcsInput);
                SplitOnUnderscores(arcsInput.back(), arcsOutput);
                arcsInput.pop_back();
                reverse(arcsInput.begin(), arcsInput.end());
                arcsInput.push_back(symbol);
                size_t max = arcsInput.size();
                if(max < arcsOutput.size()) max = arcsOutput.size();
                int fromState = 0, nextState = 0;
                for(size_t i = 0; i < max; i++) {
                    int64 ilabel = 0;
                    int64 olabel = 0;
                    if(i < arcsInput.size()) ilabel = TCLexSymbols.Find(arcsInput[i]);
                    if(i < arcsOutput.size()) olabel = atoi(arcsOutput[i].c_str());
                    if(i == max - 1) {
                        nextState = 0;
                    } else {
                        nextState = output.NumStates();
                        output.AddState();
                    }
                    output.AddArc(fromState, StdArc(ilabel, olabel, 0, nextState));
                    fromState = nextState;
                }
            }
        }
        ArcSort(&output, ILabelCompare<StdArc>()); // ready for composition
    }

}

using namespace fst;

int main(int argc, char** argv) {
    // read transducer from stdin
    StdVectorFst *input = StdVectorFst::Read("");

    // for determinization, we need an epsilon-free fst
    if(input->Properties(kEpsilons, true)) RmEpsilon(input);

    // convert olabel+weights to TCLex weights
    TCLexFst converted;
    ArcMap(*input, &converted, ToTCLexMapper());

    // determinize
    TCLexFst determinized;
    Determinize(converted, &determinized);

    // map from TCLex semiring to tropical with string representation as output
    SymbolTable symbols("tclex");
    FromTCLexMapper mapper(symbols);
    StdVectorFst back_to_syms;
    ArcMap(determinized, &back_to_syms, &mapper);

    // create decoder for string representation of TCLex weights
    StdVectorFst decoder;
    BuildPathDecoder(decoder, symbols);
    
    // compose to generate final automaton 
    StdVectorFst result;
    Compose(back_to_syms, decoder, &result);
    result.SetOutputSymbols(input->OutputSymbols());

    // write result to stdout
    result.Write("");
}

