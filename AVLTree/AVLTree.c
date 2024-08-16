// * This file contains the implementation of the AVL tree data structure. The snc_index is a void pointer that represents some secondary key for each AVLNode, define the compare_scnd_index function to compare two secondary indexes in the file where you are using this AVLTree.

#include "AVLTree.h"

// Comparing functions
int max(int a, int b)
{
    return (a > b) ? a : b;
}

int avl_height(AVLNode *node)
{
    if (!node)
    {
        return 0;
    }

    return node->height;
}

int avl_sub_tree_size(AVLNode *node)
{
    if (!node)
    {
        return 0;
    }
    return node->sub_tree_size;
}

/**
 * @brief Return the balance factor of the node
 *
 * @param node The node to calculate the balance factor
 *
 * @return int The balance factor of the node
 */
int avl_balance(AVLNode *node)
{
    return avl_height(node->left) - avl_height(node->right);
}

/**
 * @brief Update the height and sub_tree_size of the node
 *
 * @param node The node to update
 *
 * @return void
 */
void avl_update(AVLNode *node)
{
    node->height = 1 + max(avl_height(node->left), avl_height(node->right));
    node->sub_tree_size = 1 + avl_sub_tree_size(node->left) + avl_sub_tree_size(node->right);

    return;
}

/**
 * @brief Rotate the node to the left
 *
 * @param node The node to rotate
 *
 * @return AVLNode* The new root of the subtree
 */
AVLNode *avl_rotate_left(AVLNode *node)
{
    AVLNode *new_root = node->right;
    node->right = new_root->left;
    new_root->left = node;

    new_root->parent = node->parent;
    node->parent = new_root;

    if (node->right != NULL)
    {
        node->right->parent = node;
    }

    avl_update(node);
    avl_update(new_root);

    return new_root;
}

/**
 * @brief Rotate the node to the right
 *
 * @param node The node to rotate
 *
 * @return AVLNode* The new root of the subtree
 */
AVLNode *avl_rotate_right(AVLNode *node)
{
    AVLNode *new_root = node->left;
    node->left = new_root->right;
    new_root->right = node;

    new_root->parent = node->parent;
    node->parent = new_root;

    if (node->left != NULL)
    {
        node->left->parent = node;
    }

    avl_update(node);
    avl_update(new_root);

    return new_root;
}

/**
 * @brief Initialize a new AVLNode
 *
 * Initialize a new AVLNode with the secondary index and value. The secondary index is duplicated using strdup to avoid memory issues.
 *
 * @param scnd_index The secondary index of the node
 * @param value The value of the node
 *
 * @return AVLNode* The new AVLNode
 */
AVLNode *avl_init(void *scnd_index, float value)
{

    AVLNode *node = (AVLNode *)calloc(1, sizeof(AVLNode));
    if (node == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    node->height = 1;
    node->sub_tree_size = 1;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    node->scnd_index = strdup(scnd_index);
    node->value = value;

    return node;
}

/**
 * @brief Insert a new node into the AVL tree
 *
 * This function inserts a new node into the AVL tree. If the value is less than the current node, it is inserted to the left, otherwise to the right. The tree is then balanced and returned.
 *
 * @param tree The AVL tree to insert into
 * @param scnd_index The secondary index of the node
 * @param value The value of the node
 *
 * @return AVLNode* The new root of the AVL tree
 */
AVLNode *avl_insert(AVLNode *tree, void *scnd_index, float value)
{
    if (tree == NULL)
    {
        return avl_init(scnd_index, value);
    }
    else if (value < tree->value)
    {
        // if the value is less than the current node, insert it to the left
        tree->left = avl_insert(tree->left, scnd_index, value);
        tree->left->parent = tree;
    }
    else
    {
        // if the value is greater than or equal to the current node, insert it to the right
        tree->right = avl_insert(tree->right, scnd_index, value);
        tree->right->parent = tree;
    }

    // update the height and sub_tree_size of the current node
    avl_update(tree);

    // check if the tree is balanced and balance, not since this is a recursive function, the tree will be balanced from the bottom up
    int balance = avl_balance(tree);

    // left heavy and extra child on the left
    if ((balance > 1) && (avl_balance(tree->left) >= 0))
    {
        return avl_rotate_right(tree);
    }

    // right heavy and extra child on the right
    if ((balance < -1) && (avl_balance(tree->right) <= 0))
    {
        return avl_rotate_left(tree);
    }

    // left heavy and extra child on the right
    if ((balance > 1) && (avl_balance(tree->left) < 0))
    {
        tree->left = avl_rotate_left(tree->left);
        return avl_rotate_right(tree);
    }

    // right heavy and extra child on the left
    if ((balance < -1) && (avl_balance(tree->right) > 0))
    {
        tree->right = avl_rotate_right(tree->right);
        return avl_rotate_left(tree);
    }

    return tree;
}

/**
 * @brief Get the node with the minimum value in the tree
 *
 * This function returns the node with the minimum value in the tree. It is used to find the inorder successor of a node when deleting a node with two children.
 *
 * @param tree The AVL tree to search
 *
 * @return AVLNode* The node with the minimum value
 */
AVLNode *get_min_node(AVLNode *tree)
{
    AVLNode *current = tree;
    if (current == NULL)
    {
        return NULL;
    }

    while (current->left != NULL)
    {
        current = current->left;
    }
    return current;
}

/**
 * @brief Delete a node from the AVL tree
 *
 * This function deletes a node from the AVL tree. If the node has no children, it is simply removed. If the node has one child, the child is moved up to replace the node. If the node has two children, the inorder successor is found, and its value and secondary index are copied to the node, and the inorder successor is deleted. It frees the secondary index of the node.
 *
 * @param tree The AVL tree to delete from
 * @param scnd_index The secondary index of the node to delete
 * @param value The value of the node to delete
 *
 * @return AVLNode* The new root of the AVL tree
 */
AVLNode *avl_delete(AVLNode *tree, void *scnd_index, float value)
{

    if (tree == NULL)
    {
        return NULL;
    }

    if (value < tree->value)
    {
        tree->left = avl_delete(tree->left, scnd_index, value);
    }
    else if (value > tree->value)
    {
        tree->right = avl_delete(tree->right, scnd_index, value);
    }
    else if (compare_scnd_index(scnd_index, tree->scnd_index) == 0)
    {
        // found the exact node to delete
        if (tree->left == NULL)
        {
            AVLNode *temp = tree->right;

            // update the parent of the right child
            if (temp != NULL)
            {
                temp->parent = tree->parent;
            }

            // free the node
            free(tree->scnd_index);
            free(tree);
            return temp;
        }
        else if (tree->right == NULL)
        {
            AVLNode *temp = tree->left;

            // update the parent of the left child
            if (temp != NULL)
            {
                temp->parent = tree->parent;
            }

            // free the node
            free(tree->scnd_index);
            free(tree);
            return temp;
        }
        else
        {
            // need to find the inorder successor
            AVLNode *temp = get_min_node(tree->right);

            // free the secondary index of the current node
            free(tree->scnd_index);

            // copy the value and scnd_index of the inorder successor
            tree->value = temp->value;
            tree->scnd_index = strdup((char *)temp->scnd_index);

            // delete the inorder successor
            tree->right = avl_delete(tree->right, temp->scnd_index, temp->value);
        }
    }
    else
    {
        tree->right = avl_delete(tree->right, scnd_index, value);
        tree->left = avl_delete(tree->left, scnd_index, value);
    }

    // update the height and sub_tree_size of the current node
    avl_update(tree);

    // balance the tree
    int balance = avl_balance(tree);

    // left heavy and extra child on the left
    if ((balance > 1) && (avl_balance(tree->left) >= 0))
    {
        return avl_rotate_right(tree);
    }

    // right heavy and extra child on the right
    if ((balance < -1) && (avl_balance(tree->right) <= 0))
    {
        return avl_rotate_left(tree);
    }

    // left heavy and extra child on the right
    if ((balance > 1) && (avl_balance(tree->left) < 0))
    {
        tree->left = avl_rotate_left(tree->left);
        return avl_rotate_right(tree);
    }

    // right heavy and extra child on the left
    if ((balance < -1) && (avl_balance(tree->right) > 0))
    {
        tree->right = avl_rotate_right(tree->right);
        return avl_rotate_left(tree);
    }

    return tree;
}

/**
 * @brief Search for a node with a specific float value in the AVL tree
 *
 * This function searches for a node with a specific value in the AVL tree. If the value is less than the current node, it searches the left subtree, if the value is greater, it searches the right subtree. If the value is equal to the current node, it returns the node.
 *
 * @param tree The AVL tree to search
 * @param value The value to search for
 *
 * @return AVLNode* The node with the specified value
 */
AVLNode *avl_search_float(AVLNode *tree, float value)
{
    if (tree == NULL)
    {
        return NULL;
    }

    if (value < tree->value)
    {
        return avl_search_float(tree->left, value);
    }
    else if (value > tree->value)
    {
        return avl_search_float(tree->right, value);
    }
    else
    {
        return tree;
    }
}

/**
 * @brief Search for a node with a specific float value and secondary index in the AVL tree
 *
 * This function searches for a node with a specific value and secondary index in the AVL tree. If the value is less than the current node, it searches the left subtree, if the value is greater, it searches the right subtree. If the value is equal to the current node, it compares the secondary index. If the secondary index is equal, it returns the node. If the secondary index is not equal, it searches in both directions for the pair.
 *
 * @param tree The AVL tree to search
 * @param scnd_index The secondary index to search for
 * @param value The value to search for
 *
 * @return AVLNode* The node with the specified value and secondary index
 */
AVLNode *avl_search_pair(AVLNode *tree, void *scnd_index, float value)
{
    if (tree == NULL)
    {
        return NULL;
    }

    if (value < tree->value)
    {
        return avl_search_pair(tree->left, scnd_index, value);
    }
    else if (value > tree->value)
    {
        return avl_search_pair(tree->right, scnd_index, value);
    }
    else if (compare_scnd_index(scnd_index, tree->scnd_index) == 0)
    {
        return tree;
    }
    else
    {
        // look in both directions for the pair
        AVLNode *left = avl_search_pair(tree->left, scnd_index, value);
        AVLNode *right = avl_search_pair(tree->right, scnd_index, value);

        return (left != NULL) ? left : right;
    }
}

/**
 * @brief Get the node at a specific offset in the AVL tree
 *
 * This function returns the node at a specific offset in the AVL tree. It traverses the tree to find the node at the specified offset. If the offset is out of bounds, it returns NULL.
 *
 * @param node The root of the AVL tree
 * @param offset The offset to search for
 *
 * @return AVLNode* The node at the specified offset
 */
AVLNode *avl_offset(AVLNode *node, int offset)
{
    int cur_position = 0;

    while (cur_position != offset)
    {
        if (cur_position < offset && cur_position + avl_sub_tree_size(node->right) >= offset)
        {
            // the target is inside the right subtree
            node = node->right;
            cur_position += avl_sub_tree_size(node->left) + 1;
        }
        else if (cur_position > offset && cur_position - avl_sub_tree_size(node->left) <= offset)
        {
            // the target is inside the left subtree
            node = node->left;
            cur_position -= avl_sub_tree_size(node->right) + 1;
        }
        else
        {
            // go to the parent
            AVLNode *parent = node->parent;

            if (!parent)
            {
                // if the parent is null, then the node is the root, and the offset is out of bounds
                return NULL;
            }

            if (parent->left == node)
            {
                // if the node is the left child of the parent
                cur_position += avl_sub_tree_size(node->right) + 1;
            }
            else
            {
                // if the node is the right child of the parent
                cur_position -= avl_sub_tree_size(node->left) + 1;
            }

            node = parent;
        }
    }

    return node;
}

/**
 * @brief Free the AVL tree
 *
 * This function frees the AVL tree and all its contents. It recursively frees the left and right subtrees before freeing the current node.
 *
 * @param tree The AVL tree to free
 *
 * @return void
 */
void avl_free(AVLNode *tree)
{
    if (tree == NULL)
    {
        return;
    }

    avl_free(tree->left);
    avl_free(tree->right);

    free(tree->scnd_index);
    free(tree);
}

/**
 * @brief Print the AVL tree
 *
 * This function prints the AVL tree in inorder traversal, which is sorted. It prints the value, balance factor, and secondary index of each node.
 *
 * @param tree The AVL tree to print
 *
 * @return void
 */
void avl_print(AVLNode *tree)
{
    if (tree == NULL)
    {
        return;
    }

    avl_print(tree->left);
    printf("value: %f , balance: %d , snd_index: %s \n", tree->value, avl_balance(tree), (char *)tree->scnd_index);
    avl_print(tree->right);
}
