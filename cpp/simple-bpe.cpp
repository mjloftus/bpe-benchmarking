#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::string WORD_TERM_TOKEN = "\\w";

class Token {
    public:
        size_t start;
        size_t end;
        size_t next;
        Token(size_t val) {
            start = val;
            end = val + 1;
            next = end;
        }
};

class Tokenization {
    public:
        int count = 0;
        // todo Token locality (vector)
        std::vector<Token> tokenization;
        std::string_view word;
        std::string w;
        Tokenization(const std::string& textWord) {
            this->w = textWord + WORD_TERM_TOKEN;
            tokenization.reserve(this->w.size()+1);
            tokenization.push_back(Token(0));
            for (size_t i = 1; i < textWord.size(); ++i) {
                tokenization.push_back(Token(i));
            }
            // handle length 2 WORD_TERM_TOKEN
            tokenization.push_back(Token(textWord.size()));
            tokenization.back().end = this->w.size();
            tokenization.back().next = tokenization.size() + 1;
        }

        void populateStringView() {
            this->word = std::string_view(this->w);
        }
};

void initializeCorpus(std::ifstream& trainingData, std::unordered_map<std::string, Tokenization>& corpus) {
    std::string line;
    while (getline(trainingData, line)) {
        auto i = line.begin();
        while (i != line.end()) {
            while (i != line.end() && *i == ' ') ++i;
            if (i !=  line.end()) {
                auto j = i + 1;
                while (j != line.end() && *j != ' ') ++j;
                std::string word = std::string(i, j);
                if (corpus.find(word) == corpus.end()) {
                    corpus.insert({word, Tokenization(word)});
                    corpus.find(word)->second.populateStringView(); // need copy constructor
                }
                ++corpus.find(word)->second.count;
                i = j;
            }
        }
    }
}

std::string findMostFreqTokenPair(const std::unordered_map<std::string, Tokenization>& corpus) {
    std::unordered_map<std::string_view, int> pairCount;
    pairCount.reserve(10000);
    for (const auto &word : corpus) {
        for (size_t i = 0; word.second.tokenization[i].next < word.second.tokenization.size(); i = word.second.tokenization[i].next) {
            const auto& token = word.second.tokenization[i];
            const size_t& start = token.start;
            const size_t& end = word.second.tokenization[token.next].end;
            pairCount[word.second.word.substr(start, end - start)] += word.second.count;
        }
    }
    int maxFreqCount = 0;
    std::string maxFreqToken = "";
    for (const auto &pair : pairCount) {
        if (pair.second > maxFreqCount) {
            maxFreqCount = pair.second;
            maxFreqToken = pair.first;
        }
    }
    return maxFreqToken;
}

void updateCorpus(std::unordered_map<std::string, Tokenization>& corpus, const std::string& maxFreqToken) {
    for (auto &word : corpus) {
        for (size_t i = 0; i < word.second.tokenization.size() && word.second.tokenization[i].next < word.second.tokenization.size(); i = word.second.tokenization[i].next) {
            auto& token = word.second.tokenization[i];
            const size_t start = word.second.tokenization[i].start;
            const size_t end = word.second.tokenization[word.second.tokenization[i].next].end;
            if (word.second.word.substr(start, end - start) == maxFreqToken) {
                word.second.tokenization[i].end = word.second.tokenization[token.next].end;
                size_t next = word.second.tokenization[token.next].next;
                word.second.tokenization[token.next].next = word.second.tokenization.size() + 1;
                word.second.tokenization[i].next = next;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    auto begin = std::chrono::high_resolution_clock::now();
    // cmd line args
    int iterations = 10;
    std::ifstream trainingData;
    std::ofstream outputFile;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-k") && i < argc - 1) iterations = atoi(argv[++i]);
        if (!strcmp(argv[i], "-i") && i < argc - 1) trainingData.open(argv[++i]);
        if (!strcmp(argv[i], "-o") && i < argc - 1) outputFile.open(argv[++i]);
    }

    if (!trainingData.is_open()) {
        std::cout << "no input file found\n";
        return 0;
    }

    // init corpus
    std::string line;
    std::unordered_map<std::string, Tokenization> corpus;
    std::unordered_set<std::string> vocab;
    
    initializeCorpus(trainingData, corpus);
    trainingData.close();

    // perform bpe
    for (int i = 0; i < iterations; ++i) {
        std::string maxFreqToken = findMostFreqTokenPair(corpus);
        if (maxFreqToken == "") break;
        vocab.insert(maxFreqToken);
        updateCorpus(corpus, maxFreqToken);
    }
    auto end = std::chrono::high_resolution_clock::now();

    // output
    if (outputFile.is_open()) {
        for (auto token : vocab) {
            outputFile << token << '\n';
        }
        outputFile.close();
    }
    else {
        for (auto token : vocab) {
            std::cout << token << '\n';
        }
    }

    std::cout << "duration: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << " µs\n";

    return 0;
}
