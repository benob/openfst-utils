#include <iostream>
#include <vector>
#include <fst/fstlib.h>

int main(int argc, char** argv) {
    fst::StdVectorFst* old = fst::StdVectorFst::Read("");
    fst::VectorFst<fst::LogArc> input;
    fst::Map(*old, &input, fst::StdToLogMapper());
    int numStates = input.NumStates();

    std::vector<fst::LogArc::Weight> alpha(numStates, 0);
    std::vector<fst::LogArc::Weight> beta(numStates, 0);

    fst::ShortestDistance<fst::LogArc>(input, &alpha, false);
    fst::ShortestDistance<fst::LogArc>(input, &beta, true);

    for(int64 state = 0; state < numStates; state++) {
        for(fst::MutableArcIterator<fst::VectorFst<fst::LogArc> > aiter(&input, state); !aiter.Done(); aiter.Next()) {
            const fst::LogArc &arc = aiter.Value();
            double posterior = exp(-(alpha[state].Value() + arc.weight.Value() + beta[arc.nextstate].Value() - beta[input.Start()].Value()));
            aiter.SetValue(fst::LogArc(arc.ilabel, arc.olabel, posterior, arc.nextstate));
        }
    }
    fst::Map(input, old, fst::LogToStdMapper());
    old->Write("");
    delete old;
}
