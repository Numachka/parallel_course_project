#include "main.h"

#include <mpi.h>
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
   //MPI VARIABLES
   int amountOfProcesses, rankOfProcess, packPosition;
   int data[BUFFER];
   MPI_Status status;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &amountOfProcesses);
   MPI_Comm_rank(MPI_COMM_WORLD, &rankOfProcess);

   if (amountOfProcesses != 2)
   {
      printf("Run the example with two processes only\n");
      MPI_Abort(MPI_COMM_WORLD, __LINE__);
   }

   //INPUT VARIABLES
   int w1 = 0, w2 = 0, w3 = 0, w4 = 0;
   char *seq1 = (char *)malloc(sizeof(char) * BUFFER);
   int seq1Size = 0;
   char *seq2 = (char *)malloc(sizeof(char) * BUFFER);
   int seq2Size = 0;
   char *optimum = (char *)malloc(sizeof(char) * BUFFER); //Can only be maximum or minimum. containing 7 letters.

   if (rankOfProcess == 0)
   { //First process reads input and spreads it.
      readInputFile(&w1, &w2, &w3, &w4, seq1, &seq1Size, seq2, &seq2Size, optimum);

      //TODO check input before sending. | optimum | = 7, | seq1 | > | seq2 |.

      //Pack message for 2nd process.
      packPosition = 0;
      MPI_Pack(&w1, 1, MPI_INT, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(&w2, 1, MPI_INT, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(&w3, 1, MPI_INT, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(&w4, 1, MPI_INT, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(&seq1Size, 1, MPI_INT, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(seq1, seq1Size, MPI_CHAR, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(&seq2Size, 1, MPI_INT, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(seq2, seq2Size, MPI_CHAR, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(optimum, 7, MPI_CHAR, data, BUFFER, &packPosition, MPI_COMM_WORLD);

      //Send packed message.
      MPI_Send(data, packPosition, MPI_PACKED, 1, 0, MPI_COMM_WORLD);
   }
   else
   {
      //Recieve the packed message containing all the variables for calculations.
      MPI_Recv(data, BUFFER, MPI_PACKED, 0, 0, MPI_COMM_WORLD, &status);

      //Unpack the message.
      packPosition = 0;
      MPI_Unpack(data, BUFFER, &packPosition, &w1, 1, MPI_INT, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, &w2, 1, MPI_INT, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, &w3, 1, MPI_INT, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, &w4, 1, MPI_INT, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, &seq1Size, 1, MPI_INT, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, seq1, seq1Size, MPI_CHAR, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, &seq2Size, 1, MPI_INT, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, seq2, seq2Size, MPI_CHAR, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, optimum, 7, MPI_CHAR, MPI_COMM_WORLD);
   }

   // Divide the tasks between both processes
   //OUTPUT VARIABLES
   char *resultSequence = (char *)malloc(sizeof(char) * BUFFER);
   char *seq2Mutant = (char *)malloc(sizeof(char) * BUFFER);
   char *optimalMutant = (char *)malloc(sizeof(char) * BUFFER);
   int  optimalOffset = 0,  optimalScore = 0;

   //CALCULATION VARIABLES
   int startOffset = 0 , currentOffset = 0, maximumOffset = seq1Size - seq2Size;
   if (rankOfProcess == 0) { //Set offset margins for 2 parallel tasks. Dividing into 2 parts.
      startOffset = 0;
      maximumOffset = maximumOffset / 2 + 1;
   } else {
      startOffset = maximumOffset / 2 + 1;
      maximumOffset = maximumOffset;
   }
   int mutatePosition = 0, mutateLetterCounter = 0;
   int currentScore = 0;
   
   //Initial step where mutant is equal tp sequence 2.
   int mutantSize = seq2Size;
   strcpy(seq2Mutant, seq2);

   //Start computing offset(mutant(comparison)).   
   for (int i = startOffset; i < maximumOffset; i++)
   {
      currentOffset = i;
      for (int j = 0; j < seq2Size; j++)
      {
         mutatePosition = j;
         copySequenceToAnother(seq2, seq2Size, seq2Mutant);
         for (int k = 0; k < AMOUNT_OF_ENGLISH_LETTERS; k++)
         {
            mutateLetterCounter = k;
            createResultSequenceFromSequences(seq1, seq1Size, seq2Mutant, seq2Size, currentOffset, resultSequence);
            calculateScore(w1, w2, w3, w4, resultSequence, &currentScore);
            if (currentOffset == 0)
            {
               optimalScore = currentScore;
            }
            updateOptimal(&optimalScore, &currentScore, &currentOffset, &optimalOffset, optimum, seq2Mutant, seq2Size, optimalMutant);
            mutateSequence(seq2, seq2Size, seq2Mutant, mutatePosition, mutateLetterCounter);
         }
      }
   }

   copySequenceToAnother(seq2Mutant, seq2Size, optimalMutant);

   if (rankOfProcess == 0) { //Get results from slave process and choose the better result.
      char* slaveResultMutant = (char *) malloc(sizeof(char) * BUFFER);
      int slaveResultOffset;
      int slaveResultScore;

      //Recieve results.
      MPI_Recv(data, BUFFER, MPI_PACKED, 1, 0, MPI_COMM_WORLD, &status);

      //Unpack results.
      packPosition = 0;
      MPI_Unpack(data, BUFFER, &packPosition, slaveResultMutant, seq2Size, MPI_CHAR, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, &slaveResultOffset, 1, MPI_INT, MPI_COMM_WORLD);
      MPI_Unpack(data, BUFFER, &packPosition, &slaveResultScore, 1, MPI_INT, MPI_COMM_WORLD);
      printf("receieved slave result: \n");
      printResults(slaveResultMutant, slaveResultOffset, slaveResultScore);
      printf("master results.\n");
      printResults(optimalMutant, optimalOffset, optimalScore);

      //compare and switch the returned slave mutant with the master mutant if needed.
      updateOptimal(&optimalScore, &slaveResultScore, &slaveResultOffset, &optimalOffset, optimum, slaveResultMutant, seq2Size, optimalMutant);

      printf("Final results.\n");
      printResults(optimalMutant, optimalOffset, optimalScore);
      free(slaveResultMutant);

      writeOutputFile(optimalMutant, optimalOffset, optimalScore);

   }
   else 
   { //After calculation slave process sends its result to master process.
      printf("slave results:\n");
      printResults(optimalMutant, optimalOffset, optimalScore);
      //Pack results.
      packPosition = 0;
      MPI_Pack(optimalMutant, seq2Size, MPI_CHAR, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(&optimalOffset, 1, MPI_INT, data, BUFFER, &packPosition, MPI_COMM_WORLD);
      MPI_Pack(&optimalScore, 1, MPI_INT, data, BUFFER, &packPosition, MPI_COMM_WORLD);

      //Send results.
      MPI_Send(data, packPosition, MPI_PACKED, 0, 0 ,MPI_COMM_WORLD);
   } 
// On each process - perform a first half of its task with OpenMP
// #pragma omp parallel for
//    for (i = 0; i < PART; i++) {
//             data[i]++;
//    }

// On each process - perform a second half of its task with CUDA
// if (computeOnGPU(data + PART, PART) != 0)
//    MPI_Abort(MPI_COMM_WORLD, __LINE__);

// // Collect the result on one of processes
// if (rank == 0) {
//    MPI_Recv(data + 2 * PART, 2 * PART, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

// }
// else {
//    MPI_Send(data, 2 * PART, MPI_INT, 0, 0, MPI_COMM_WORLD);
// }

// Perform a test just to verify that integration MPI + OpenMP + CUDA worked as expected
freeEverything(seq1, seq2, optimum, seq2Mutant, resultSequence, optimalMutant);

MPI_Finalize();

return 0;
}

void mutateSequence(char *seq2, int seq2Size, char *seq2Mutant, int mutatePosition, int mutateLetterCounter)
{
   char letter1 = *(seq2Mutant + mutatePosition);
   char letter2 = (char)mutateLetterCounter + 65; //Alaphabet letter - A....Z
   if (!checkIfConservative(letter1, letter2))
   {
      *(seq2Mutant + mutatePosition) = letter2;
   }
}

void copySequenceToAnother(char *seqFrom, int seqSize, char *seqTo)
{
   for (int i = 0; i < seqSize; i++)
   {
      *(seqTo + i) = *(seqFrom + i);
   }
}

void updateOptimal(int *optimalScore, int *currentScore, int *offset, int *optimaloffset, char *optimum, char *seq2Mutant, int seq2Size, char *optimaMutant)
{
   if (strcmp(optimum, "minimum") == 0)
   {
      if (*currentScore < *optimalScore)
      {
         *optimalScore = *currentScore;
         *optimaloffset = *offset;
         copySequenceToAnother(seq2Mutant, seq2Size, optimaMutant);
      }
   }
   else
   {
      if (*currentScore > *optimalScore)
      {
         *optimalScore = *currentScore;
         *optimaloffset = *offset;
         copySequenceToAnother(seq2Mutant, seq2Size, optimaMutant);
      }
   }
}

//Reads result character sequence and calculates its score value.
void calculateScore(int w1, int w2, int w3, int w4, char *resultSequence, int *currentScore)
{
   char currentSymbol = ' ';
   int i = 0, numOfMatches = 0, numOfConservatives = 0, numOfSemiConservatives = 0, numOfNoMatches = 0;
   while (currentSymbol != '\0')
   {
      switch (currentSymbol)
      {
      case MATCH:
         numOfMatches++;
         break;
      case CONSERVATIVE:
         numOfConservatives++;
         break;
      case SEMI_CONSERVATIVE:
         numOfSemiConservatives++;
         break;
      case NO_MATCH:
         numOfNoMatches++;
         break;
      }
      currentSymbol = *(resultSequence + i);
      i++;
   }
   *currentScore = (w1 * numOfMatches) - (w2 * numOfConservatives) - (w3 * numOfSemiConservatives) - (w4 * numOfNoMatches);
}

//Compares 2 sequences and outputs the fitting result character sequence.
void createResultSequenceFromSequences(char *seq1, int seq1Size, char *seq2Mutant, int seq2Size, int offset, char *resultSequence)
{
   char sequence1Letter = ' ', sequence2Letter = ' ';
   for (int i = offset; i < seq2Size; i++)
   {
      sequence1Letter = *(seq1 + i);                //Starts at offset.
      sequence2Letter = *(seq2Mutant + i - offset); //Starts at 0.
      resultSequence[i - offset] = compareLetters(sequence1Letter, sequence2Letter);
   }
   resultSequence[offset + seq2Size] = '\0';
}

//Compares 2 letters and returns their character according to rules.
char compareLetters(char letter1, char letter2)
{
   if (letter1 == letter2)
   {
      return MATCH;
   }
   else if (checkIfConservative(letter1, letter2))
   {
      return CONSERVATIVE;
   }
   else if (checkIfSemiConservative(letter1, letter2))
   {
      return SEMI_CONSERVATIVE;
   }
   else
   {
      return NO_MATCH;
   }
}

int checkIfConservative(char letter1, char letter2)
{
   return checkCharsInArray(letter1, letter2, conservativeArray, conservativeArraySize);
}

int checkIfSemiConservative(char letter1, char letter2)
{
   return checkCharsInArray(letter1, letter2, semiConservativeArray, semiConservativeArraySize);
}

int checkCharsInArray(char letter1, char letter2, char **array, int size)
{
   for (int i = 0; i < size; i++)
   {
      if (strchr(*(array + i), letter1) && strchr(*(array + i), letter2))
      {
         return 1;
      }
      return 0;
   }
}

void readInputFile(int *w1, int *w2, int *w3, int *w4, char *seq1, int *seq1Size, char *seq2, int *seq2Size, char *optimum)
{
   FILE *file = fopen(INPUT_FILE, "r");
   if (file)
   {
      fscanf(file, "%d %d %d %d\n", w1, w2, w3, w4);
      fscanf(file, "%s", seq1);
      *seq1Size = strlen(seq1);
      fscanf(file, "%s", seq2);
      *seq2Size = strlen(seq2);
      fscanf(file, "%s", optimum);
      fclose(file);
   }
   else
   {
      perror("Problem with input file!\n");
   }
}

void writeOutputFile(char *mutant, int offset, int score)
{
   FILE *file = fopen(OUTPUT_FILE, "w");
   if (file)
   {
      fprintf(file, "Mutant:\n%s\nOffset:\n%d\nAlignment score:\n%d\n", mutant, offset, score);
      fclose(file);
   }
   else
   {
      perror("Problem with output file!\n");
   }
}

void freeEverything(char *seq1, char *seq2, char *optimum, char *seq2Mutant, char *resultSequence, char *optimalMutant)
{
   free(seq1);
   free(seq2);
   free(optimum);
   free(seq2Mutant);
   free(optimalMutant);
   free(resultSequence);
}

void printResults(char *optimalMutant, int optimalOffset, int optimalScore)
{
   printf("Results\n");
   printf("%s\n%d\n%d\n", optimalMutant, optimalOffset, optimalScore);
   printf("\n");
}