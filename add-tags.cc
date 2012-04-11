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

#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <fst/fstlib.h>

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "usage: " << argv[0] << " <dict>\n";
        return 1;
    }
    std::map<std::string, int> words;
    std::map<std::string, int> tags;
    std::vector<std::string> reverse_tags;
    tags["np"] = 1;
    reverse_tags.push_back("np");
    std::vector<std::vector<int> > tags_for_word;
    std::ifstream input(argv[1]);
    std::string line;
    while(!input.eof()) {
        std::getline(input, line);
        std::istringstream tokenizer(line);
        std::string word, tag;
        int length;
        //tokenizer >> length;
        tokenizer >> word;
        std::vector<int> word_tags;
        while(tokenizer >> tag) {
            //std::cerr << word << " " << tag << "\n";
            std::map<std::string, int>::const_iterator found = tags.find(tag);
            if(found != tags.end()) {
                word_tags.push_back(found->second);
            } else {
                word_tags.push_back(tags.size() + 1);
                tags[tag] = tags.size();
                reverse_tags.push_back(tag);
            }
        }
        words[word] = words.size();
        //std::cerr << words[word] << "\n";
        tags_for_word.push_back(word_tags);
    }
    std::string word;
    fst::StdVectorFst automaton;
    automaton.AddState();
    fst::SymbolTable isyms("input");
    fst::SymbolTable osyms("output");
    isyms.AddSymbol("<eps>");
    osyms.AddSymbol("<eps>");
    while(!std::cin.eof()) {
        if(!(std::cin >> word)) break;
        std::map<std::string, int>::const_iterator found = words.find(word);
        automaton.AddState();
        if(found == words.end()) {
            words[word] = words.size();
            tags_for_word.push_back(std::vector<int>());
            tags_for_word.back().push_back(tags["np"]);
        }
        //std::cerr << words[word] << "\n";
        for(std::vector<int>::const_iterator tag = tags_for_word[words[word] - 1].begin(); tag != tags_for_word[words[word] - 1].end(); tag++) {
            //std::cerr << *tag << "\n";
            int64 word_symbol = isyms.AddSymbol(word);
            int64 tag_symbol = osyms.AddSymbol(reverse_tags[*tag - 1]);
            automaton.AddArc(automaton.NumStates() - 2, fst::StdArc(word_symbol, tag_symbol, 0, automaton.NumStates() - 1));
        }
    }
    automaton.SetFinal(automaton.NumStates() - 1, 0);
    automaton.SetInputSymbols(&isyms);
    automaton.SetOutputSymbols(&osyms);
    automaton.SetStart(0);
    automaton.Write("");
}
