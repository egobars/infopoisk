#include "../b-tree/b_tree.h"
#include "../disk_component/component.h"

#include <algorithm>
#include <bitset>
#include <map>

class LSMTree {
public:
    LSMTree(size_t min_degree = 2, size_t max_components = 1, size_t component_size_multiplier = 10)
        : MaxComponents_(max_components)
        , ComponentSizeMultiplier_(component_size_multiplier)
        , BTree_(min_degree)
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

    bool Get(std::string& key, std::string& result_value) {
        auto result = BTree_.Get(key);
        if (result.IsDeleted) {
            return false;
        }
        if (result.IsFound) {
            result_value = std::move(result.Value);
            return true;
        }

        for (size_t i = 0; i < MaxComponents_; ++i) {
            result = Components_[i].Get(key);
            if (result.IsDeleted) {
                return false;
            }
            if (result.IsFound) {
                result_value = std::move(result.Value);
                return true;
            }
        }
        return false;
    }

    std::vector<std::pair<std::string, V>> GetQuery(std::string& start_key, std::string& end_key) {
        std::vector<std::pair<std::string, V>> result;
        std::map<std::string, bool> is_key_deleted;
        auto tree_result = BTree_.GetQuery(start_key, end_key);
        for (auto& kvt : tree_result) {
            if (kvt.Tombstone) {
                is_key_deleted[kvt.Key] = true;
            } else {
                result.emplace_back(kvt.Key, kvt.Value);
            }
        }

        for (size_t i = 0; i < MaxComponents_; ++i) {
            std::vector<KVTombstone> cmp_result;
            Components_[i].GetQuery(start_key, end_key, cmp_result);

            for (auto& kvt : cmp_result) {
                if (kvt.Tombstone) {
                    is_key_deleted[kvt.Key] = true;
                } else {
                    if (is_key_deleted.find(kvt.Key) == is_key_deleted.end()) {
                        result.emplace_back(kvt.Key, kvt.Value);
                    }
                }
            }
        }

        std::sort(result.begin(), result.end());
        return result;
    }

    void Add(std::string& key, std::string& value) {
        BTree_.Add(key, value);

        if (BTree_.GetSize() > ComponentSizeMultiplier_ && MaxComponents_ > 0) {
            std::vector<KVTombstone> b_tree_data = BTree_.List();
            size_t first_ptr = 0;
            size_t second_ptr = 0;
            auto first_kvt = b_tree_data[first_ptr];
            size_t first_size = b_tree_data.size();
            size_t second_size = Components_[0].GetSize();
            std::string tmp_file_name = FileNames_[0] + "_tmp";
            FILE* tmp_file = fopen(tmp_file_name.c_str(), "wb");
            fclose(tmp_file);
            tmp_file = fopen(tmp_file_name.c_str(), "rb+");

            FILE* file = fopen(FileNames_[0].c_str(), "rb");
            KVTombstone second_kvt;
            if (second_size != 0) {
                Components_[0].ReadFromFile(second_ptr, second_kvt, file);
            }
            while (first_ptr < first_size || second_ptr < second_size) {
                if (first_ptr == first_size) {
                    MoveComponentPointer(second_ptr, 0, 0, second_kvt, second_size, file, tmp_file);
                    continue;
                }
                if (second_ptr == second_size) {
                    Components_[0].WriteToFile(first_kvt, tmp_file, true);
                    ++first_ptr;
                    if (first_ptr != first_size) {
                        first_kvt = b_tree_data[first_ptr];
                    }
                    continue;
                }
                if (first_kvt.Key < second_kvt.Key) {
                    Components_[0].WriteToFile(first_kvt, tmp_file, true);
                    ++first_ptr;
                    if (first_ptr != first_size) {
                        first_kvt = b_tree_data[first_ptr];
                    }
                } else if (first_kvt.Key == second_kvt.Key) {
                    Components_[0].WriteToFile(first_kvt, tmp_file, true);
                    ++first_ptr;
                    if (first_ptr != first_size) {
                        first_kvt = b_tree_data[first_ptr];
                    }
                    ++second_ptr;
                    if (second_ptr != second_size) {
                        Components_[0].ReadFromFile(second_ptr, second_kvt, file);
                    }
                } else {
                    MoveComponentPointer(second_ptr, 0, 0, second_kvt, second_size, file, tmp_file);
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
                KVTombstone first_kvt;
                if (first_size != 0) {
                    Components_[i].ReadFromFile(first_ptr, first_kvt, file1);
                }

                FILE* file2 = fopen(FileNames_[i + 1].c_str(), "rb");
                KVTombstone second_kvt;
                if (second_size != 0) {
                    Components_[i + 1].ReadFromFile(second_ptr, second_kvt, file2);
                }
                while (first_ptr < first_size || second_ptr < second_size) {
                    if (first_ptr == first_size) {
                        MoveComponentPointer(second_ptr, i + 1, i + 1, second_kvt, second_size, file2, tmp_file);
                        continue;
                    }
                    if (second_ptr == second_size) {
                        MoveComponentPointer(first_ptr, i, i + 1, first_kvt, first_size, file1, tmp_file);
                        continue;
                    }
                    if (first_kvt.Key < second_kvt.Key) {
                        MoveComponentPointer(first_ptr, i, i + 1, first_kvt, first_size, file1, tmp_file);
                        continue;
                    } else if (first_kvt.Key == second_kvt.Key) {
                        MoveComponentPointer(first_ptr, i, i + 1, first_kvt, first_size, file1, tmp_file);
                        ++second_ptr;
                        if (second_ptr != second_size) {
                            Components_[i + 1].ReadFromFile(second_ptr, second_kvt, file2);
                        }
                    } else {
                        MoveComponentPointer(second_ptr, i + 1, i + 1, second_kvt, second_size, file2, tmp_file);
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

    void Delete(std::string& key) {
        BTree_.Delete(key);
    }

private:
    void MoveComponentPointer(size_t& pointer, size_t read_index, size_t write_index, KVTombstone& kvt, size_t max_size, FILE* file, FILE* tmp_file) {
        Components_[write_index].WriteToFile(kvt, tmp_file, true);
        ++pointer;
        if (pointer != max_size) {
            Components_[read_index].ReadFromFile(pointer, kvt, file);
        }
    }

    const size_t BUFFER_SIZE = 1024;

    size_t MaxComponents_;
    size_t ComponentSizeMultiplier_;
    BTree BTree_;
    std::vector<std::string> FileNames_;
    std::vector<DiskComponent> Components_ = {};
};
