#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <list>
#include <fst/fstlib.h>

int main(int argc, char** argv) {
    bool is_transducer = false;
    if(argc == 2 && std::string(argv[1]) == "-t") {
        is_transducer = true;
    } else if(argc != 1) {
        std::cerr << "usage: " << argv[0] << " [-t]\n";
        return 1;
    }
    fst::SymbolTable states("states");
    fst::SymbolTable isyms("input");
    fst::SymbolTable osyms("output");
    isyms.AddSymbol("<eps>");
    osyms.AddSymbol("<eps>");
    fst::StdVectorFst automaton;
    int line_num = 0;
    while(!std::cin.eof()) {
        line_num++;
        std::string line;
        std::getline(std::cin, line);
        std::vector<std::string> tokens;
        std::istringstream tokenizer(line);
        while(1) {
            std::string token;
            if(!(tokenizer >> token)) break;
            tokens.push_back(token);
        }
        if(std::cin.eof()) break;
        int64 from_state = -1, to_state = -1, in_symbol = -1, out_symbol = -1;
        double weight = 0;
        if(tokens.size() == 0) {
            std::cerr << "error: empty line in automaton, line " << line_num << "\n";
            return 1;
        } else {
            from_state = states.Find(tokens[0]);
            if(from_state == -1) {
                from_state = automaton.AddState();
                states.AddSymbol(tokens[0], from_state);
            }
            if(tokens.size() <= 2) {
                if(tokens.size() == 2 && !(std::istringstream(tokens[1]) >> weight)) {
                    std::cerr << "error: weight not a valid number, line " << line_num << "\n";
                    return 1;
                }
                automaton.SetFinal(from_state, weight);
            } else {
                to_state = states.Find(tokens[1]);
                if(to_state == -1) {
                    to_state = automaton.AddState();
                    states.AddSymbol(tokens[1], to_state);
                }
                in_symbol = isyms.AddSymbol(tokens[2]);
                if(is_transducer) {
                    if(tokens.size() < 4) {
                        std::cerr << "error: missing output symbol in transducer, line " << line_num << "\n";
                        return 1;
                    }
                    out_symbol = osyms.AddSymbol(tokens[3]);
                    if(tokens.size() == 5 && !(std::istringstream(tokens[4]) >> weight)) {
                        std::cerr << "error: weight not a valid number, line " << line_num << "\n";
                        return 1;
                    }
                    if(tokens.size() > 5) {
                        std::cerr << "error: too many fields in transudcer, line " << line_num << "\n";
                        return 1;
                    }
                } else {
                    out_symbol = in_symbol;
                    if(tokens.size() == 4 && !(std::istringstream(tokens[3]) >> weight)) {
                        std::cerr << "error: weight not a valid number, line " << line_num << "\n";
                        return 1;
                    }
                    if(tokens.size() > 4) {
                        std::cerr << "error: too many fields in acceptor, line " << line_num << "\n";
                        return 1;
                    }
                }
                automaton.AddArc(from_state, fst::StdArc(in_symbol, out_symbol, weight, to_state));
            }
        }
    }
    automaton.SetStart(0);
    automaton.SetInputSymbols(&isyms);
    if(is_transducer) automaton.SetOutputSymbols(&osyms);
    else automaton.SetOutputSymbols(&isyms);
    automaton.Write("");
    return 0;
}
