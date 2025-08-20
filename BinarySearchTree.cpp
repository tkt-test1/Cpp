// BinarySearchTree.cpp
//
// 概要:
//   二分探索木 (BST) の簡易実装
//
// 実装機能:
//   - 挿入 insert()
//   - 探索 search()
//   - 中順走査 inorder()

#include <iostream>
using namespace std;

struct Node {
    int key;
    Node* left;
    Node* right;
    Node(int k) : key(k), left(nullptr), right(nullptr) {}
};

class BST {
public:
    Node* root = nullptr;

    void insert(int key) {
        root = insertRec(root, key);
    }

    bool search(int key) {
        return searchRec(root, key);
    }

    void inorder() {
        inorderRec(root);
        cout << endl;
    }

private:
    Node* insertRec(Node* node, int key) {
        if (!node) return new Node(key);
        if (key < node->key) node->left = insertRec(node->left, key);
        else if (key > node->key) node->right = insertRec(node->right, key);
        return node;
    }

    bool searchRec(Node* node, int key) {
        if (!node) return false;
        if (key == node->key) return true;
        if (key < node->key) return searchRec(node->left, key);
        return searchRec(node->right, key);
    }

    void inorderRec(Node* node) {
        if (!node) return;
        inorderRec(node->left);
        cout << node->key << " ";
        inorderRec(node->right);
    }
};

int main() {
    BST tree;
    tree.insert(5);
    tree.insert(2);
    tree.insert(8);
    tree.insert(1);
    tree.insert(3);

    cout << "Inorder traversal: ";
    tree.inorder();

    cout << "Search 3: " << (tree.search(3) ? "Found" : "Not Found") << endl;
    cout << "Search 7: " << (tree.search(7) ? "Found" : "Not Found") << endl;
}
