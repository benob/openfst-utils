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

using namespace fst;

/* make sure that all final states have no outgoing arcs, AND that there are no epsilon transitions
 *     0) remove epsilon transitions
 *     1) add super final state
 *     2) for each arc going to a final state, duplicate it to the super final state
 *        it's weight is the final state weight times the arc weight
 *     3) make all final states non final except for the super final state
 *     4) trim old final states that had no outgoing arcs
 */

int main(int argc, char** argv) {
    StdVectorFst* input = StdVectorFst::Read("");
    RmEpsilon(input);
    input->AddState(); // super final state
    for(StateIterator<StdVectorFst> siter(*input); !siter.Done(); siter.Next()) {
        StdArc::StateId state = siter.Value();
        std::vector<StdArc> arcs;
        for(ArcIterator<StdVectorFst> aiter(*input, state); !aiter.Done(); aiter.Next()) {
            const StdArc& arc = aiter.Value();
            if(input->Final(arc.nextstate) != TropicalWeight::Zero()) {
                arcs.push_back(StdArc(arc.ilabel, arc.olabel, Times(arc.weight, input->Final(arc.nextstate)), input->NumStates() - 1));
            }
        }
        for(std::vector<StdArc>::iterator i = arcs.begin(); i != arcs.end(); i++) {
            input->AddArc(state, *i);
        }
    }
    for(StateIterator<StdVectorFst> siter(*input); !siter.Done(); siter.Next()) {
        StdArc::StateId state = siter.Value();
        if(input->Final(state) != TropicalWeight::Zero()) {
            input->SetFinal(state, TropicalWeight::Zero());
        }
    }
    input->SetFinal(input->NumStates() - 1, TropicalWeight::One());
    Connect(input);
    input->Write("");
}
