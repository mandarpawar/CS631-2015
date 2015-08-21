/*
 * BPlus Tree Code
 *
 * Originally developed by:
 * Sandeep Joshi 113050022
 * Nikhil Patil 113059004
 * github: git://github.com/nikpatil/BPlusTree.git  or git://github.com/sandeeppjoshi/BPlusTree.git
 *
 * Modified for CS 631 Assignment-1
 */

#include "FileHandler.h"
#include "LookupIter.h"
#include <math.h>

#include<set>

using namespace std;

/*----------------------------------------------------------------------------------------------------------------------------
 * Index class contains main functions like lookup and insert.
 * Data Representation:
 * Node contains keys and payload arranged as follows:
 * Header,keys,payload
 * Keys and payload is arraged such that there is free space in the middle.This is done to avoid extra shifting.
 */

class Index {
public:
    FileHandler *fHandler;
    TreeNode *root;
    char rootAddress[NODE_OFFSET_SIZE];
    KeyType keytype;
    int payloadlen;
    char *header;
    int node_address_size;

    ~Index() {

    }
    /*
     *Construnctor for new Index
     */
    Index(char* indexName, KeyType *keytype, int payloadlen) {
        fHandler = new FileHandler(indexName);
        this->keytype.numAttrs = keytype->numAttrs;
        for (int i = 0; i < keytype->numAttrs; i++) {
            this->keytype.attrTypes[i] = keytype->attrTypes[i];
            this->keytype.attrLen[i] = keytype->attrLen[i];
        }
        this->payloadlen = payloadlen;
        header = (char *) malloc(BLOCK_SIZE);
        Utils::copyBytes(&header[NODE_OFFSET_SIZE],
                         Utils::getBytesForInt(payloadlen), sizeof(int));
        Utils::copyBytes(&header[NODE_OFFSET_SIZE + sizeof(payloadlen)],
                         Utils::getBytesForKeyType(this->keytype), sizeof(KeyType));
    }

    /*
     * Constructor to load already existing index
     */
    Index(char* indexName) {
        fHandler = new FileHandler(indexName, 'o');
        header = (char *) malloc(BLOCK_SIZE);
        fHandler->readBlock(0, header);
        Utils::copyBytes(rootAddress, header, NODE_OFFSET_SIZE);
        root = new TreeNode();
        loadNode(root, rootAddress);
        payloadlen = Utils::getIntForBytes(&header[NODE_OFFSET_SIZE]);
        keytype = Utils::getKeyTypeForBytes(
                &header[NODE_OFFSET_SIZE + sizeof(payloadlen)]);
    }

    /*
     * This stores the node on the disk. If the offset passed is -1 node will be stored at the end of the file.
     * This indicated newly added node.
     */
    int storeNode(TreeNode *node, long long int offset) {
        if (offset == -1) {
            offset = fHandler->getSize();

        }
        Utils::copyBytes(node->myaddr, Utils::getBytesForInt(offset), NODE_OFFSET_SIZE);
        char *block = (char *) malloc(BLOCK_SIZE);
        int position = 0;
        Utils::copyBytes(&block[position], Utils::getBytesForInt(offset),
                         NODE_OFFSET_SIZE);
        position += NODE_OFFSET_SIZE;
        block[position] = node->flag;
        position += 1;
        Utils::copyBytes(&block[position], Utils::getBytesForInt(node->numkeys),
                         sizeof(node->numkeys));
        position += sizeof(node->numkeys);
        Utils::copyBytes(&block[position], (node->data), sizeof(node->data));
        fHandler->writeBlock(offset, block);
        free(block);
        return 0;
    }
    /*
     * This loads the node.We are using an in memory structure for node.Following function generates the node from the block read.
     */
    int loadNode(TreeNode *here, char *offset) {
        int position = 0;
        char *block = (char *) calloc(BLOCK_SIZE, BLOCK_SIZE);

        fHandler->readBlock(Utils::getIntForBytes(offset), block);
        Utils::copyBytes(here->myaddr, offset, NODE_OFFSET_SIZE);
        position += NODE_OFFSET_SIZE;

        here->flag = block[position];
        position += 1;
        here->numkeys = Utils::getIntForBytes(&(block[position]));
        position += sizeof(here->numkeys);
        Utils::copyBytes(here->data, &(block[position]), sizeof(here->data));

        free(block);
        return 0;
    }

    /*
     * Main insert function to insert the node in tree.
     */
    int insert(char key[], char payload[]) {
        if (root == 0) {
            addFirstElement(key, payload);
            return 0;
        }

        TreeNode * current = new TreeNode();
        loadNode(current, rootAddress);

        char nodekey[keylen(&keytype)];
        char accessPath[MAX_TREE_HEIGHT][NODE_OFFSET_SIZE];
        int height = 0, i;
        int isLesser;

        while (current != 0) {
            cout<<"Before Insertion:"<< (long int)(*key)<<endl;
            current->display(keytype);
            Utils::copyBytes((accessPath[height++]), (current->myaddr), NODE_OFFSET_SIZE);
            for (i = 0; i < current->numkeys; i++) {
                current->getKey(keytype, nodekey, i);
                isLesser = compare(nodekey, key, keytype);
                if (isLesser >= 0) {
                    //cout<<"IsLesser:"<<isLesser;
                    break;
                }
            }

            if (current->flag == 'c'){
                if(splitNecessary(current->numkeys + 1, 'c') != 1){
                    addToLeafNoSplit(key, payload, i, &current);
                }
                else{       //Here split is necessary.
                    /*
                      * TODO: Fill in the required code.
                      * Hint: You will need to use the function handleLeafSplit
                      */
                    //call handleLeafSplit function..
                       //int handleLeafSplit(byte key[], byte payload[], TreeNode **rcvd_node,
                                       //char accessPath[][NODE_OFFSET_SIZE], int height, int insertPos, int splitPos){
                    if(isLesser == 0)   //duplicate exists
                    {
                        cout<<"Split: Duplicate value was supposed to be inserted"<<endl;

                        int insertpos = i;
                        cout<<"split: insert position:"<<i<<endl;
                        int splitpos = i;
                        for( ; i < current->numkeys ; i++)
                        {
                            current->getKey(keytype, nodekey, i);
                            isLesser = compare(nodekey, key, keytype);
                            if (isLesser > 0) {
                                //cout<<"IsLesser:"<<isLesser;
                                break;
                            }
                        }

                        if( i < current->numkeys)               // That is at least one element bigger than k exists in the node
                        {
                            cout<<"split:duplicate value: bigger element ends the node"<<endl;
                            splitpos = i+1;     //This is because we are adding one element before.
                            handleLeafSplit( key , payload , &current , accessPath , height , insertpos , splitpos );
                        }
                        else if(i == current->numkeys)          // This is when array ends with k.
                        {
                            if(insertpos == 0)                  //Arrary starts and ends with k.
                            {
                                //ignore the insert key.
                                cout<<"Alert: value dropped as it has reached maximum limit"<<endl;
                                current = 0;
                            }
                            else
                            {
                                splitpos = insertpos;
                                handleLeafSplit( key , payload , &current , accessPath , height , insertpos , splitpos );
                            }

                        }
                        // i > numkeys is not possible.



                        //Algorithm: split the node when duplicate starts..
                        //Nothing else..
                        /*if(i!=0)        //Split only when this current node is sole node
                            handleLeafSplit( key , payload , &current , accessPath , height , i , i );
                        else
                        {
                            //i is zero, that means input key is duplicate and is equal to first key of the node.
                            //So we split the node after the first set of equal keys finish.
                            for( ; i < current->numkeys ; i++)
                            {
                                current->getKey(keytype, nodekey, i);
                                isLesser = compare(nodekey, key, keytype);
                                if (isLesser > 0) {
                                    //cout<<"IsLesser:"<<isLesser;
                                    break;
                                }
                            }
                            if(i != current->numkeys)
                            {//insert@ith position and split @ (i+1)th position
                                handleLeafSplit( key , payload , &current , accessPath , height , i , i+1 );
                            }
                            else
                            {
                                //This is the case where node is full with same values.. No room to put new value.. so ignore it..
                                current = 0;
                            }

                            //current = 0;        //This will just ignore the insert key.
                        }*/

                        //current=0;        //now handleLeafSplit will do this.
                    }
                    else                //normal case..New key is not duplicate..
                    {
//                        cout<<"Handle split:"<< key <<","<< payload <<","<< height<<","<< i<<","<<current->numkeys<<","<< ceil(((current->numkeys)+1)/2) <<endl;
                        //handleLeafSplit( key , payload , &current , accessPath , height , i , ceil(((current->numkeys)+1)/2.0) );
                        int insertat = i;
                        int splitpostition = i;
                        if(splitpostition == 0)
                            splitpostition++;
                        handleLeafSplit( key , payload , &current , accessPath , height , insertat , splitpostition );
                    }

                       //cout<<"Handle split:"<< key <<","<< payload <<","<< height<<","<< i<<","<<current->numkeys<<","<< ceil(((current->numkeys)+1)/2) <<endl;
                       //handleLeafSplit( key , payload , &current , accessPath , height , i , ceil(((current->numkeys)+1)/2.0) );
                }
            }
            else
            {
                if(isLesser == 0)           //THis is to handle equal to case, in which next pointer is to be selected.
                    i++;
                handleNonLeaf(&current, i);
            }
        }

        loadNode(root, rootAddress);
        return 0;

    }

    /*
	 * This is to handle first addition. First addition needs to update header of the file with nodes address
	 */
    int addFirstElement(byte *key, byte *payload) {
        root = new TreeNode();
        root->numkeys = 0;
        root->flag = 'c';
        root->addData(keytype, key, payloadlen, payload, 0);
        root->numkeys = 1;
        Utils::copyBytes(header, Utils::getBytesForInt((long long int) 1), NODE_OFFSET_SIZE);
        Utils::copyBytes(rootAddress, Utils::getBytesForInt((long long int) 1), NODE_OFFSET_SIZE);
        fHandler->writeBlock(0, header);
        storeNode(root, 1);             //Root node is stored at offset 1...Offset zero is header block...
        return 0;
    }
    /*
	 * Get the next node to load
	 */
    int handleNonLeaf(TreeNode **rcvd_node, int position) {
        TreeNode *node = *rcvd_node;
        char *nextNodeAddress;
        nextNodeAddress = (char *) malloc(NODE_OFFSET_SIZE);
        node->getPayload(NODE_OFFSET_SIZE, nextNodeAddress, position);
        loadNode(*rcvd_node, nextNodeAddress);

        free(nextNodeAddress);
        return 0;
    }

    int addToLeafNoSplit(byte *key, byte *payload, int position, TreeNode **rcvd_node) {
        TreeNode *node = *rcvd_node;

        node->addData(keytype, key, payloadlen, payload, position);
        node->numkeys = node->numkeys + 1;
        storeNode(node, Utils::getIntForBytes(node->myaddr));

        *rcvd_node = 0;
        return 0;
    }

    int handleLeafSplit(byte key[], byte payload[], TreeNode **rcvd_node,
                        char accessPath[][NODE_OFFSET_SIZE], int height, int insertPos, int splitPos){
        TreeNode *node = *rcvd_node;
        TreeNode *newLeaf = new TreeNode();
        /*
         * Temporary space to hold the node that will result on addition of new data
         */
        int tempSpaceSize = DATA_SIZE + payloadlen + keylen(&keytype);      //Existing data size + key + value(payload) pair
        char *tempSpace = (char*) calloc(tempSpaceSize, sizeof(char));

        copyToTemp(key, payload, insertPos, node, tempSpaceSize, tempSpace);

        node->numkeys = node->numkeys + 1;

        doSplit(node, newLeaf, tempSpaceSize, tempSpace, splitPos);

        newLeaf->flag = 'c';
        newLeaf->numkeys = node->numkeys - splitPos;
        node->numkeys = splitPos;

        /*
         * Get the parent to add pointers to. accessPath has list of all nodes accessed till now. This array gives us the pareent
         */
        TreeNode* parent = new TreeNode();
        for (int i = 0; i < height; i++) {
            if (strncmp(accessPath[i], (node->myaddr), NODE_OFFSET_SIZE) == 0) {
                if (i != 0)
                    loadNode(parent, accessPath[i - 1]);
            }
        }

        char nextKey[keylen(&keytype)];

        newLeaf->getKey(keytype, nextKey, 0);
        storeNode(node, Utils::getIntForBytes(node->myaddr));
        storeNode(newLeaf, -1);
        char left[NODE_OFFSET_SIZE];
        Utils::copyBytes(left, node->myaddr, NODE_OFFSET_SIZE);
        char right[NODE_OFFSET_SIZE];
        Utils::copyBytes(right, newLeaf->myaddr, NODE_OFFSET_SIZE);
        char parentAdd[NODE_OFFSET_SIZE];
        Utils::copyBytes(parentAdd, parent->myaddr, NODE_OFFSET_SIZE);
        if (node != root)
            delete (node);
        delete (newLeaf);
        delete (parent);
        insertIntoParent(left, nextKey, right, parentAdd, height,
                         accessPath);

        *rcvd_node = 0;
        return 0;
    }

    int doSplit(TreeNode *node, TreeNode *newLeaf, int tempSpaceSize, char *tempSpace, int splitPos) {
        for (int i = 0; i < splitPos; i++) {
            Utils::copyBytes(&(node->data[(i) * keylen(&keytype)]),&(tempSpace[(i * keylen(&keytype))]), keylen(&keytype));
            Utils::copyBytes(&(node->data[DATA_SIZE - ((i + 1)) * payloadlen]),&(tempSpace[tempSpaceSize - ((i + 1) *payloadlen)]),payloadlen);
        }

        for (int i = splitPos; i < node->numkeys; i++) {
            Utils::copyBytes(&(newLeaf->data[(i - splitPos) * keylen(&keytype)]),&(tempSpace[(i * keylen(&keytype))]), keylen(&keytype));
            Utils::copyBytes(&(newLeaf->data[DATA_SIZE - ((i + 1) - splitPos) * payloadlen]),&(tempSpace[tempSpaceSize - ((i + 1) * payloadlen)]),payloadlen);
        }

        return 0;
    }

    int copyToTemp(byte *key, byte *payload, int position, TreeNode *node, int tempSpaceSize,
                   char *tempSpace) {
        Utils::copyBytes(tempSpace, node->data,(node->numkeys) * keylen(&keytype));
        Utils::copyBytes(&tempSpace[tempSpaceSize - (node->numkeys) * payloadlen],&(node->data[DATA_SIZE - (node->numkeys) * payloadlen]),(node->numkeys) *
                                                                                                                                          payloadlen);

        for (int j = node->numkeys - 1; j >= (position); j--) {
            Utils::copyBytes(&(tempSpace[(j + 1) * keylen(&keytype)]), &(tempSpace[j * keylen(&keytype)]), keylen(&keytype));
        }
        strncpy(&(tempSpace[(position) * keylen(&keytype)]), key, keylen(&keytype));

        for (int j = (tempSpaceSize - node->numkeys * payloadlen);j < (tempSpaceSize - position *
                                                                                       payloadlen); j += payloadlen) {
            Utils::copyBytes(&(tempSpace[j - payloadlen]), &(tempSpace[j]), payloadlen);
        }
        strncpy(&(tempSpace[tempSpaceSize - (position + 1) * payloadlen]),payload, payloadlen);

        return 0;
    }

    /*
     * This inserts pointers into node.May result in node split which is also handled.This results in recursive call to add pointers
     * to the parents up the tree till root
     */
    int insertIntoParent(byte left[NODE_OFFSET_SIZE], byte key[],
                         byte right[NODE_OFFSET_SIZE], byte parentOffset[NODE_OFFSET_SIZE],
                         int height, char accessPath[][NODE_OFFSET_SIZE]) {
        if (strncmp(rootAddress, left, NODE_OFFSET_SIZE) == 0) {
            TreeNode *newRoot = new TreeNode();
            newRoot->numkeys = 1;
            newRoot->flag = 'n';
            Utils::copyBytes(newRoot->data, key, keylen(&keytype));
            Utils::copyBytes(&(newRoot->data[DATA_SIZE - NODE_OFFSET_SIZE]),
                             left, NODE_OFFSET_SIZE);
            Utils::copyBytes(&(newRoot->data[DATA_SIZE - NODE_OFFSET_SIZE * 2]),
                             right, NODE_OFFSET_SIZE);
            root = newRoot;
            storeNode(newRoot, -1);
            Utils::copyBytes(rootAddress, root->myaddr, NODE_OFFSET_SIZE);

            byte *block = (byte *) malloc(BLOCK_SIZE);

            fHandler->readBlock(0, block);
            Utils::copyBytes(block, rootAddress, NODE_OFFSET_SIZE);
            fHandler->writeBlock(0, block);
            free(block);
            return 0;
        }

        TreeNode *parent = new TreeNode();
        loadNode(parent, parentOffset);
        int i;
        char nodekey[keylen(&keytype)];
        for (i = 0; i < parent->numkeys; i++) {
            parent->getKey(keytype, nodekey, i);
            int isLesser = compare(nodekey, key, keytype);
            if (isLesser >= 0) {
                break;
            }
        }
        if (splitNecessary(parent->numkeys + 1, parent->flag) != 1) {
            parent->addData(keytype, key, NODE_OFFSET_SIZE, right, i);
            parent->numkeys = parent->numkeys + 1;
            storeNode(parent, Utils::getIntForBytes(parent->myaddr));

        } else {
            TreeNode *newNonLeaf = new TreeNode();
            int numPointers = parent->numkeys;
            if (parent->flag != 'c')
                numPointers++;
            int tempSpaceSize = DATA_SIZE + payloadlen + keylen(&keytype);
            char *tempSpace = (char *) calloc(tempSpaceSize, sizeof(char));
            Utils::copyBytes(tempSpace, parent->data,
                             (parent->numkeys) * keylen(&keytype));
            Utils::copyBytes(
                    &tempSpace[tempSpaceSize - (numPointers) * NODE_OFFSET_SIZE],
                    &(parent->data[DATA_SIZE - (numPointers) * NODE_OFFSET_SIZE]),
                    (numPointers) * NODE_OFFSET_SIZE);
            for (int j = parent->numkeys - 1; j >= (i); j--) {
                Utils::copyBytes(&(tempSpace[(j + 1) * keylen(&keytype)]),
                                 &(tempSpace[j * keylen(&keytype)]), keylen(&keytype));
            }
            strncpy(&(tempSpace[(i) * keylen(&keytype)]), key,
                    keylen(&keytype));
            int pointerPosition = i;
            if (parent->flag != 'c')
                pointerPosition++;
            for (int j = (tempSpaceSize - numPointers * NODE_OFFSET_SIZE);
                 j < (tempSpaceSize - pointerPosition * NODE_OFFSET_SIZE);
                 j += NODE_OFFSET_SIZE) {
                Utils::copyBytes(&(tempSpace[j - NODE_OFFSET_SIZE]),
                                 &(tempSpace[j]), NODE_OFFSET_SIZE);
            }
            strncpy(
                    &(tempSpace[tempSpaceSize
                                - (pointerPosition + 1) * NODE_OFFSET_SIZE]), right,
                    NODE_OFFSET_SIZE);

            parent->numkeys = parent->numkeys + 1;
            int n_by_two = (parent->numkeys) / 2;
            int k = 0;
            for (int i = 0; i < n_by_two; i++) {
                Utils::copyBytes(&(parent->data[(i) * keylen(&keytype)]),
                                 &(tempSpace[(i * keylen(&keytype))]), keylen(&keytype));
                Utils::copyBytes(
                        &(parent->data[DATA_SIZE - ((i + 1)) * NODE_OFFSET_SIZE]),
                        &(tempSpace[tempSpaceSize - ((i + 1) * NODE_OFFSET_SIZE)]),
                        NODE_OFFSET_SIZE);
                k = i + 1;
            }
            if (parent->flag != 'c')
                Utils::copyBytes(
                        &(parent->data[DATA_SIZE - ((k + 1)) * NODE_OFFSET_SIZE]),
                        &(tempSpace[tempSpaceSize - ((k + 1) * NODE_OFFSET_SIZE)]),
                        NODE_OFFSET_SIZE);
            for (int i = n_by_two + 1; i < parent->numkeys; i++) {
                Utils::copyBytes(
                        &(newNonLeaf->data[(i - (n_by_two + 1))
                                           * keylen(&keytype)]),
                        &(tempSpace[(i * keylen(&keytype))]), keylen(&keytype));
                Utils::copyBytes(
                        &(newNonLeaf->data[DATA_SIZE
                                           - ((i + 1) - (n_by_two + 1)) * NODE_OFFSET_SIZE]),
                        &(tempSpace[tempSpaceSize - ((i + 1) * NODE_OFFSET_SIZE)]),
                        NODE_OFFSET_SIZE);
                k = i + 1;
            }
            if (parent->flag != 'c')
                Utils::copyBytes(
                        &(newNonLeaf->data[DATA_SIZE
                                           - ((k + 1) - (n_by_two + 1)) * NODE_OFFSET_SIZE]),
                        &(tempSpace[tempSpaceSize - ((k + 1) * NODE_OFFSET_SIZE)]),
                        NODE_OFFSET_SIZE);
            newNonLeaf->flag = 'n';
            newNonLeaf->numkeys = parent->numkeys - n_by_two - 1;
            parent->numkeys = n_by_two;

            TreeNode* grandParent = new TreeNode();
            for (int i = 0; i < height; i++) {
                if (strncmp(accessPath[i], (parent->myaddr), NODE_OFFSET_SIZE)
                    == 0) if (i != 0)
                    loadNode(grandParent, accessPath[i - 1]);
            }

            char nextKey[keylen(&keytype)];

            Utils::copyBytes(nextKey,
                             &(tempSpace[(n_by_two * keylen(&keytype))]),
                             keylen(&keytype));
            storeNode(newNonLeaf, -1);
            storeNode(parent, Utils::getIntForBytes(parent->myaddr));

            char left[NODE_OFFSET_SIZE];
            Utils::copyBytes(left, parent->myaddr, NODE_OFFSET_SIZE);
            char right[NODE_OFFSET_SIZE];
            Utils::copyBytes(right, newNonLeaf->myaddr, NODE_OFFSET_SIZE);
            char parentAdd[NODE_OFFSET_SIZE];
            Utils::copyBytes(parentAdd, grandParent->myaddr, NODE_OFFSET_SIZE);
            if (parent != root)
                delete (parent);
            delete (newNonLeaf);
            delete (grandParent);
            insertIntoParent(left, nextKey, right, parentAdd, height,
                             accessPath);
        }
        return 0;
    }
    /*
     * To check if the split is necessary
     */

    int splitNecessary(int numkeys, char type) {
        int allowedKeys;
        if (type == 'c')
            allowedKeys = (DATA_SIZE)/ ((keylen(&keytype) + payloadlen) + NODE_OFFSET_SIZE);
        else {
            allowedKeys = (DATA_SIZE)/ ((keylen(&keytype) + NODE_OFFSET_SIZE)+ NODE_OFFSET_SIZE);
        }
        if (numkeys > allowedKeys)
            return 1;
        return 0;
    }

    /*
     * Function to fetch single payload
     */
    int findFirst(char key[], char payload[]){
        if(root == 0) {
            printf("BPlus Tree empty.");
            return 1;
        }
        TreeNode * current = new TreeNode();
        loadNode(current,rootAddress);

        char nodekey[keylen(&keytype)];

        int i, isLesser;
        while(current != 0) {
            current->display(keytype);

            for (i = 0 ; i<current->numkeys ; i++ ) {
                current->getKey(keytype,nodekey,i);
                isLesser = compare(nodekey,key,keytype);
                if ( isLesser == 1 || (isLesser ==0 && current->flag =='c') ){
                    break;
                }
            }

            if (current->flag == 'c') {             //This means it is leaf node..and you found data
                if (isLesser != 0)	//key not found
                    return 1;

                //key found, copy payload
                strncpy(payload,&(current->data[DATA_SIZE-(i+1)*payloadlen]),payloadlen);
                return 0;
            }
            else
                handleNonLeaf(&current, i);
        }
        return 1;
    }

    /*
     * Fuction to get an iterator for the results.
     */
    LookupIter* find(char key[]) {

        LookupIter* liResult = NULL;

        if (root == 0) {
            liResult = new LookupIter();
            printf("BPlus Tree empty.");
            //return emptyResult;
        }
        else
        {

            /*
             * TODO: Fill in code to lookup the index and return an iterator for the results
             */
            //First go to the desired child node.
            //Check if key exists.
            //If exists:
            //    Create lookup iterator object and populate values and return.
            //else
            //    Create null iterator and return.

            TreeNode * current = new TreeNode();
            loadNode(current,rootAddress);

            char nodekey[keylen(&keytype)];

            int i, isLesser=-2;
            while(current != 0) {
                current->display(keytype);

                for (i = 0 ; i<current->numkeys ; i++ ) {
                    current->getKey(keytype,nodekey,i);
                    isLesser = compare(nodekey,key,keytype);
                    if ( isLesser == 1 || (isLesser ==0 && current->flag =='c') ){
                        break;
                    }
                }

                if (current->flag == 'c') {                 //This means it is leaf node..and you found data
                    if (isLesser != 0)                      //key not found
                    {
                        liResult = new LookupIter();     //return null iterator.
                    }
                    else
                    {   //key found, copy payload
                        //strncpy(payload,&(current->data[DATA_SIZE-(i+1)*payloadlen]),payloadlen);
                        //return 0;
                        liResult = new LookupIter(key, keytype, current, i ,payloadlen);
                    }
                    break;
                }
                else
                    handleNonLeaf(&current, i);
            }
        }
        return liResult;
    }

};

void testDups(Index *index);

void testInserts(Index *index);

void indexHandlingSample();

/*
 * main function.
 */
int main() {
    //Sample code to handle indexes.
    indexHandlingSample();


     //* Checkout testInserts() and testDups() for initial testing and understanding the LookupIterator API
     //* You should create more test cases of your own for correctness checking
     //* We will use a separate set of test cases to evaluate your submission

    cout<< "This is going to be awesome" << endl;
    cout<<"TODO: Implementation"<<endl;
   return 0;
}

void indexHandlingSample() {
    KeyType keyType;
    keyType.numAttrs = 1;
    keyType.attrTypes[0] = intType;
    keyType.attrLen[0] = 8;

    char *filename = "indexomp1.ind";
    Index *index = new Index(filename, &keyType, PAYLOAD_LEN);
    testInserts(index);
    delete(index);
}

void testInserts(Index *index) {
    int a;
    srand(1);
    char *keyN = (char *)calloc(8,1);
    char payL[PAYLOAD_LEN];
    //int inputarr[] = {1,2,3,4,5,7,4,3,5,5,6,5}; //12
    //int inputarr[] = {4,6,6,6,6,5,8,1,2};   //6
    int inputarr[] = {1,2,3,4,5,4,3,5,5,6,5,5}; //12

    int numInserts = 12;
    for(int i = 0 ; i <  numInserts; i++){
        a = rand()%100;
        printf("***inserting %d\n",inputarr[i]);
        //printf("--------------------------------\n");
        Utils::copyBytes(keyN,Utils::getBytesForInt(inputarr[i]),8);
        strcpy(payL,keyN);
        index->insert(keyN,payL);
        //index->find(keyN);
        cout<<"---------------------------------\n";

    }


    //index->findFirst(keyN, payL);
    //cout<<"Value received: "<<Utils::getIntForBytes(payL)<<endl;

    Utils::copyBytes(keyN,Utils::getBytesForInt(3),8);
    //Iterator Testing
    LookupIter * li = index->find(keyN);
    //Display Iterator data.
    if(!li->isNull())
    {
        char data[PAYLOAD_LEN];
        li->get(data);
        cout<<"Iterator Data:"<<Utils::getIntForBytes(data)<<endl;
        while(li->hasNext())
        {
            li->next();
            li->get(data);
            cout<<"Iterator Data:"<<Utils::getIntForBytes(data)<<endl;
        }
    }


    //cin >> a;
    return;
}

void doInsert(Index *index, int a) {
    char *keyN = (char *) calloc(8, 1);
    char payL[PAYLOAD_LEN];

    Utils::copyBytes(keyN, Utils::getBytesForInt(a), 8);
    strcpy(payL, keyN);
    index->insert(keyN, payL);
    cout<<"---- "<<a<<" ----"<<endl;
    index->find(keyN);
    cout<<"========="<<endl;
}

void testDups(Index *index) {
    int a;
    int i;

    int testVals[] = {3,3,3,5,4,5,6};
    int arrSize = 7;
    set<int> testValsSet;

    cout<<"Starting inserts"<<endl;
    for (i = 0; i < arrSize; i++) {
        a = testVals[i];
        testValsSet.insert(a);

        doInsert(index, a);
    }
    cout<<"Done inserting"<<endl<<endl;

    while(!(testValsSet.empty())){
        int curVal = *(testValsSet.begin());
        testValsSet.erase(testValsSet.begin());

        char *keyN = (char *) calloc(8, 1);
        Utils::copyBytes(keyN, Utils::getBytesForInt(curVal), 8);

        LookupIter* res = index->find(keyN);
        char payL[PAYLOAD_LEN];
        int count = 0;
        while(res->hasNext()){
            res->next();
            res->get(payL);
            count++;
        }
        printf("res %d : %d\n", curVal, count);
        delete(res);
    }
}


