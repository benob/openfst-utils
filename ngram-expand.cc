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
#include <tr1/unordered_map>
#include <list>

using namespace fst;
using namespace std;

typedef int64 State;

struct Context {
    State inputState;
    State outputState;
    list<StdArc> seq;
    size_t length;
    Context() {}
    Context(State _inputState, State _outputState, size_t _length) : inputState(_inputState), outputState(_outputState), length(_length) {}
    Context& operator=(const Context& other) {
        inputState = other.inputState;
        outputState = other.outputState;
        length = other.length;
        seq = other.seq;
        return *this;
    }
    void print() {
        cerr << "state: in=" << inputState << " out=" << outputState << " labels:";
        for(list<StdArc>::const_iterator i = seq.begin(); i != seq.end(); i++) {
            cerr << " " << i->ilabel;
        }
        cerr << endl;
    }
    void push(const StdArc& arc) {
        seq.push_back(arc);
        inputState = arc.nextstate;
        if(seq.size() > length) seq.pop_front();
    }
    struct LabelsHash {
        size_t operator()(const Context& a) const {
            size_t output = a.inputState;
            list<StdArc>::const_iterator i = a.seq.begin(); 
            if(a.seq.size() == a.length) i++;
            for(; i != a.seq.end(); i++) output ^= i->ilabel ^ i->olabel;
            return output;
        }
    };
    struct LabelsEqual {
        int operator()(const Context& a, const Context& b) const {
            if(a.inputState != b.inputState) return false;
            if(a.seq.size() != b.seq.size()) return false;
            list<StdArc>::const_iterator i = a.seq.begin();
            list<StdArc>::const_iterator j = b.seq.begin();
            if(a.seq.size() == a.length) i++;
            if(b.seq.size() == b.length) j++;
            for(; i != a.seq.end() && j != b.seq.end(); i++, j++) {
                if(i->ilabel != j->ilabel) return false;
                if(i->olabel != j->olabel) return false;
            }
            return true;
        }
    };
    StdArc::Weight weight() const {
        return seq.back().weight;
    }
    int64 ilabel() const {
        return seq.back().ilabel;
    }
    int64 olabel() const {
        return seq.back().ilabel;
    }
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

    unordered_map<Context, State, Context::LabelsHash, Context::LabelsEqual> outputStates;
    list<Context> queue; // queue all unprocessed contexts

    State outputStart = 0;
    output.AddState();
    output.SetStart(outputStart);
    queue.push_back(Context(input->Start(), outputStart, ngram_size));

    while(queue.size() > 0) {
        Context current = queue.front();
        queue.pop_front();
        State inputState = current.inputState;
        State arcStartState = current.outputState;
        for(ArcIterator<StdVectorFst> aiter(*input, inputState); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            Context next = current;
            next.push(arc);
            unordered_map<Context, State>::iterator found = outputStates.find(next);
            int arcEndState = output.NumStates();
            if(found == outputStates.end()) {
                outputStates[next] = arcEndState;
                output.AddState();
                next.outputState = arcEndState;
                queue.push_back(next);
            } else {
                arcEndState = found->second;
            }
            if(input->Final(arc.nextstate) != arc.weight.Zero()) {
                output.SetFinal(arcEndState, input->Final(arc.nextstate));
            }
            output.AddArc(arcStartState, StdArc(next.ilabel(), next.olabel(), next.weight(), arcEndState));
        }
    }
    output.SetInputSymbols(input->InputSymbols());
    output.SetOutputSymbols(input->OutputSymbols());
    output.Write("");
    delete input;
}
