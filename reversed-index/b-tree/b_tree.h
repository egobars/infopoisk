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
        GetResult result = { false, V() };
        if (!Root_) {
            return result;
        }
        auto tmpResult = Root_->Get(key);
        if (tmpResult.IsFound) {
            return { tmpResult.IsFound, *tmpResult.Value };
        }
        return { tmpResult.IsFound, V() };
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
        Root_->Add(key, dummy);
    }

    std::vector<KV> List() {
        std::vector<KV> result;
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

        TmpGetResult(bool isFound, V* value)
            : IsFound(isFound)
            , Value(value)
        {}
    };

    class BTreeNode {
        friend class BTree;
    public:
        BTreeNode(size_t min_degree)
            : MinDegree_(min_degree) 
        {};

        TmpGetResult Get(K& key) {
            TmpGetResult badResult = { false, nullptr };
            for (size_t i = 0; i < Keys_.size(); ++i) {
                if (Keys_[i] == key) {
                    return { true, &Values_[i] };
                }
                if (Keys_[i] > key) {
                    return IsLeaf_ ? badResult : Childs_[i]->Get(key);
                }
            }
            return IsLeaf_ ? badResult : Childs_.back()->Get(key);
        }

        bool Add(K& key, V& value) {
            for (size_t i = 0; i < Keys_.size(); ++i) {
                if (Keys_[i] == key) {
                    Values_[i] |= value;
                    return false;
                }
                if (Keys_[i] > key) {
                    if (IsLeaf_) {
                        Keys_.insert(Keys_.begin() + i, key);
                        Values_.insert(Values_.begin() + i, value);
                        return true;
                    }
                    return AddToChild(i, key, value);
                }
            }
            if (IsLeaf_) {
                Keys_.push_back(key);
                Values_.push_back(value);
                return true;
            }
            return AddToChild(Keys_.size(), key, value);
        }

        void List(std::vector<KV>& result) {
            for (size_t i = 0; i < Keys_.size(); ++i) {
                if (!IsLeaf_) {
                    Childs_[i]->List(result);
                }
                result.emplace_back(Keys_[i], Values_[i]);
            }
            if (!IsLeaf_) {
                Childs_.back()->List(result);
            }
        }

    private:
        size_t Size() {
            return Keys_.size();
        }

        bool GetIsLeaf() {
            return IsLeaf_;
        }

        void SetIsLeaf(bool value) {
            IsLeaf_ = value;
        }

        std::pair<K&, V&> GetMedian() {
            return { Keys_[MinDegree_ - 1], Values_[MinDegree_ - 1] };
        }

        void AppendKeyAndValue(K& key, V& value) {
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

        bool AddToChild(size_t index, K& key, V& value) {
            bool adding_result = Childs_[index]->Add(key, value);
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
        std::vector<K> Keys_;
        std::vector<V> Values_;
        std::vector<std::shared_ptr<BTreeNode>> Childs_;
        std::shared_ptr<BTreeNode> Parent_;
    };

    std::shared_ptr<BTreeNode> Root_;
    size_t Size_ = 0;
    size_t MinDegree_;
};
