//#include "binaryTree.h"

template<typename type>
BinaryTree<type>::BinaryTree(){
	root.left=NULL;
	root.right=NULL;
	root.endNode=false;
	curt=&root;
}

template<typename type>
void BinaryTree<type>::__binaryTreeDelete(BinaryTree<type>::NODE* nn){
	// recursivly delete the nodes

	if(nn->right != NULL)
		__binaryTreeDelete(nn->right);
	if(nn->left !=NULL)
		__binaryTreeDelete(nn->left);

	// finally delete this node
	delete nn;
}
template<typename type>
BinaryTree<type>::~BinaryTree(){
	if(root.left!=NULL)
		__binaryTreeDelete(root.left);
	if(root.right!=NULL)
		__binaryTreeDelete(root.right);
}

template<typename type>
BinaryTree<type>& BinaryTree<type>::left(){
	if(curt->left!=NULL){
		curt=curt->left;
	}
	return *this;
}
template<typename type>
BinaryTree<type>& BinaryTree<type>::right(){
	if(curt->right!=NULL){
		curt=curt->right;
	}
	return *this;
}

template<typename type>
bool BinaryTree<type>::atEndNode(){
	return curt->endNode;
}

template<typename type>
BinaryTree<type>& BinaryTree<type>::sequence(int sequence, int length){
	for(int x=length-1;x>=0;x--){

		if(((sequence>>x) & 1) == 0){
			if(curt->left==NULL) return *this; // dont go past the valid tree parts
			curt=curt->left;
		}else{
			if(curt->right==NULL) return *this; // dont go past the valid tree parts
			curt=curt->right;
		}
	}
	return *this;
}

template<typename type>
void BinaryTree<type>::reset(){
	curt=&root;
}

template<typename type>
void BinaryTree<type>::addNode(int sequence,int length,type data){
	this->reset();
	for(int x=length-1;x>=0;x--){

		if(((sequence>>x) & 1) == 0){
			if(curt->left==NULL){
				curt->left = new NODE;
				curt->left->left=NULL;
				curt->left->right=NULL;
				curt->left->endNode=false;
			}
			curt=curt->left;
		}else{
			if(curt->right==NULL){
				curt->right = new NODE;
				curt->right->left=NULL;
				curt->right->right=NULL;
				curt->right->endNode=false;
			}
			curt=curt->right;
		}
	}

	//curt now points to the end node we have made
	curt->data = data;
	curt->endNode = true;
}

template<typename type>
type& BinaryTree<type>::data(){
	return curt->data;
}