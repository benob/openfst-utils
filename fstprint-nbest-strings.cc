#include <fst/fstlib.h>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "usage: cat <fst> | " << argv[0] << " <n>\n";
        return 1;
    }
    int n;
    std::stringstream reader(argv[1]);
    reader >> n;
    if(n < 1) {
        std::cerr << "error: invalid n = " << n << "\n";
        return 2;
    }
    fst::StdVectorFst* input = fst::StdVectorFst::Read("");
    fst::StdVectorFst result;
    fst::ShortestPath(*input, &result, n);
    fst::Push(&result, fst::REWEIGHT_TO_INITIAL);
    const fst::SymbolTable* outputSymbols = result.OutputSymbols();
    const fst::SymbolTable* inputSymbols = result.InputSymbols();

    for(fst::ArcIterator<fst::StdVectorFst> aiter(result, result.Start()); !aiter.Done(); aiter.Next()) {
        const fst::StdArc& path = aiter.Value();
        int64 state = path.nextstate;
        std::cout << path.weight;
        if(path.ilabel == path.olabel) {
            if(path.olabel != 0) {
                if(outputSymbols) std::cout << " " << outputSymbols->Find(path.olabel);
                else std::cout << " " << path.olabel;
            }
        } else {
            if(path.olabel != 0) {
                if(inputSymbols) std::cout << " " << inputSymbols->Find(path.ilabel);
                else std::cout << " " << path.ilabel;
                if(outputSymbols) std::cout << "/" << outputSymbols->Find(path.olabel);
                else std::cout << "/" << path.olabel;
            }
        }
        while(result.Final(state) == fst::StdArc::Weight::Zero()) {
            fst::ArcIterator<fst::StdVectorFst> nextIter(result, state);
            if(nextIter.Done()) {
                break;
                // this should not happen
            }
            const fst::StdArc arc = nextIter.Value();
            if(arc.ilabel == arc.olabel) {
                if(arc.olabel != 0) {
                    if(outputSymbols) std::cout << " " << outputSymbols->Find(arc.olabel);
                    else std::cout << " " << arc.olabel;
                }
            } else {
                if(arc.olabel != 0) {
                    if(inputSymbols) std::cout << " " << inputSymbols->Find(arc.ilabel);
                    else std::cout << " " << arc.ilabel;
                    if(outputSymbols) std::cout << "/" << outputSymbols->Find(arc.olabel);
                    else std::cout << "/" << arc.olabel;
                }
            }
            state = arc.nextstate;
        }
        std::cout << "\n";
    }


}
