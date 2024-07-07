#include "AVLTree.h"

// Currenly using recursion 



int max (int a, int b) {
    return (a > b) ? a : b;
}

int avl_depth(AVLNode * node) {
    if (!node) {
        return 0;
    }

    return node->depth;
}

int avl_sub_tree_size(AVLNode * node) {
    if (!node) {
        return 0;
    }

    return node->sub_tree_size;
}

int avl_balance(AVLNode * node) {
    return avl_depth(node->left) - avl_depth(node->right);
}

void avl_update(AVLNode * node) {
    node->depth = 1 + max(avl_depth(node->left), avl_depth(node->right));
    node->sub_tree_size = 1 + avl_sub_tree_size(node->left) + avl_sub_tree_size(node->right);

    return;
}

AVLNode * avl_rotate_left(AVLNode * node) {
    AVLNode * new_root = node->right;
    node->right = new_root->left;
    new_root->left = node;

    new_root->parent = node->parent;
    node->parent = new_root;

    if (node->right != NULL) {
        node->right->parent = node;
    }

    avl_update(node);
    avl_update(new_root);

    return new_root;
}


AVLNode * avl_rotate_right(AVLNode * node) {
    AVLNode * new_root = node->left;
    node->left = new_root->right;
    new_root->right = node;

    new_root->parent = node->parent;
    node->parent = new_root;

    if (node->left != NULL) {
        node->left->parent = node;
    }

    avl_update(node);
    avl_update(new_root);

    return new_root;
}       


AVLNode * avl_init(float value) {

    AVLNode * node = (AVLNode *) calloc(1, sizeof(AVLNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    node->depth = 1;
    node->sub_tree_size = 1;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    node->value = value;

    return node;
}

AVLNode * avl_insert(AVLNode * tree, float value) {
    if (tree == NULL) {
        // if the tree is empty, create a new node and return it
        return avl_init(value);
    } else if (value < tree->value) {
        // if the value is less than the current node, insert it to the left
        tree->left = avl_insert(tree->left,value);
        tree->left->parent = tree;
    } else {
        // if the value is greater than or equal to the current node, insert it to the right
        tree->right = avl_insert(tree->right,value);
        tree->right->parent = tree;
    }

    // update the depth and sub_tree_size of the current node
    avl_update(tree);       

    // check if the tree is balanced and balance, not since this is a recursive function, the tree will be balanced from the bottom up
    int balance = avl_balance(tree);
    
    
    // left heavy and extra child on the left
    if ((balance > 1) && (avl_balance(tree->left) >= 0)) {
        return avl_rotate_right(tree);
    }

    // right heavy and extra child on the right
    if ((balance < -1) && (avl_balance(tree->right) <= 0)) {
        return avl_rotate_left(tree);
    }   

    // left heavy and extra child on the right
    if ((balance > 1) && (avl_balance(tree->left) < 0)) {
        tree->left = avl_rotate_left(tree->left);
        return avl_rotate_right(tree);
    }

    // right heavy and extra child on the left 
    if ((balance < -1) && (avl_balance(tree->right) > 0)) {
        tree->right = avl_rotate_right(tree->right);
        return avl_rotate_left(tree);
    }

    return tree;
}

AVLNode * get_min_node(AVLNode * tree) {
    AVLNode * current = tree;
    while (current->left != NULL) {
        current = current->left;
    }
    return current;
}

AVLNode * avl_delete(AVLNode * tree, float value) {

    if (tree == NULL) {
        return NULL;
    }

    if (value < tree->value) {
        tree->left = avl_delete(tree->left, value);
    } else if (value > tree->value) {
        tree->right = avl_delete(tree->right, value);
    } else {
        if (tree->left == NULL) {
            AVLNode * temp = tree->right;

            // update the parent of the right child
            if (temp != NULL) {
                temp->parent = tree->parent;
            }
            
            // free the node
            free(tree);
            return temp;
        } else if (tree->right == NULL) {
            AVLNode * temp = tree->left;

            // update the parent of the left child
            if (temp != NULL) {
                temp->parent = tree->parent;
            }

            // free the node
            free(tree);
            return temp;
        } else {
            // need to find the inorder successor
            AVLNode * temp = get_min_node(tree->right);

            // copy the value of the inorder successor to the current node
            tree->value = temp->value;

            // delete the inorder successor
            tree->right = avl_delete(tree->right, temp->value);
        }

    }

    // update the depth and sub_tree_size of the current node
    avl_update(tree);

    // balance the tree
    int balance = avl_balance(tree);

    // left heavy and extra child on the left
    if ((balance > 1) && (avl_balance(tree->left) >= 0)) {
        return avl_rotate_right(tree);
    }

    // right heavy and extra child on the right
    if ((balance < -1) && (avl_balance(tree->right) <= 0)) {
        return avl_rotate_left(tree);
    }   

    // left heavy and extra child on the right
    if ((balance > 1) && (avl_balance(tree->left) < 0)) {
        tree->left = avl_rotate_left(tree->left);
        return avl_rotate_right(tree);
    }

    // right heavy and extra child on the left 
    if ((balance < -1) && (avl_balance(tree->right) > 0)) {
        tree->right = avl_rotate_right(tree->right);
        return avl_rotate_left(tree);
    }

    return tree;
}

AVLNode * avl_search(AVLNode * tree, float value) {
    if (tree == NULL) {
        return NULL;
    }

    if (value < tree->value) {
        return avl_search(tree->left, value);
    } else if (value > tree->value) {
        return avl_search(tree->right, value);
    } else {
        return tree;
    }
}

// free the tree
void avl_free(AVLNode * tree) {
    if (tree == NULL) {
        return;
    }

    avl_free(tree->left);
    avl_free(tree->right);

    free(tree);
}

// print inoder traversal, which is sorted
void avl_print(AVLNode * tree) {
    if (tree == NULL) {
        return;
    }

    avl_print(tree->left);
    printf("%f\n", tree->value);
    avl_print(tree->right);
}









        





