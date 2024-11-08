#include "../b-tree/b_tree.h"
#include "../disk_component/component.h"
#include "english_stem.h"

#include <algorithm>
#include <bitset>
#include <map>
#include <sstream>
#include <unordered_map>

class Finder;

class Trie {
public:
    Trie(size_t level)
        : Level_(level)
    {
        if (Level_ < 2) {
            Edges_.resize(255, Trie(level + 1));
        }
    }

    void Add(std::string& word, size_t index) {
        IsActivated_ = true;
        Value_.add(index);
        if (Level_ < 2 && Level_ < word.size()) {
            Edges_[word[Level_]].Add(word, index);
        }
    }

    roaring::Roaring Get(std::string& word) {
        if (Level_ == 2) {
            return Value_;
        }
        if (Level_ < word.size() && Edges_[word[Level_]].IsActivated()) {
            return Edges_[word[Level_]].Get(word);
        }
        return Value_;
    }

    bool IsActivated() {
        return IsActivated_;
    }

private:
    size_t Level_ = 0;
    bool IsActivated_ = false;
    std::vector<Trie> Edges_;
    roaring::Roaring Value_;
};

class Index {
    friend class Finder;
public:
    Index(size_t min_degree = 2, size_t max_components = 1, size_t component_size_multiplier = 10)
        : MaxComponents_(max_components)
        , ComponentSizeMultiplier_(component_size_multiplier)
        , BTree_(min_degree)
        , Trie_(0)
    {
        for (size_t i = 0; i < max_components; ++i) {
            std::string file_name = "file_";
            file_name += '0' + i;
            FileNames_.emplace_back(std::move(file_name));
            Components_.emplace_back(FileNames_[i]);

            FILE* file = fopen(FileNames_[i].c_str(), "wb");
            fclose(file);
        }
    }

    void AddDocument(std::string document_name, std::string document_text) {
        K cur_index = DocumentNames_.size();
        DocumentNames_.emplace_back(document_name);
        auto words = ParseDocument(document_text);
        auto to_add_bitmap = roaring::Roaring();
        to_add_bitmap.add(cur_index);

        for (auto& word : words) {
            if (IndexByWord_.find(word) == IndexByWord_.end()) {
                IndexByWord_[word] = IndexByWord_.size();
                Words_.push_back(word);
            }
            Add(IndexByWord_[word], to_add_bitmap);

            auto cur_word_index = IndexByWord_[word];
            if (word.size() > 2) {
                for (size_t i = 0; i < word.size() - 2; ++i) {
                    std::string n_gram = word.substr(i, 3);
                    if (WordsByNGram_.find(n_gram) == WordsByNGram_.end()) {
                        WordsByNGram_[n_gram] = roaring::Roaring();
                    }
                    WordsByNGram_[n_gram].add(cur_word_index);
                }
            }

            Trie_.Add(word, cur_word_index);
        }
    }

    roaring::Roaring GetDocumentsByWord(std::string word) {
        roaring::Roaring result;
        // Lemmatize(word);
        if (IndexByWord_.find(word) == IndexByWord_.end()) {
            return result;
        }
        Get(IndexByWord_[word], result);
        return result;
    }

    roaring::Roaring GetDocumentsByWildcard(std::string word) {
        roaring::Roaring ok_words;
        size_t wildcard_index = 0;
        bool is_met = false;
        for (auto symbol : word) {
            if (symbol == '*') {
                is_met = true;
                break;
            }
            ++wildcard_index;
        }
        if (!is_met) {
            return ok_words;
        }
        std::string left = word.substr(0, wildcard_index);
        std::string right = word.substr(wildcard_index + 1, word.size() - wildcard_index - 1);
        if (left.size() > 2) {
            for (size_t i = 0; i < left.size() - 2; ++i) {
                std::string n_gram = left.substr(i, 3);
                if (WordsByNGram_.find(n_gram) == WordsByNGram_.end()) {
                    continue;
                }
                ok_words |= WordsByNGram_[n_gram];
            }
        } else {
            ok_words = Trie_.Get(left);
        }
        if (right.size() > 2) {
            for (size_t i = 0; i < right.size() - 2; ++i) {
                std::string n_gram = right.substr(i, 3);
                if (WordsByNGram_.find(n_gram) == WordsByNGram_.end()) {
                    continue;
                }
                ok_words |= WordsByNGram_[n_gram];
            }
        }
        roaring::Roaring cleared_ok_words;
        for (auto idx : ok_words) {
            auto cur_word = Words_[idx];
            if (cur_word.size() < left.size() + right.size()) {
                continue;
            }
            if (cur_word.substr(0, wildcard_index) != left || cur_word.substr(cur_word.size() - right.size(), right.size()) != right) {
                continue;
            }
            cleared_ok_words.add(idx);
        }
        roaring::Roaring result;
        for (auto idx : cleared_ok_words) {
            roaring::Roaring new_word_documents;
            Get(idx, new_word_documents);
            result |= new_word_documents;
        }
        return result;
    }

    void Get(K key, V& result_value) {
        result_value = BTree_.Get(key).Value;
        for (size_t i = 0; i < MaxComponents_; ++i) {
            result_value |= Components_[i].Get(key).Value;
        }
    }

    void Add(K key, V& value) {
        BTree_.Add(key, value);

        if (BTree_.GetSize() > ComponentSizeMultiplier_ && MaxComponents_ > 0) {
            std::vector<KV> b_tree_data = BTree_.List();
            size_t first_ptr = 0;
            size_t second_ptr = 0;
            auto first_kv = b_tree_data[first_ptr];
            size_t first_size = b_tree_data.size();
            size_t second_size = Components_[0].GetSize();
            std::string tmp_file_name = FileNames_[0] + "_tmp";
            FILE* tmp_file = fopen(tmp_file_name.c_str(), "wb");
            fclose(tmp_file);
            tmp_file = fopen(tmp_file_name.c_str(), "rb+");

            FILE* file = fopen(FileNames_[0].c_str(), "rb");
            KV second_kv;
            if (second_size != 0) {
                Components_[0].ReadFromFile(second_ptr, second_kv, file);
            }
            while (first_ptr < first_size || second_ptr < second_size) {
                if (first_ptr == first_size) {
                    MoveComponentPointer(second_ptr, 0, 0, second_kv, second_size, file, tmp_file);
                    continue;
                }
                if (second_ptr == second_size) {
                    Components_[0].WriteToFile(first_kv, tmp_file, true);
                    ++first_ptr;
                    if (first_ptr != first_size) {
                        first_kv = b_tree_data[first_ptr];
                    }
                    continue;
                }
                if (first_kv.Key < second_kv.Key) {
                    Components_[0].WriteToFile(first_kv, tmp_file, true);
                    ++first_ptr;
                    if (first_ptr != first_size) {
                        first_kv = b_tree_data[first_ptr];
                    }
                } else if (first_kv.Key == second_kv.Key) {
                    first_kv.Value |= second_kv.Value;
                    Components_[0].WriteToFile(first_kv, tmp_file, true);
                    ++first_ptr;
                    if (first_ptr != first_size) {
                        first_kv = b_tree_data[first_ptr];
                    }
                    ++second_ptr;
                    if (second_ptr != second_size) {
                        Components_[0].ReadFromFile(second_ptr, second_kv, file);
                    }
                } else {
                    MoveComponentPointer(second_ptr, 0, 0, second_kv, second_size, file, tmp_file);
                }
            }
            fclose(file);
            file = fopen(FileNames_[0].c_str(), "wb");

            size_t file_size = ftell(tmp_file);
            fseek(tmp_file, 0, SEEK_SET);
            char buffer[BUFFER_SIZE + 1];
            for (size_t i = 0; i < file_size / BUFFER_SIZE; ++i) {
                fread(buffer, sizeof(char), BUFFER_SIZE, tmp_file);
                fwrite(buffer, sizeof(char), BUFFER_SIZE, file);
            }
            fread(buffer, sizeof(char), file_size % BUFFER_SIZE, tmp_file);
            fwrite(buffer, sizeof(char), file_size % BUFFER_SIZE, file);

            fclose(file);
            fclose(tmp_file);

            tmp_file = fopen(tmp_file_name.c_str(), "wb");
            fclose(tmp_file);
            BTree_.Erase();
            Components_[0].SwapTmp();
        }

        size_t cur_component_max_size = ComponentSizeMultiplier_ * ComponentSizeMultiplier_;
        for (size_t i = 0; i < MaxComponents_ - 1; ++i) {
            if (Components_[i].GetSize() > cur_component_max_size) {
                size_t first_ptr = 0;
                size_t second_ptr = 0;

                size_t first_size = Components_[i].GetSize();
                size_t second_size = Components_[i + 1].GetSize();

                std::string tmp_file_name = FileNames_[i + 1] + "_tmp";
                FILE* tmp_file = fopen(tmp_file_name.c_str(), "wb");
                fclose(tmp_file);
                tmp_file = fopen(tmp_file_name.c_str(), "rb+");

                FILE* file1 = fopen(FileNames_[i].c_str(), "rb");
                KV first_kv;
                if (first_size != 0) {
                    Components_[i].ReadFromFile(first_ptr, first_kv, file1);
                }

                FILE* file2 = fopen(FileNames_[i + 1].c_str(), "rb");
                KV second_kv;
                if (second_size != 0) {
                    Components_[i + 1].ReadFromFile(second_ptr, second_kv, file2);
                }
                while (first_ptr < first_size || second_ptr < second_size) {
                    if (first_ptr == first_size) {
                        MoveComponentPointer(second_ptr, i + 1, i + 1, second_kv, second_size, file2, tmp_file);
                        continue;
                    }
                    if (second_ptr == second_size) {
                        MoveComponentPointer(first_ptr, i, i + 1, first_kv, first_size, file1, tmp_file);
                        continue;
                    }
                    if (first_kv.Key < second_kv.Key) {
                        MoveComponentPointer(first_ptr, i, i + 1, first_kv, first_size, file1, tmp_file);
                        continue;
                    } else if (first_kv.Key == second_kv.Key) {
                        first_kv.Value |= second_kv.Value;
                        MoveComponentPointer(first_ptr, i, i + 1, first_kv, first_size, file1, tmp_file);
                        ++second_ptr;
                        if (second_ptr != second_size) {
                            Components_[i + 1].ReadFromFile(second_ptr, second_kv, file2);
                        }
                    } else {
                        MoveComponentPointer(second_ptr, i + 1, i + 1, second_kv, second_size, file2, tmp_file);
                    }
                }
                fclose(file1);
                file1 = fopen(FileNames_[i].c_str(), "wb");
                fclose(file1);
                fclose(file2);
                file2 = fopen(FileNames_[i + 1].c_str(), "wb");

                size_t file2_size = ftell(tmp_file);
                fseek(tmp_file, 0, SEEK_SET);
                char buffer[BUFFER_SIZE + 1];
                for (size_t i = 0; i < file2_size / BUFFER_SIZE; ++i) {
                    fread(buffer, sizeof(char), BUFFER_SIZE, tmp_file);
                    fwrite(buffer, sizeof(char), BUFFER_SIZE, file2);
                }
                fread(buffer, sizeof(char), file2_size % BUFFER_SIZE, tmp_file);
                fwrite(buffer, sizeof(char), file2_size % BUFFER_SIZE, file2);

                fclose(file2);
                fclose(tmp_file);

                tmp_file = fopen(tmp_file_name.c_str(), "wb");
                fclose(tmp_file);
                Components_[i].Erase();
                Components_[i + 1].SwapTmp();
            }

            cur_component_max_size *= ComponentSizeMultiplier_;
        }
    }

    void Delete(K key) {
        BTree_.Delete(key);
    }

private:
    void Lemmatize(std::string& word) {
        std::string result;
        std::remove_copy_if(
            word.begin(), word.end(),            
            std::back_inserter(result),   
            std::ptr_fun<int, int>(&std::ispunct)  
        );
        std::transform(
            result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); }
        );
        stemming::english_stem<> StemEnglish;
        std::wstring wword(result.begin(), result.end());
        StemEnglish(wword);
        word = std::string(wword.begin(), wword.end());
    }

    std::vector<std::string> ParseDocument(std::string& document_text) {
        std::vector<std::string> result;

        std::stringstream stream(document_text);
        std::string word;
        while (stream >> word) {
            /*if (word.size() < 3) {
                continue;
            }
            Lemmatize(word);*/
            result.emplace_back(word);
        }
        return result;
    }

    void MoveComponentPointer(size_t& pointer, size_t read_index, size_t write_index, KV& kv, size_t max_size, FILE* file, FILE* tmp_file) {
        Components_[write_index].WriteToFile(kv, tmp_file, true);
        ++pointer;
        if (pointer != max_size) {
            Components_[read_index].ReadFromFile(pointer, kv, file);
        }
    }

    const size_t BUFFER_SIZE = 1024;

    size_t MaxComponents_;
    size_t ComponentSizeMultiplier_;
    BTree BTree_;
    std::vector<std::string> FileNames_;
    std::vector<DiskComponent> Components_ = {};

    std::vector<std::string> DocumentNames_;
    std::unordered_map<std::string, K> IndexByWord_;
    std::vector<std::string> Words_;

    std::unordered_map<std::string, roaring::Roaring> WordsByNGram_;
    Trie Trie_;
};

class Finder {
public:
    Finder(Index& index)
        : Index_(index)
    {}

    Finder(Index& index, std::string word, bool is_wildcard)
        : Index_(index)
        , Bitmap_(is_wildcard ? Index_.GetDocumentsByWildcard(word) : Index_.GetDocumentsByWord(word))
    {}

    Finder& And(Finder& finder) {
        Bitmap_ &= finder.Bitmap_;
        return *this;
    }

    Finder& And(std::string word) {
        Finder finder(Index_, word, false);
        return And(finder);
    }

    Finder& Or(Finder& finder) {
        Bitmap_ |= finder.Bitmap_;
        return *this;
    }

    Finder& Or(std::string word) {
        Finder finder(Index_, word, false);
        return Or(finder);
    }

    Finder& Without(Finder& finder) {
        Bitmap_ -= finder.Bitmap_;
        return *this;
    }

    Finder& Without(std::string word) {
        Finder finder(Index_, word, false);
        return Without(finder);
    }

    std::vector<std::string> GetDocuments() {
        std::vector<std::string> result;
        uint32_t* indexes = new uint32_t[Bitmap_.cardinality()];
        Bitmap_.toUint32Array(indexes);
        for (size_t i = 0; i < Bitmap_.cardinality(); ++i) {
            result.emplace_back(Index_.DocumentNames_[indexes[i]]);
        }
        return result;
    }

private:
    Index& Index_;
    roaring::Roaring Bitmap_;
};
