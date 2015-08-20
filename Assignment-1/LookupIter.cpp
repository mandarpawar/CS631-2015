#include "LookupIter.h"

LookupIter::LookupIter(char *_key, KeyType _keyType, TreeNode *_node, int _position, int _payloadLen) {
    key = _key;
    keyType = _keyType;
    node = _node;
    if(_position>=0 && _position < _node->numkeys)
        position = _position;
    else
        position = 0;
    payloadlen = _payloadLen;
    nullIter = false;
}

LookupIter::LookupIter() {
    nullIter = true;
}

bool LookupIter::isNull() {
    return nullIter;
}

bool LookupIter::hasNext() {
    /*
     * TODO: Fill in code
     *
     * Hint: See usage in Index.cpp.main() to understand functionality
     * Also see LookupIter.h
     */
    //Till the end of node is not found..
    //find next key, check if it matches with the iterator key. if it matches, then return true.
    bool bRetCode = false;
    if(isNull())
        return bRetCode;

    int current_position = position;

    if( current_position>=0 && (current_position < (node->numkeys)-1) )
    {
        //Get key from next_position
        byte next_key[keylen(&keyType)];
        strncpy( next_key , &(node->data[(position)*keylen(&keyType)]) ,keylen(&keyType));
        if(compare(next_key,key,keyType) == 0)
            bRetCode = true;
    }

    return bRetCode;
}

int LookupIter::next() {
    /*
     * TODO: Fill in code
     *
     * Hint: See usage in Index.cpp.main() to understand functionality
     * Also see LookupIter.h
     */
    //set position to this next key.. call to hasNext is necessary before calling this function..
    int iRetCode = -1;

    if(  position >= 0  && ( position < (node->numkeys)-1 ) )
    {
        position++;
        iRetCode = 0;
    }
    return iRetCode;
}


int LookupIter::get(char *payload) {
    /*
     * TODO: Fill in code
     */
    //Return payload of current position.
    int iRetCode = 0;
    strncpy(payload,&(node->data[DATA_SIZE-(position+1)*payloadlen]),payloadlen);
    return iRetCode;

}
