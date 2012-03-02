#include <fst/fstlib.h>
#include <tr1/functional>
#include "categorial-weight.h"

using namespace fst;

/*namespace fst {

    const int kCategorialNodeEpsilon = 0;
    const int kCategorialNodeInfinity = -1;
    const int kCategorialNodeError = -2;
    const int kCategorialNodeTimes = -3;
    const int kCategorialNodeDivide = -4;

    class CategorialNode {
        int label;
        const CategorialNode *left;
        const CategorialNode *right;

        public:

        CategorialNode() : label(kCategorialNodeEpsilon), left(NULL), right(NULL) {}
        CategorialNode(int _label) : label(_label), left(NULL), right(NULL) {}
        CategorialNode(int _label, const CategorialNode* _left, const CategorialNode* _right) : label(_label), left(_left), right(_right) {}

        typedef CategorialNode ReverseWeight;
        size_t Size() const {
            if(label == kCategorialNodeEpsilon) return 0;
            size_t output = 1;
            if(left != NULL) output += left->Size();
            if(right != NULL) output += right->Size();
            return output;
        }
        static CategorialNode One() {
            return CategorialNode(kCategorialNodeEpsilon);
        }
        static CategorialNode Zero() {
            return CategorialNode(kCategorialNodeInfinity);
        }
        static const std::string& Type() {
            static const std::string type = "categorial";
            return type;
        }
        std::list<int> Value() const {
            std::list<int> output;
            crawl(output);
            return output;
        }
        void crawl(std::list<int> &values) const {
            if(left != NULL) left->crawl(values);
            if(label != kCategorialNodeEpsilon) values.push_back(label);
            if(right != NULL) right->crawl(values);
        }
        CategorialNode& operator=(const CategorialNode& other) {
            label = other.label;
            left = other.left;
            right = other.right;
            return *this;
        }
        bool Member() const {
            return label == kCategorialNodeError;
        }
        size_t Hash() const {
            size_t output = label;
            if(left != NULL) output = (output << 1) ^ left->Hash();
            if(right != NULL) output = (output << 1) ^ right->Hash();
            return output;
        }

        CategorialNode Quantize(float delta) const {
            return *this;
        }

        CategorialNode Reverse() const {
            return *this;
        }

        static uint64 Properties() {
            return kLeftSemiring | kRightSemiring | kCommutative | kPath | kIdempotent;
        }

        ostream& Write(ostream& strm) const {
            cerr << "ERROR: Write() called" << endl;
            //operator<<(strm, *this);
            return strm;
        }

        ostream& Read(ostream& strm) const {
            cerr << "ERROR: Read() called" << endl;
            //strm >> *this;
            return strm;
        }
    };

    bool operator==(const CategorialNode& first, const CategorialNode& second) {
        std::list<int> first_labels;
        std::list<int> second_labels;
        if(first.Size() != second.Size()) return false;
        first.crawl(first_labels);
        second.crawl(second_labels);
        std::list<int>::iterator iter_first = first_labels.begin();
        std::list<int>::iterator iter_second = second_labels.begin();
        while(iter_first != first_labels.end()) {
            if(*iter_first != *iter_second) return false;
            iter_first++;
            iter_second++;
        }
        return true;
    }

    bool operator!=(const CategorialNode& first, const CategorialNode& second) {
        return !(first == second);
    }

    // lexicographic order
    bool operator<(const CategorialNode& first, const CategorialNode& second) {
        std::list<int> first_labels;
        std::list<int> second_labels;
        first.crawl(first_labels);
        second.crawl(second_labels);
        std::list<int>::iterator iter_first = first_labels.begin();
        std::list<int>::iterator iter_second = second_labels.begin();
        while(iter_first != first_labels.end() && iter_second != second_labels.end()) {
            if(*iter_first < *iter_second) return true;
            else if(*iter_first > *iter_second) return false;
            iter_first++;
            iter_second++;
        }
        if(iter_first != first_labels.end()) return false;
        return true;
    }
    bool ApproxEqual(const CategorialNode& first, const CategorialNode& second, float delta) {
        return first == second;
    }

    inline CategorialNode Plus(const CategorialNode& first, const CategorialNode& second) {
        if(first < second) return first;
        return second;
    }

    inline CategorialNode Times(const CategorialNode& first, const CategorialNode& second) {
        if(first == CategorialNode::Zero() || second == CategorialNode::Zero()) return CategorialNode::Zero();
        return CategorialNode(kCategorialNodeTimes, &first, &second);
    }

    inline CategorialNode Divide(const CategorialNode& first, const CategorialNode& second, DivideType typ = DIVIDE_ANY) {
        if(first == second) return CategorialNode::One();
        if(second == CategorialNode::Zero()) return CategorialNode(kCategorialNodeError);
        if(first == CategorialNode::Zero()) return CategorialNode::Zero();
        return CategorialNode(kCategorialNodeDivide, &second, &first);
    }
    inline ostream &operator<<(ostream &strm, const CategorialNode &weight) {
        std::list<int> labels = weight.Value();
        for(std::list<int>::const_iterator i = labels.begin(); i != labels.end(); i++) {
            if(i != labels.begin()) strm << ",";
            if(*i == kCategorialNodeDivide) strm << "\\";
            else if(*i == kCategorialNodeTimes) strm << "_";
            else strm << *i;
        }
        return strm;
    }

}*/

typedef CategorialWeight<int,CATEGORIAL_LEFT> StdCategorialWeight;
//typedef CategorialNode StdCategorialWeight;
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
    convertToTCLexSemiring(*input, converted);
    //print(converted, input->InputSymbols(), input->OutputSymbols());
    Determinize(converted, &determinized);
    SymbolTable symbols("tclex");
    //symbols.AddTable(*(input->OutputSymbols()));
    convertToTCLexSymbols(determinized, back_to_syms, symbols);
    buildMapper(mapper, symbols);
    back_to_syms.SetInputSymbols(input->InputSymbols());
    back_to_syms.SetOutputSymbols(&symbols);
    mapper.SetInputSymbols(&symbols);
    mapper.SetOutputSymbols(input->OutputSymbols());
    //mapper.Write("");
    //StdComposeFst result(back_to_syms, mapper);
    StdVectorFst result;
    ArcSort(&mapper, ILabelCompare<StdArc>());
    Compose(back_to_syms, mapper, &result);
    result.Write("");
    /*back_to_syms.SetInputSymbols(input->InputSymbols());
    back_to_syms.SetOutputSymbols(&symbols);
    back_to_syms.Write("");*/
    //print(determinized, input->InputSymbols(), input->OutputSymbols());
}

