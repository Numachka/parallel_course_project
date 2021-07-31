#pragma once

#define PART  100

void test(int *data, int n);
int computeOnGPU(int *data, int n);

#define __INDEX
//Input/Output definitions and global variables.
#define INPUT_FILE "./input.txt"
#define BUFFER 255
#define OUTPUT_FILE "./output.txt"
#define MATCH '*'
#define CONSERVATIVE ':'
#define SEMI_CONSERVATIVE '.'
#define NO_MATCH ' '
#define MAX_MUTATIONS 351
#define AMOUNT_OF_ENGLISH_LETTERS 26

char *conservativeArray[] = {
    (char *)"NDEQ", (char *)"NEQK", (char *)"STA", (char *)"MILV",
    (char *)"QHRK", (char *)"NHQK", (char *)"FYW", (char *)"HY", (char *)"MILF"};
int conservativeArraySize = 9;
char *semiConservativeArray[] = {
    (char *)"SAG", (char *)"ATV", (char *)"CSV", (char *)"SGND", (char *)"STPA", (char *)"STNK",
    (char *)"NEQHRJ", (char *)"NDEQHK", (char *)"SNDEQK", (char *)"HFY", (char *)"FVLIM"};
int semiConservativeArraySize = 11;

void readInputFile(int *w1, int *w2, int *w3, int *w4, char *seq1, int *seq1Size, char *seq2, int *seq2Size, char *optimum);
void writeOutputFile(char *mutant, int offset, int score);
void createResultSequenceFromSequences(char *seq1, int seq1Size, char *seq2Mutant, int seq2Size, int offset, char *resultSequence);
int checkIfSemiConservative(char letter1, char letter2);
int checkIfConservative(char letter1, char letter2);
int checkCharsInArray(char letter1, char letter2, char **array, int size);
char compareLetters(char letter1, char letter2);
void mutateSequence(char *seq2, int seq2Size, char *seq2Mutant, int mutatePosition, int mutateLetterCounter);
void calculateScore(int w1, int w2, int w3, int w4, char *resultSequence, int *currentScore);
void updateOptimal(int *optimalScore, int *currentScore, int *offset, int *optimalOffset, char *optimum, char *seq2Mutant, int seq2Size, char *optimalMutant);
void freeEverything(char *seq1, char *seq2, char *optimum, char *seq2Mutant, char *resultSequence, char *optimalMutant);
void printSequence(char *sequence);
void copySequenceToAnother(char *seqFrom, int seqSize, char *seqTo);
void printResults(char *optimalMutant, int optimalOffset, int optimalScore);