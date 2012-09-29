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

// inspired by http://code.google.com/p/pyopenfst/source/browse/opfst_beamsearch.cc

using namespace std;

template <class M>
class SpecialMatcher: public fst::RhoMatcher< fst::SigmaMatcher< fst::PhiMatcher< M > > >
{
    public:
        typedef typename M::FST FST;
        typedef typename M::Arc Arc;
        typedef typename Arc::StateId StateId;
        typedef typename Arc::Label Label;
        typedef typename Arc::Weight Weight;

        SpecialMatcher(const SpecialMatcher<M> &matcher, bool safe = false)
            : fst::RhoMatcher< fst::SigmaMatcher< fst::PhiMatcher< M > > >(matcher, safe)
        {}

        virtual SpecialMatcher<M> *Copy(bool safe = false) const {
            return new SpecialMatcher<M>(*this, safe);
        }

        virtual ~SpecialMatcher() {
            delete matcher_;
        }

        SpecialMatcher(const FST &fst,
                fst::MatchType match_type, int64 rho = -3, int64 sigma = -2, int64 phi = -1)
            :           fst::RhoMatcher< fst::SigmaMatcher< fst::PhiMatcher< M > > >
                        (fst, match_type, rho, fst::MATCHER_REWRITE_ALWAYS,
                         new fst::SigmaMatcher< fst::PhiMatcher< M> >
                         (fst, match_type, sigma, fst::MATCHER_REWRITE_ALWAYS,
                          new fst::PhiMatcher< M >
                          (fst, match_type, phi, fst::MATCHER_REWRITE_ALWAYS)))
    {}


        M *matcher_;

};


int main(int argc, char** argv) {
    if(argc != 3) {
        cerr << "usage: " << argv[0] << " <input1> <input2>\n";
        return 1;
    }
    fst::StdVectorFst* input1 = fst::StdVectorFst::Read(argv[1]);
    fst::StdVectorFst* input2 = fst::StdVectorFst::Read(argv[2]);

    bool relabel;
    fst::SymbolTable *symbolMap = fst::MergeSymbolTable(*(input1->OutputSymbols()), *(input2->InputSymbols()), &relabel);

    int64 rho = symbolMap->Find("<rho>");
    int64 sigma = symbolMap->Find("<sigma>");
    int64 phi = symbolMap->Find("<phi>");

    fst::Relabel(input1, NULL, symbolMap);
    fst::Relabel(input2, symbolMap, NULL);

    input1->SetOutputSymbols(symbolMap);
    input2->SetInputSymbols(symbolMap);

    fst::ArcSort(input1, fst::StdOLabelCompare());
    fst::ArcSort(input2, fst::StdILabelCompare());

    typedef SpecialMatcher< fst::SortedMatcher<fst::StdFst> > StdSpecialMatcher;

    fst::ComposeFstOptions <fst::StdArc, StdSpecialMatcher> opts;

    opts.matcher1 = new StdSpecialMatcher(*input1, fst::MATCH_OUTPUT, rho, sigma, phi);
    opts.matcher2 = new StdSpecialMatcher(*input2, fst::MATCH_INPUT, rho, sigma, phi);

    fst::StdComposeFst composed(*input1, *input2, opts);

    fst::StdVectorFst output(composed);
    fst::Connect(&output);
    output.Write("");

    delete symbolMap;
}
