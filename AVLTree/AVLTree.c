#include "AVLTree.h"

// Comparing functions
int max (int a, int b) {
    return (a > b) ? a : b;
}


int avl_height(AVLNode * node) {
    if (!node) {
        return 0;
    }

    return node->height;
}

int avl_sub_tree_size(AVLNode * node) {
    if (!node) {
        return 0;
    }

    return node->sub_tree_size;
}

int avl_balance(AVLNode * node) {
    return avl_height(node->left) - avl_height(node->right);
}

void avl_update(AVLNode * node) {
    node->height = 1 + max(avl_height(node->left), avl_height(node->right));
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


AVLNode * avl_init(void * scnd_index, float value) {

    AVLNode * node = (AVLNode *) calloc(1, sizeof(AVLNode));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    node->height = 1;
    node->sub_tree_size = 1;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    node->scnd_index = scnd_index;
    node->value = value;

    return node;
}

AVLNode * avl_insert(AVLNode * tree, void * scnd_index, float value) {
    if (tree == NULL) {
        return avl_init(scnd_index, value);
    } else if (value < tree->value) {
        // if the value is less than the current node, insert it to the left
        tree->left = avl_insert(tree->left, scnd_index, value);
        tree->left->parent = tree;
    } else {
        // if the value is greater than or equal to the current node, insert it to the right
        tree->right = avl_insert(tree->right, scnd_index, value);
        tree->right->parent = tree;
    }

    // update the height and sub_tree_size of the current node
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

// delete a node by (float,scnd_index) pair 
AVLNode * avl_delete(AVLNode * tree,void * scnd_index, float value) {

    if (tree == NULL) {
        return NULL;
    }

    if (value < tree->value) {
        tree->left = avl_delete(tree->left, scnd_index, value);
    } else if (value > tree->value) {
        tree->right = avl_delete(tree->right, scnd_index, value);
    } else if (compare_scnd_index( scnd_index,tree->scnd_index) == 0 ) {
    // found the exact node to delete
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

            // copy the value and scnd_index of the inorder successor
            tree->value = temp->value;
            tree->scnd_index = temp->scnd_index;

            // delete the inorder successor
            tree->right = avl_delete(tree->right, temp->scnd_index, temp->value);
        }
    } else {
        // keep searching for the node to delete
        tree->left = avl_delete(tree->left, scnd_index, value);
        tree->right = avl_delete(tree->right, scnd_index, value);
    }

    // update the height and sub_tree_size of the current node
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


AVLNode * avl_search_float(AVLNode * tree, float value) {
    if (tree == NULL) {
        return NULL;
    }

    if (value < tree->value) {
        return avl_search_float(tree->left, value);
    } else if (value > tree->value) {
        return avl_search_float(tree->right, value);
    } else {
        return tree;
    }
}


AVLNode * avl_search_pair(AVLNode * tree, void * scnd_index, float value) {
    if (tree == NULL) {
        return NULL;
    }

    if (value < tree->value) {
        return avl_search_pair(tree->left, scnd_index, value);
    } else if (value > tree->value) {
        return avl_search_pair(tree->right, scnd_index, value);
    } else if (compare_scnd_index(scnd_index, tree->scnd_index) == 0) {
        return tree;
    } else {
        // look in both directions for the pair
        AVLNode * left = avl_search_pair(tree->left, scnd_index, value);
        AVLNode * right = avl_search_pair(tree->right, scnd_index, value);

        return (left != NULL) ? left : right;
    }

}   


// offset the rank of the node in the AVL tree by the value specified by the offset parameter
AVLNode * avl_offset(AVLNode * node, int offset) {
    int cur_position = 0;

    while(cur_position != offset) {
        if (cur_position < offset && cur_position + avl_sub_tree_size(node->right) >= offset) {
            // the target is inside the right subtree
            node = node->right;
            cur_position += avl_sub_tree_size(node->left) + 1;
        } else if (cur_position > offset && cur_position - avl_sub_tree_size(node->left) <= offset) {
            // the target is inside the left subtree
            node = node->left;
            cur_position -= avl_sub_tree_size(node->right) + 1;
        } else {
            // go to the parent
            AVLNode * parent = node->parent;

            if (!parent) {
                // if the parent is null, then the node is the root, and the offset is out of bounds
                return NULL;    
            }

            if (parent->left == node) {
                // if the node is the left child of the parent
                cur_position += avl_sub_tree_size(parent->right) + 1;
            } else {
                // if the node is the right child of the parent
                cur_position -= avl_sub_tree_size(parent->left) + 1;
            }

            node = parent;
        }


    }

    return node;
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
    printf("value: %f , balance: %d , snd_index: %p \n", tree->value , avl_balance(tree), tree->scnd_index);
    avl_print(tree->right);
}

        





