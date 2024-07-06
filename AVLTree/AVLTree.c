#include "AVLTree.h"

void avl_init(AVLNode * node) {
    node->depth = 1;
    node->sub_tree_size = 1;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
}

int avl_balance(AVLNode * node) {
    return node->left->depth - node->right->depth;
}

void avl_update(AVLNode * node) {
    node->depth = 1 + (node->left->depth > node->right->depth ? node->left->depth : node->right->depth);
    node->sub_tree_size = 1 + (node->left->sub_tree_size + node->right->sub_tree_size);
}


AVLNode * ll_rotate(AVLNode * node) {
    AVLNode * new_root = node->left;
    node->left = new_root->right;
    new_root->right = node;

    avl_update(node);
    avl_update(new_root);

    return new_root;
}

AVLNode * rr_rotate(AVLNode * node) {
    AVLNode * new_root = node->right;
    node->right = new_root->left;
    new_root->left = node;

    avl_update(node);
    avl_update(new_root);

    return new_root;
}   

AVLNode * lr_rotate(AVLNode * node) {
    node->left = rr_rotate(node->left);
    return ll_rotate(node);
}

AVLNode * rl_rotate(AVLNode * node) {
    node->right = ll_rotate(node->right);
    return rr_rotate(node);
}
