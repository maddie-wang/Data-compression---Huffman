// Assignment 6: Huffman
// Maddie Wang, 11/26/2017
// Huffman is an algorithm that compresses data to make a file have a smaller number of bytes.
// This program includes an extra feature : Being able to gracefully handle bad input files.

#include "simpio.h"
#include "encoding.h"
#include "pqueue.h"
#include <iostream>
#include <string>
#include "bitstream.h"
#include "filelib.h"
using namespace std;

const string SECRET_CODE = "11111101";

bool hasSecretCode (ibitstream& input);
void decodeHelper(ibitstream& input, HuffmanNode* treeTraveler, HuffmanNode* encodingTree, ostream& output, int bit);
Map<int, string> encodingMapHelper(Map<int, string>& encodingMap, HuffmanNode* encodingTree, string path);

// buildFrequencyTable uses user input to build a map of characters and their frequencies.
Map<int, int> buildFrequencyTable(istream& input) {
    Map<int, int> freqTable;
    int readChar;
    readChar = input.get();
    while (!input.fail()) { // until the whole file is read, keep reading input characters and increasing frequency for that character
        freqTable[readChar]++;
        readChar = input.get();
    }
    freqTable[PSEUDO_EOF]++; // adds one to the special "end-of-file" (EOF) byte
    return freqTable;
}

// buildEncodingTree accepts a frequency table and uses it to create a Huffman
// encoding binary tree based on those frequencies. It will return a pointer to the node representing the root of the tree.
HuffmanNode* buildEncodingTree(const Map<int, int>& freqTable) {
    PriorityQueue<HuffmanNode*> huffQueue; // priority queues will automatically order the huffman queue by lowest frequency, as needed
    for (int ch : freqTable) // for every character:frequency in the frequency table, convert it
    { // into a huffman pointer node and add it to the huffman priority queue
        HuffmanNode* node = new HuffmanNode();
        node->character = ch;
        node->count = freqTable[ch];
        huffQueue.enqueue(node, freqTable[ch]); // enqueues a character with the character's frequency as its priority
    }
    while (huffQueue.size() > 1) { // Remove two nodes from the front of priority queue (two with smallest frequencies).
        // Make them children of new node. Reinsert new node in queue until there is one HuffmanNode left
        HuffmanNode* childA = huffQueue.dequeue();
        HuffmanNode* childB = huffQueue.dequeue();
        HuffmanNode* parent = new HuffmanNode();
        parent->character = NOT_A_CHAR;
        parent->count = childA->count + childB->count;
        parent->zero = childA;
        parent->one = childB;
        huffQueue.enqueue(parent, parent->count);
    }
    return huffQueue.peek(); // return node representing root of encoding tree
}

// By traversing the encoding tree, buildEncodingMap produces a map of characters and their binary representations.
Map<int, string> buildEncodingMap(HuffmanNode* encodingTree) {
    Map<int, string> encodingMap;
    return encodingMapHelper(encodingMap, encodingTree, ""); // recurisive helper function
}

// encodingMapHelper produces a map of characters and their binary representations using pre-order recursive tree traversal.
// This function uses the encoding map and encoding tree to build a binary path. Once completed, it adds this path to the
// the leaf node's character.
Map<int, string> encodingMapHelper(Map<int, string>& encodingMap, HuffmanNode* encodingTree, string path) {
    if (encodingTree != NULL) {
        if (encodingTree->character!=NOT_A_CHAR) { // base case success; node is a leaf node.
            encodingMap[encodingTree->character] = path; // maps the lead node's character to the binary path it created
        }
        encodingMapHelper(encodingMap, encodingTree->zero, path + "0"); // go left!
        encodingMapHelper(encodingMap, encodingTree->one, path + "1"); // go right!
    }
    return encodingMap;
}

// Using the encoding map, encodeData encodes user input into a shorter binary representation. Writes binary representation in output.
void encodeData(istream& input, const Map<int, string>& encodingMap, obitstream& output) {
    string bitString;
    int readChar;
    readChar = input.get();
    while (!input.fail()) { // until the whole file is read, reads characters from input
        bitString += encodingMap[readChar]; // concacts the binary reprentation of character onto bitString
        readChar = input.get();
    }
    bitString += encodingMap[PSEUDO_EOF]; // adds our final PSEUDO EOF key to our binary string
    for (int i = 0; i < bitString.length(); i++) {
        output.writeBit(bitString[i]-'0'); // writes each binary number to the output from our binary string. -'0' to convert into 0s and 1s.
    }
}

// decodeData uses a Huffman tree to decode input that was previously encoded with its binary patterns. Writes the decoded text in output.
// decodeData calls on decodeHelper as a recursive helper function.
void decodeData(ibitstream& input, HuffmanNode* encodingTree, ostream& output) {
    HuffmanNode* treeTraveler = encodingTree;
    int bit = input.readBit();
    decodeHelper(input, treeTraveler, encodingTree, output, bit);
}

// decodeHelper recursively decodes input encoded in binary patterns from a Huffman tree. Writes decoded text in output.
void decodeHelper(ibitstream& input, HuffmanNode* treeTraveler, HuffmanNode* encodingTree, ostream& output, int bit) {
    // we will read each bit one by one, following the bits' path on the encoding tree
   // until we have reached a leaf node. Then, we will write the leaf node's character to output and repeat until we've read all the bits.
    if(bit == -1 || treeTraveler->character==PSEUDO_EOF) { // base case failure: stop once we see the pseudo eof code or there's no more input
        return;
    }
    if(treeTraveler!=NULL && treeTraveler->one==NULL && treeTraveler->zero==NULL) { // base case success:
        // once we've hit a tree node, write that character and reset tree traveler to point to root node
        output.put(treeTraveler->character);
        treeTraveler = encodingTree;
    }
    if (bit == 0) { // Recursive call: Go left, and read next bit
        decodeHelper(input, treeTraveler->zero, encodingTree, output, input.readBit());
    }
    else if (bit == 1) { // Recursive call: Go right, and read next bit
        decodeHelper(input, treeTraveler->one, encodingTree, output, input.readBit());
    }
}

// This function compresses the given input file into the given output file, using Huffman encoding.
void compress(istream& input, obitstream& output) {
    for (int i = 0; i < SECRET_CODE.length(); i++) { // writes the secret code in the beginning of file
        output.writeBit(SECRET_CODE[i]-'0');
    }
    Map<int, int> freqTable = buildFrequencyTable(input);
    output << freqTable; // writes frequency table to output
    rewindStream(input); // we rewind input because we've went to input.fail() from our buildFrequencyTable() function
    HuffmanNode* encodingTree = buildEncodingTree(freqTable); // converts freqtable to tree
    Map<int, string> encodingMap = buildEncodingMap(encodingTree); // converts tree to map
    encodeData(input, encodingMap, output); // encodes output with our encoding map and input
    freeTree(encodingTree);
}

// This function reads bits from the given input file then writes the original contents of the file. This function
// decodes the bits by reversing Huffman encoding.
void decompress(ibitstream& input, ostream& output) {
    bool isValid = hasSecretCode(input); // checks to see if the file is valid (if it contains secret code in beginning)
    if(isValid) {
        Map<int, int> freqTable;
        input >> freqTable; // writes header into frequency table
        HuffmanNode* encodingTree = buildEncodingTree(freqTable);
        decodeData(input, encodingTree, output); // decodes the input using encoding tree. writes that to output.
        freeTree(encodingTree);
    }
    else {
        cout << "Nice try, you're missing the secret code. Your file is invalid." << endl;
    }
}

// This function checks the beginning of the file for the secret code that only valid compressed files will have.
bool hasSecretCode (ibitstream& input) {
    bool secretCode = true;
    for (int i = 0; i < SECRET_CODE.length(); i++) { // checks for secret code by seeing if each bit matches the secret code's
        int bit = input.readBit();
        if(bit!=SECRET_CODE[i]-'0') { // sets secretCode to false when any bit doesn't match secret code
            secretCode = false;
        }
    }
    return secretCode;
}

// This function recursively frees the memory of a given root node that is associated with the HuffmanNode tree.
void freeTree(HuffmanNode* node) {
    if (node == NULL) { // base case failure: node is null
        return;
    }
    freeTree(node->zero); // otherwise recursively deletes every memory address in tree (backtracking)
    freeTree(node->one);
    delete node;
}
