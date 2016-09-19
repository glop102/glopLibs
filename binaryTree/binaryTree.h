#ifndef GLOP_BINARYTREE
#define GLOP_BINARYTREE

#include <stdlib.h>

template<typename type>
class BinaryTree{
public:
	struct NODE{
		NODE* left;
		NODE* right;
		bool endNode;
		type data;
	};
	NODE root;
	NODE* curt;

	BinaryTree();
	~BinaryTree();
	void __binaryTreeDelete(BinaryTree<type>::NODE* nn);

	BinaryTree& left();
	BinaryTree& right();
	bool atEndNode(); // is this a leaf of the tree?
	BinaryTree& sequence(int sequence, int length); // follow this sequence from the curt node
	void reset(); // go back to the root

	void addNode(int sequence,int length,type data);
	type& data(); // get the data from teh current node
};

#include "binaryTree.cpp"

#endif