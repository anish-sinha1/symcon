#include <iostream>
#include <mutex>
#include <optional>
#include <stack>
#include <vector>

class Node {
   public:
    explicit Node(const int min_order) : min_order_{min_order} {
        out_link_ = right_link_ = nullptr;
        leaf_ = true;
        root_ = false;
    };
    ~Node() = default;
    inline void Latch() {
        latch_.lock();
    }

    inline void Unlatch() {
        latch_.unlock();
    }

    inline void SetRoot(bool root) {
        root_ = root;
    }

    inline auto IsRoot() -> bool {
        return root_;
    }

    inline void SetLeaf(bool leaf) {
        leaf_ = leaf;
    }

    inline auto IsLeaf() -> bool {
        return leaf_;
    }

    inline void SetKeys(std::vector<int*>& keys) {
        keys_ = keys;
    }

    inline auto GetChildren() -> std::vector<void*>& {
        return children_;
    }

    inline void SetChildren(std::vector<void*>& children) {
        children_ = children;
    }

    inline void SetRightLink(Node* right_link) {
        right_link_ = right_link;
    }

    inline void SetOutLink(Node* out_link) {
        out_link_ = out_link;
    }

    auto InsertSafe(int* key) -> bool {
        keys_.push_back(key);
        std::sort(keys_.begin(), keys_.end());
        return false;
    }

    auto FindInsertPosition(const int& key) -> std::optional<int> {
        auto start = 0;
        auto end = keys_.size() - 1;
        while (start <= end) {
            auto mid = (start + end) / 2;
            if (*keys_[mid] == key)
                return {};
            else if (*keys_[mid] < key)
                start = mid + 1;
            else
                end = mid - 1;
        }
        return end + 1;
    }

    template <typename T>
    static auto SplitVec(std::vector<T>& vec) -> std::tuple<std::vector<T>, std::vector<T>> {
        auto mid = vec.size() % 2 == 0 ? vec.size() / 2 : vec.size() / 2 + 1;
        std::vector<T> low(vec.begin(), vec.begin() + mid);
        std::vector<T> high(vec.begin() + mid, vec.end());
        return std::make_tuple(low, high);
    }

    /**
     * Split a node into two halves and potentially create a new root (if the node being split was the previous root)
     */
    auto SplitNode(Node* current_root) -> std::tuple<Node*, Node*, Node*, int*> {
        std::lock_guard<std::mutex> latch{latch_};
        auto split_keys = SplitVec<int*>(keys_);
        auto left_half_keys = std::get<0>(split_keys);
        auto right_half_keys = std::get<1>(split_keys);
        auto promoted_key = left_half_keys.back();
        Node* root = nullptr;
        if (root_) {
            root = new Node(min_order_);
            auto new_root_keys = std::vector<int*>{promoted_key};
        }
        left_half_keys.pop_back();
        auto right = new Node(min_order_);
        right->SetKeys(right_half_keys);
        right->SetRightLink(right_link_);
        SetRightLink(right);
        SetKeys(left_half_keys);
        if (root != nullptr) {
            root->children_.push_back(this);
            root->children_.push_back(right);
        }
        if (leaf_) {
            current_root = root;
            return std::make_tuple(this, right, root, promoted_key);
        }
        auto split_children = SplitVec<void*>(children_);
        auto left_half_children = std::get<0>(split_children);
        auto right_half_children = std::get<1>(split_children);
        SetChildren(left_half_children);
        right->SetChildren(right_half_children);
        return std::make_tuple(this, right, root, promoted_key);
    }

    auto Scannode(const int& key) -> void* {
        auto index_option = FindInsertPosition(key);
        if (!index_option.has_value()) return nullptr;
        auto index = index_option.value();
        if (index == keys_.size() && right_link_ != nullptr) return right_link_;
        return children_[index];
    }

    /**
     * This function returns a latched node
     *
     */
    static auto MoveRight(Node* current, int& val) -> Node* {
        if (current == nullptr) return nullptr;
        current->Latch();
        void* t = current->Scannode(val);
        Node* target = nullptr;
        while (t == current->right_link_ && current->right_link_ != nullptr) {
            target = static_cast<Node*>(t);
            target->Latch();
            current->Unlatch();
            current = target;
        }
        return target;
    }

    auto IsSafe() -> bool {
        return min_order_ < keys_.size() && 2 * min_order_ > keys_.size();
    }

    void InsertSafe(int key, void* val) {
        auto key_insert_loc = FindInsertPosition(key).value();
        auto child_insert_loc = key_insert_loc;
        keys_.insert(keys_.begin() + key_insert_loc, &key);
        children_.insert(children_.begin() + child_insert_loc, val);
    }

   private:
    bool leaf_;
    bool root_;
    int min_order_;
    std::vector<int*> keys_;
    std::vector<void*> children_;
    std::mutex latch_;
    Node* right_link_;
    Node* out_link_;

    friend class Tree;
};

class Tree {
   public:
    explicit Tree(int min_order) : min_order_{min_order} {
        root_ = nullptr;
    }
    ~Tree() = default;

    auto Insert(const int& key, const int& val) -> bool {
        auto stack = std::stack<Node*>{};
        auto current = root_;
        if (current == nullptr) {
            current = new Node(min_order_);
            auto keys = std::vector<int*>{const_cast<int*>(&key)};
            auto children = std::vector<void*>{const_cast<int*>(&val)};
            current->SetKeys(keys);
            current->SetChildren(children);
            root_ = current;
            return true;
        }

        while (!current->IsLeaf() && current->Scannode(val) != nullptr) {
            auto t = current;
            current = static_cast<Node*>(t->Scannode(val));
            if (current != current->right_link_) stack.push(t);
        }
        auto res = Node::MoveRight(current, const_cast<int&>(val));
        if (res->FindInsertPosition(val).has_value()) {
            std::cout << "Unable to insert value " << val << " as it already exists in tree" << std::endl;
            return false;
        }
        return false;
    }

    auto Delete(const int& key) -> bool {
        return false;
    }

    auto Search() -> void* {
        return nullptr;
    }

   private:
    auto Insert(const int& key, const int& val, std::vector<int> stack) -> bool {
        return false;
    }
    Node* root_;
    int min_order_;
};

auto main() -> int {
    return 0;
}
