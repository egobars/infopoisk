#include "../common/common.h"

#include <memory>
#include <vector>
#include <string>
#include <vector>
#include <iostream>


class BTree {
public:
    BTree(size_t min_degree)
        : MinDegree_(min_degree) {
        Root_ = std::make_shared<BTreeNode>(min_degree);
        Root_->SetIsLeaf(true);
    }

    GetResult Get(K& key) {
        GetResult result = { false, V(), false };
        if (!Root_) {
            return result;
        }
        auto tmpResult = Root_->Get(key);
        if (tmpResult.IsFound && !tmpResult.IsDeleted) {
            return { tmpResult.IsFound, *tmpResult.Value, tmpResult.IsDeleted };
        }
        return { tmpResult.IsFound, V(), tmpResult.IsDeleted };
    }

    std::vector<KVTombstone> GetQuery(K& start_key, K& end_key) {
        std::vector<KVTombstone> result;
        Root_->GetQuery(start_key, end_key, result);
        return result;
    }

    void Add(K& key, V& value) {
        if (Root_->Add(key, value)) {
            ++Size_;
        }

        if (Root_->Size() == MinDegree_ * 2 - 1) {
            auto new_child = std::make_shared<BTreeNode>(MinDegree_);
            auto new_root = std::make_shared<BTreeNode>(MinDegree_);
            auto key_value = Root_->GetMedian();
            new_root->AppendKeyAndValue(key_value.first, key_value.second);
            new_root->AppendChild(Root_);
            new_root->AppendChild(new_child);
            Root_->Split(new_child);
            if (Root_->GetIsLeaf()) {
                new_child->SetIsLeaf(true);
            }
            Root_ = new_root;
        }
    }

    void Delete(K& key) {
        V dummy = V();
        Root_->Add(key, dummy, true);
    }

    std::vector<KVTombstone> List() {
        std::vector<KVTombstone> result;
        Root_->List(result);
        return result;
    }

    void Erase() {
        Root_ = std::make_shared<BTreeNode>(MinDegree_);
        Root_->SetIsLeaf(true);
        Size_ = 0;
    }

    size_t GetSize() {
        return Size_;
    }

private:
    struct TmpGetResult {
        bool IsFound = false;
        V* Value;
        bool IsDeleted = false;
    };

    class BTreeNode {
        friend class BTree;
    public:
        BTreeNode(size_t min_degree)
            : MinDegree_(min_degree) 
        {};

        TmpGetResult Get(K& key) {
            TmpGetResult badResult = { false, nullptr, false };
            for (size_t i = 0; i < Keys_.size(); ++i) {
                if (Keys_[i].Key == key) {
                    return { true, &Values_[i], Keys_[i].Tombstone };
                }
                if (Keys_[i].Key > key) {
                    return IsLeaf_ ? badResult : Childs_[i]->Get(key);
                }
            }
            return IsLeaf_ ? badResult : Childs_.back()->Get(key);
        }

        void GetQuery(K& start_key, K& end_key, std::vector<KVTombstone>& result) {
            for (size_t i = 0; i < Keys_.size(); ++i) {
                if (!IsLeaf_) {
                    if (Keys_[i].Key > start_key) {
                        Childs_[i]->GetQuery(start_key, end_key, result);
                    }
                }
                if (Keys_[i].Key >= start_key && Keys_[i].Key <= end_key) {
                    result.push_back(KVTombstone());
                    result.back().Key = Keys_[i].Key;
                    result.back().Value = Values_[i];
                    result.back().Tombstone = Keys_[i].Tombstone;
                }
            }
            if (!IsLeaf_) {
                if (Keys_.back().Key < end_key) {
                    Childs_.back()->GetQuery(start_key, end_key, result);
                }
            }
        }

        bool Add(K& key, V& value, bool is_deleting=false) {
            for (size_t i = 0; i < Keys_.size(); ++i) {
                if (Keys_[i].Key == key) {
                    Keys_[i].Tombstone = is_deleting;
                    Values_[i] = value;
                    return false;
                }
                if (Keys_[i].Key > key) {
                    if (IsLeaf_) {
                        Keys_.insert(Keys_.begin() + i, {
                            key,
                            is_deleting
                        });
                        Values_.insert(Values_.begin() + i, value);
                        return true;
                    }
                    return AddToChild(i, key, value, is_deleting);
                }
            }
            if (IsLeaf_) {
                Keys_.push_back({ key, is_deleting });
                Values_.push_back(value);
                return true;
            }
            return AddToChild(Keys_.size(), key, value, is_deleting);
        }

        void List(std::vector<KVTombstone>& result) {
            for (size_t i = 0; i < Keys_.size(); ++i) {
                if (!IsLeaf_) {
                    Childs_[i]->List(result);
                }
                result.emplace_back(Keys_[i].Key, Values_[i], Keys_[i].Tombstone);
            }
            if (!IsLeaf_) {
                Childs_.back()->List(result);
            }
        }

    private:
        struct KeyWithTombstone {
            K Key;
            bool Tombstone;
        };

        size_t Size() {
            return Keys_.size();
        }

        bool GetIsLeaf() {
            return IsLeaf_;
        }

        void SetIsLeaf(bool value) {
            IsLeaf_ = value;
        }

        std::pair<KeyWithTombstone&, V&> GetMedian() {
            return { Keys_[MinDegree_ - 1], Values_[MinDegree_ - 1] };
        }

        void AppendKeyAndValue(KeyWithTombstone& key, V& value) {
            Keys_.push_back(key);
            Values_.push_back(value);
        }

        void AppendChild(std::shared_ptr<BTreeNode> child) {
            Childs_.push_back(child);
        }

        void Split(std::shared_ptr<BTreeNode> rightNode) {
            for (size_t i = MinDegree_; i < Keys_.size(); ++i) {
                rightNode->AppendKeyAndValue(Keys_[i], Values_[i]);
                if (!IsLeaf_) {
                    rightNode->AppendChild(Childs_[i]);
                }
            }
            Keys_.resize(MinDegree_ - 1);
            Values_.resize(MinDegree_ - 1);
            if (!IsLeaf_) {
                rightNode->AppendChild(Childs_.back());
                Childs_.resize(MinDegree_);
            }
        }

        bool AddToChild(size_t index, K& key, V& value, bool is_deleted=false) {
            bool adding_result = Childs_[index]->Add(key, value, is_deleted);
            if (Childs_[index]->Size() == 2 * MinDegree_ - 1) {
                auto new_node = std::make_shared<BTreeNode>(MinDegree_);
                Childs_.insert(Childs_.begin() + index + 1, new_node);
                auto key_value = Childs_[index]->GetMedian();
                Keys_.insert(Keys_.begin() + index, key_value.first);
                Values_.insert(Values_.begin() + index, key_value.second);
                Childs_[index]->Split(new_node);
                if (Childs_[index]->GetIsLeaf()) {
                    new_node->SetIsLeaf(true);
                }
            }
            return adding_result;
        }

        size_t MinDegree_;
        bool IsLeaf_ = false;
        std::vector<KeyWithTombstone> Keys_;
        std::vector<V> Values_;
        std::vector<std::shared_ptr<BTreeNode>> Childs_;
        std::shared_ptr<BTreeNode> Parent_;
    };

    std::shared_ptr<BTreeNode> Root_;
    size_t Size_ = 0;
    size_t MinDegree_;
};
