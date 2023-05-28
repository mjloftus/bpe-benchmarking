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
        Token* next;
        Token(int val) {
            start = val;
            end = val + 1;
            next = nullptr;
        }
};

class Tokenization {
    public:
        int count = 0;
        Token* head;
        std::string_view word;
        std::string w;
        Tokenization(const std::string& textWord) {
            this->w = textWord + WORD_TERM_TOKEN;
            head = new Token(0);
            Token* cur = head;
            for (size_t i = 1; i < textWord.size(); ++i) {
                cur->next = new Token(i);
                cur = cur->next;
            }
            // handle length 2 WORD_TERM_TOKEN
            cur->next = new Token(textWord.size());
            ++cur->next->end;
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
        for (auto i = word.second.head; i->next != nullptr; i = i->next) {
            pairCount[word.second.word.substr(i->start, i->next->end - i->start)] += word.second.count;
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
        for (auto i = word.second.head; i != nullptr && i->next != nullptr; i = i->next) {
            if (word.second.word.substr(i->start, i->next->end - i->start) == maxFreqToken) {
                i->end = i->next->end;
                Token* toDelete = i->next;
                i->next = i->next->next;
                delete toDelete;
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
