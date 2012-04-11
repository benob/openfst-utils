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

int main(int argc, char** argv) {
    EncodeMapper<StdArc> mapper(kEncodeLabels, ENCODE);
    StdVectorFst *ifst = StdVectorFst::Read("");
    RmEpsilon(ifst);
    Encode(ifst, &mapper);
    Determinize(*ifst, ifst);
    Minimize(ifst);
    Decode(ifst, mapper);
    ifst->Write("");
}
