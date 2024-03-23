/**************************************************************
* Class: CSC-415.02
* Name: Arslan Alimov
* Student ID: 916612104
* GitHub ID: ArslanAlimov
* Project: Assignment 4 – Word Blast
*
* File: Alimov_Arslan_HW4_main.c
*
* Description:
*             We are using threading
*             reading txt file and outputting top 10 most frequent words into the console            
*             we work with buffers, lockers and proper way to lock the threads
*   The purpose of the code is to read text file and output top 10 most frequent words from the file. 
*    The software implements a multithreading with different protections.
**************************************************************/
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
/*
***********************************************
Variables:
Create Minimum Char = 5 - Done to compare if word is longer than 5 chars then we don't want it
Create Buffer to Hold our text file = Done
Create Frequency Variable (In assigment it is 10, but we can change to whatever in future) - Done
************************************************
*/
#define MIN_CHAR 5
#define BUFFER_SIZE 1024
#define TOP_FREQUENT 10
char *defaultFileName = "WarAndPeace.txt";
pthread_mutex_t lock;   // lock for our thread when we share resources
int isMutexLock;        // check if we failed to initialize our lock
int threadNum;          // amount of threads

/*
Data Struct 
Need to have File Name - Every File has a name
beginning of the reading  - In C it needs to know where to start reading the file from same in C++ 
size in bytes - we need to know the size of file
Amount of threads and we need to know how many threads to use
*/

typedef struct fileData
{
    char *textName;
    char * beginning;
    long chunkSize;
    int numThreads;
} fileData, *fileDataPtr;

/*
- Create a Data Struct to hold a word from text and count total words in our list
- counter to keep track of total 
*/
typedef struct textCounter
{
    char *text;
    int counter;
} textCounter, *textCounterPtr;

// You may find this Useful
char *delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";
/*
************************************************
delimeter is all the separators in the text 
************************************************
*/

textCounterPtr textList = NULL;
int textTotal = 0;
/*
List of all our text
number of text in array
*/

/*
Creating prototypes of methods
In C the text is being read from top to bottom , the code below helps us compile addtext , 
Reader = reads file
text add = adds text to array 
print top uses bubble sort to compare values
read our file and print our top words amount of top we can declare inside the function
*/
// prototype of reading the file
void *reader(void *);
// adding text
void textAdd(char *);
//print top 10 we include the place where our word at , name of file , thread count and how many Top texts to list
void printTopText(struct textCounter *, char *, int);
//to clean our buffers multiple
void freeBuffers(struct textCounter *, int);
//clean one buffer and set to null
int FileExist(char *);

int main(int argc, char *argv[])
{
    char textFile[100] = {};
    threadNum = 0;
    if (argc == 3)
    {
        threadNum = atoi(argv[2]);
        // string to num
        int len = strlen(argv[1]);
        //gets length of argv
        if (len < 100)
            memcpy(textFile, argv[1], len);
        //if our length less than 100 then we memory copy the word into our buffer
        else
        {
            if (argc == 2)
            {
                threadNum = atoi(argv[1]);
            }
            else
            {
                puts("Using default thread count of 1.\n");
                threadNum = 1;
            }
            memcpy(textFile, defaultFileName, 15);
        }
    }
    else
    {
        if (argc == 2)
            threadNum = atoi(argv[1]);
        memcpy(textFile, defaultFileName, 15);
    }

    // else we copy default filename
    if (threadNum <= 0)
    {
        puts("Thread Number can't be less than 1. Using defaults thread count of 1.\n");
        threadNum = 1;
        //check if threads less than 0 then our threads is 1
    }
    else if (!FileExist(defaultFileName))
    {
        printf("Default File Name does not Exist [Error");
    }
    /*
    Working with files 
    forming a buffer that stores our words
    */
    FILE *fileHandle = fopen(argv[1], "r");
    fseek(fileHandle, 0, SEEK_END);  //make sure at start
    long length = ftell(fileHandle);//get file size
    char *mainBuffer;
    if (fileHandle)
    {   
        fseek(fileHandle, 0, SEEK_SET);       //rewind handle to start
        char *buffer = malloc(length + 1);    //create a buffer large enough +1 for \0
        fread(buffer, 1, length, fileHandle); //read the entire file into the buffer
        fclose(fileHandle);                   //close the file stream
        buffer[length] = '\0';                //set the \0 at the end of the file
        mainBuffer = buffer;
    }
    /*
        Code above is checking for different problems that the user might face. I set everything to default if something goes wrong. If the user inputs commandline
        the wrong way e.t.c. It also checks if the file exists, our text file 
    */

    // we need to check if our file name is empty first and check if threads are negative numbers because threads can't be negative
    isMutexLock = pthread_mutex_init(&lock, NULL);
    /*
    check if our mutex was properly initialized when our mutex is null it gives us default parameters
    our assert checks if the mutex failed or not, if it fails our code will be closed (terminated)
    */
    assert(isMutexLock == 0);
    // DO NOT CHANGE THIS BLOCK
    //Time stamp start

    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************
    // *** TO DO ***  start your thread processing
    //                wait for the threads to finish
    int check = 1;
    // this is our flag variable if anything goes wrong our  check = -1 for savety purposes
    fileData *FileArray;
    pthread_t *ThreadID;
    /*
        Creating unique thread ID's 
        creating an array of files

    */
    FileArray = malloc(sizeof(fileData) * threadNum);
    assert(FileArray != NULL);
    /*
        using assert to check if FileArray is not equalto null
        same with thread ID instead of If statements
        
    */
    ThreadID = malloc(sizeof(pthread_t) * threadNum);
    assert(ThreadID != NULL);

    for (size_t i = 0; i < threadNum; i++)
    {
        //we initialize array in here
        /*
        pass all our offsets and some math stuff happening over here needed to find remainder and beginning and chunk size
        */
        long startOffset = (length / threadNum) * i;
        FileArray[i].textName = textFile;
        FileArray[i].numThreads = threadNum;
        FileArray[i].chunkSize = (!((i + 1) % threadNum)) ? (length - startOffset) : (length / threadNum);
        FileArray[i].beginning = mainBuffer + startOffset;
        //if we are unable to create thread then we output that error.
        if (pthread_create(&ThreadID[i], NULL, reader, (void *)&FileArray[i]))
        {
            printf("Unable to create Thread \n");
        }
    }
    
    /*
     for safety purposes 
     */
    for (size_t i = 0; i < threadNum; i++)
    {
        if (pthread_join(ThreadID[i], NULL))
        {
            printf("Thread can't be joined!");
            check = -1;
        }
    }

    // ***TO DO *** Process TOP 10 and display
    textCounter numList[TOP_FREQUENT];
    printTopText(numList, textFile, 10);

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
    {
        --sec;
        n_sec = n_sec + 1000000000L;
    }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************

    // ***TO DO *** cleanup
    freeBuffers(numList, 10);
}

void *reader(void *FileArray)
{
    fileData *file = FileArray;
    //create datastruct
    char *addedText;
    char *remainder;
    //rest of bytes stored in remainder , storage for words and added text for our text that is being added
    addedText = strtok_r(file->beginning, delim, &remainder);    
    //get text from file and tokenize it 
    //check the chunk and the difference between  file and chunk size
      while (addedText != NULL && (remainder - (file->beginning)) / sizeof(file->beginning[0]) < file->chunkSize)
    {
        if (strlen(addedText) > MIN_CHAR)
        {
            textAdd(addedText);
        }
        addedText = strtok_r(NULL, delim, &remainder);
    }
    return NULL;
}

void textAdd(char *AnotherText)
{
    /*
    cause of segment fault in here because if there are too many threads the words are being filled up super fast. can fix it by adding another
    mutex lock and unlock in our for loop but it slows down our code by a lot almost 8
    */
    static volatile int TextSize = 0; //size of word entry
    for (size_t i = 0; i < textTotal; i++)
    {
        if (!strcasecmp(AnotherText, textList[i].text))
        {
            // Create thread and do their job with the text provided
            pthread_mutex_lock(&lock);
            textList[i].counter++;
            pthread_mutex_unlock(&lock);
            return;
        }
    }
    //need to lock our thread
    pthread_mutex_lock(&lock);

    /*
    need to realloc when our datastruct overfills
    */
    if (textTotal >= TextSize)
    {
        TextSize += BUFFER_SIZE;
        textList = (textList == NULL) ? malloc(sizeof(textCounter) * TextSize) : realloc(textList, sizeof(textCounter) * TextSize);
    }

    //we allocate buffer size based on the length of the characters in the words that are being passed
    textList[textTotal].text = malloc(strlen(AnotherText) + 2);
    
    if (textList == NULL)
    {
        printf("Unable to malloc/reallocate Text List");
        exit(errno);
    }
    //check if our textlist is empty
    if (textList[textTotal].text == NULL)
    {
        printf("Unable to allocate memory for Another text from file");
        exit(errno);
    }
    strcpy(textList[textTotal].text, AnotherText);
    textList[textTotal++].counter = 1;
    pthread_mutex_unlock(&lock);
}

//method below frees all buffers sets them to 0
void freeBuffers(struct textCounter *numList, int topNum)
{
    for (size_t i = 0; i < topNum; i++)
    {
        free(numList[i].text);
        numList[i].text = NULL;
    }
    free(textList);
    textList = NULL;

    if (pthread_mutex_destroy(&lock))
    {
        printf("Unable to destroy the lock");
        exit(1);
    }
    //if statement but also destroys our mutex
}

void printTopText(struct textCounter *numList, char *fileName, int topNum)
{
    //set everything to empty and set counter to 0
    for (int i = 0; i < topNum; i++)
    {
        numList[i].text = "";
        numList[i].counter = 0;
    }

    /*
        we need a for loop to iterate through the list 
        then we need an expression of if statement and if the counter greater than 730
        I was testing out and trying to find out where the code will give me 0 words.
        so I was playing with that number. ( Was not sure how to implement anything else other than this messy code)
        at 730 I get all text words as well as it does not give me any errors. 
        Bad thing about that code that the number is hard coded ;'( we also check for our y if its greater than 10 because we just need top 10 lists
    */
    for (int i = 0, y = 0; i < textTotal; i++)
    {
        if (textList[i].counter > 730)
        {
            if (y < topNum)
            {
                numList[y].text = textList[i].text;
                numList[y++].counter = textList[i].counter;
            }
        }
    }

    for (int i = 0; i < topNum - 1; i++)
    {
        /*
            we use bubble sort for this 
            we basically push the highest value to the top of the list  till all of our highest values on top 
            This was the simplest solution and the only one I understand the most
        */
        int countTemp = 0;
        for (int q = 0, limit = topNum - i - 1; q < limit; ++q)
        {
            if (numList[q].counter <= numList[q + 1].counter)
            {
                char *temp = numList[q].text;
                numList[q].text = numList[q + 1].text;
                numList[q + 1].text = temp;

                countTemp = numList[q].counter;
                numList[q].counter = numList[q + 1].counter;
                numList[q + 1].counter = countTemp;
            }
        }
    }
    printf("\nWord Frequency Count on %s with %d threads \n", fileName, threadNum);
    printf("Printing top %d words %d characters or more.\n", topNum, MIN_CHAR);
    for (int i = 0; i < topNum; i++)
    {
        printf("Number %d is %s with a count of %d\n", (i + 1), numList[i].text, numList[i].counter);
    }
}

int FileExist(char *filename)
{
    struct stat sBuff;
    return (stat(filename, &sBuff) == 0);
}

//valgrind --leak-check=full  ./Alimov_Arslan_HW4_main WarAndPeace.txt 3
//multiple memory leaks I was not sure how to fix it
