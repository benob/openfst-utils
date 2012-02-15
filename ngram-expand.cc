#include <fst/fstlib.h>
#include <set>
#include <tr1/unordered_map>
#include <list>

using namespace fst;
using namespace std;

typedef int64 State;

struct Clique {
    State inputState;
    State outputState;
    list<StdArc> seq;
    Clique() {}
    Clique(State _inputState, State _outputState) : inputState(_inputState), outputState(_outputState) {}
    Clique& operator=(const Clique& other) {
        inputState = other.inputState;
        seq = other.seq;
        return *this;
    }
    void print() {
        cerr << "state: " << inputState << " labels:";
        for(list<StdArc>::const_iterator i = seq.begin(); i != seq.end(); i++) {
            cerr << " " << i->ilabel;
        }
        cerr << endl;
    }
    void push(const StdArc& arc, int maxsize) {
        if(seq.size() > 0) {
            inputState = seq.back().nextstate;
        }
        seq.push_back(arc);
        if((int) seq.size() > maxsize) seq.pop_front();
    }
    State next() {
        return seq.back().nextstate;
    }
    struct LabelsHash {
        size_t operator()(const Clique& a) const {
            size_t output = a.inputState;
            for(list<StdArc>::const_iterator i = a.seq.begin(); i != a.seq.end(); i++) output ^= i->ilabel ^ i->olabel;
            return output;
        }
    };
    struct LabelsEqual {
        int operator()(const Clique& a, const Clique& b) const {
            if(a.inputState != b.inputState) return false;
            if(a.seq.size() != b.seq.size()) return false;
            list<StdArc>::const_iterator i = a.seq.begin();
            list<StdArc>::const_iterator j = b.seq.begin();
            for(; i != a.seq.end() && j != b.seq.end(); i++, j++) {
                if(i->ilabel != j->ilabel) return false;
                if(i->olabel != j->olabel) return false;
            }
            return true;
        }
    };
    struct Compare {
        bool operator()(const Clique& a, const Clique& b) {
            if(a.inputState != b.inputState) return a.inputState < b.inputState;
            if(a.seq.size() != b.seq.size()) return a.seq.size() < b.seq.size();
            list<StdArc>::const_iterator i = a.seq.begin();
            list<StdArc>::const_iterator j = b.seq.begin();
            for(; i != a.seq.end() && j != b.seq.end(); i++, j++) {
                if(i->ilabel < j->ilabel) return true;
                if(i->olabel < j->olabel) return true;
                //if(i->weight < j->weight) return true;
                if(i->nextstate < j->nextstate) return true;
            }
            return false;
        }
    };
    struct CompareLabels {
        bool operator()(const Clique& a, const Clique& b) {
            if(a.inputState != b.inputState) return a.inputState < b.inputState;
            if(a.seq.size() != b.seq.size()) return a.seq.size() < b.seq.size();
            list<StdArc>::const_iterator i = a.seq.begin();
            list<StdArc>::const_iterator j = b.seq.begin();
            for(; i != a.seq.end() && j != b.seq.end(); i++, j++) {
                if(i->ilabel < j->ilabel) return true;
                if(i->olabel < j->olabel) return true;
            }
            return false;
        }
    };
};

int main(int argc, char** argv) {

    int ngram_size = 2;
    if(argc >= 2) ngram_size = atoi(argv[1]);

    StdVectorFst *input = StdVectorFst::Read("");
    if(ngram_size < 2) { // nothing to do
        input->Write("");
        return 0;
    }
    StdVectorFst output;

    unordered_map<Clique, State, Clique::LabelsHash, Clique::LabelsEqual> ngrams;
    set<Clique, Clique::Compare> visited;
    list<Clique> queue;

    State startState = 0;
    output.AddState();
    output.SetStart(startState);
    for(ArcIterator<StdVectorFst> aiter(*input, input->Start()); !aiter.Done(); aiter.Next()) {
        const StdArc &arc = aiter.Value();
        State nextState = output.NumStates();
        Clique arcs(input->Start(), nextState);
        arcs.push(arc, ngram_size);
        queue.push_back(arcs);
        ngrams[arcs] = nextState;
        output.AddState();
        output.AddArc(startState, StdArc(arc.ilabel, arc.olabel, arc.weight, nextState));
    }

    while(queue.size() > 0) {
        Clique current = queue.front();
        //current.print();
        queue.pop_front();
        State inputState = current.next();
        State arcStartState = current.outputState;
        for(ArcIterator<StdVectorFst> aiter(*input, inputState); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            Clique next = current;
            //next.print();
            next.push(arc, ngram_size);
            ////////////////////////////////////////////////////
            unordered_map<Clique, State>::iterator found = ngrams.find(next);
            int arcEndState = output.NumStates();
            if(found == ngrams.end()) {
                ngrams[next] = arcEndState;
                output.AddState();
            } else {
                arcEndState = found->second;
            }
            next.outputState = arcEndState;
            if(input->Final(arc.nextstate) != arc.weight.Zero()) {
                output.SetFinal(arcEndState, input->Final(arc.nextstate));
            }
            output.AddArc(arcStartState, StdArc(arc.ilabel, arc.olabel, arc.weight, arcEndState));
            ////////////////////////////////////////////////////
            if(visited.insert(next).second == true) {
                queue.push_back(next);
            }
        }
    }
    output.SetInputSymbols(input->InputSymbols());
    output.SetOutputSymbols(input->OutputSymbols());
    output.Write("");
    delete input;
}
