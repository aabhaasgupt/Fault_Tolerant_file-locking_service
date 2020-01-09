#include "defns.h"

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <iostream>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"
#define numOfFiles 20
#define ECHOMAX 255 /* Longest string to echo */

FILE *lockTable;
FILE *clientTable;
FILE *F[numOfFiles];

void clrscr()
{
    system("@cls||clear");
}
void liner()
{
    
    printf("\n" MAG);
    for(int i=0;i<80;i++)
    {
        printf("=");
    }
    printf(RESET"\n");
}
void serializeReq(char *str, struct request rr)
{

    char cn[50], rn[50], in[50];
    snprintf(cn, sizeof(cn), "%d", rr.c);
    snprintf(rn, sizeof(rn), "%d", rr.r);
    snprintf(in, sizeof(in), "%d", rr.i);

    strcpy(str, rr.m);
    strcat(str, "-");
    strcat(str, cn);
    strcat(str, "-");
    strcat(str, in);
    strcat(str, "-");
    strcat(str, rn);
    strcat(str, "-");
    strcat(str, rr.operation);
    strcat(str, ":");
}

struct request deserializeReq(char *strr)
{
    char strOrig[300];

    strcpy(strOrig, strr);
    char str[500];
    strcpy(str, strOrig);
    struct request rr;
    strcpy(rr.m, strtok(str, "-"));
    rr.c = std::stoi(strtok(NULL, "-"));
    rr.i = std::stoi(strtok(NULL, "-"));
    rr.r = std::stoi(strtok(NULL, "-"));
    strcpy(rr.operation, strtok(NULL, ":"));
    char tempOp[80];
    strcpy(tempOp, rr.operation);
    strcpy(rr.operationType, strtok(tempOp, "-"));
    if (strcmp(rr.operationType, "close") == 0)
    {
        strcpy(rr.fileName, strtok(NULL, ":"));
    }
    else
    {
        strcpy(rr.fileName, strtok(NULL, "-"));
    }
    if (strcmp(rr.operationType, "read") == 0 || strcmp(rr.operationType, "lseek") == 0)
    {

        rr.readOrLseek = std::stoi(strtok(NULL, ":"));
    }
    else if (strcmp(rr.operationType, "write") == 0)
    {
        char tempVar[60];
        strcpy(tempVar, strtok(NULL, ":"));
        int len = strlen(tempVar);
        tempVar[len - 1] = '\0';
        char *contents_chopped = tempVar + 1;
        strcpy(rr.writeString, contents_chopped);
    }
    else if (strcmp(rr.operationType, "open") == 0)
    {
        strcpy(rr.openMode, strtok(NULL, ":"));
    }
    return rr;
}
//
struct request GetReqObj(const char *m, int c, int i, int r, const char *operation)
{
    struct request rr;
    strcpy(rr.m, m);
    rr.c = c;
    rr.i = i;
    rr.r = r;
    strcpy(rr.operation, operation);
    return rr;
}

void getClientLine(struct request req, char *response, char *newLine)
{

    char cn[50], rn[50], in[50];
    snprintf(cn, sizeof(cn), "%d", req.c);
    snprintf(rn, sizeof(rn), "%d", req.r);
    snprintf(in, sizeof(in), "%d", req.i);

    strcat(newLine, req.m);
    strcat(newLine, " ");
    strcat(newLine, cn);
    strcat(newLine, " ");
    strcat(newLine, in);
    strcat(newLine, " ");
    strcat(newLine, rn);
    strcat(newLine, " ");
    strcat(newLine, req.operation);
    strcat(newLine, ":");
    strcat(newLine, response);
    strcat(newLine, ";\n");
}

void addNewClient(struct request req)
{
    char response[20];
    strcpy(response, "<NO-REQUEST-YET>");
    char newLine[300 + 1] = "";
    getClientLine(req, response, newLine);
    //printf("\nnewline:%s\n",newLine);
    FILE *fptr;
    char tempM[24];
    int tempC;
    char temp[] = "temp.txt";
    char fname[100];
    strcpy(fname, "clientTable.txt");

    fptr = fopen("temp.txt", "w"); // open the temporary file in write mode
    if (!fptr)
    {
        printf(RED "Unable to open a temporary file to write!!\n" RESET);
        fclose(fptr);
        return;
    }
    //printf("======\n%s",newLine);

    fputs(newLine, fptr);
    char line[300 + 1] = "";

    char line2[300 + 1] = "";
    fseek(clientTable, 0, SEEK_SET);
    while (fgets(line, sizeof(line), clientTable))
    {
        strcpy(line2, line);
        fputs(line2, fptr);
        //printf("\n%s",line2);
    }
    //printf("\n======");
    fclose(clientTable);
    fclose(fptr);
    remove(fname); // remove the original file
    rename(temp, fname);
    clientTable = fopen("clientTable.txt", "r+");
    fseek(clientTable, 0, SEEK_SET);

    printf(GRN "New Client Added:%s,%d\n" RESET, req.m, req.c);
}

void adjustClientTable(struct request req, char *response)
{
    char newLine[300 + 1] = "";
    getClientLine(req, response, newLine);
    FILE *fptr;
    char tempM[24];
    int tempC;
    char temp[] = "temp.txt";
    char fname[100];
    strcpy(fname, "clientTable.txt");

    fptr = fopen("temp.txt", "w"); // open the temporary file in write mode
    if (!fptr)
    {
        printf(RED "Unable to open a temporary file to write!!\n" RESET);
        fclose(fptr);
        return;
    }
    char line[300 + 1] = "";

    char line2[300 + 1] = "";
    fseek(clientTable, 0, SEEK_SET);
    while (fgets(line, sizeof(line), clientTable))
    {
        strcpy(line2, line);
        strcpy(tempM, strtok(line, " "));
        tempC = std::stoi(strtok(NULL, " "));
        if (strcmp(tempM, req.m) == 0 && tempC == req.c)
        {
            fputs(newLine, fptr);
        }
        else
        {
            fputs(line2, fptr);
        }
    }
    fclose(clientTable);
    fclose(fptr);
    remove(fname); // remove the original file
    rename(temp, fname);
    clientTable = fopen("clientTable.txt", "r+");
}

void initializeTables()
{
    
    lockTable = fopen("lockTable.txt", "w");
    clientTable = fopen("clientTable.txt", "w");
    fclose(lockTable);
    fclose(clientTable);

    
    lockTable = fopen("lockTable.txt", "r+");
    clientTable = fopen("clientTable.txt", "r+");


    for (int i = 0; i < numOfFiles; i++)
    {
        F[i] = (FILE *)malloc(sizeof(FILE *));
    }
}

void writeBlind(FILE *file, char *bytesToWrite)
{

    
    for (int i = 0; bytesToWrite[i] != '\0'; i++)
    {
        fputc(bytesToWrite[i], file);
    }
}

char *readBlind(FILE *file, int bytesToRead)
{
    char *buffer;
    buffer = (char *)malloc(sizeof(char) * bytesToRead);
    for (int i = 0; i < bytesToRead; i++)
    {
        buffer[i] = fgetc(file);
    }
    buffer[bytesToRead] = '\0';
    return buffer;
}

void addNewFile(char *fileName)
{
    char newLine[30];
    strcpy(newLine, fileName);
    strcat(newLine, " 0\n");
    FILE *fptr;
    char tempFN[10];
    char temp[] = "temp.txt";
    char fname[100];
    strcpy(fname, "lockTable.txt");

    fptr = fopen("temp.txt", "w"); // open the temporary file in write mode
    if (!fptr)
    {
        printf(RED "Unable to open a temporary file to write!!\n" RESET);
        fclose(fptr);
        return;
    }

    fputs(newLine, fptr);
    char line[300 + 1] = "";

    char line2[300 + 1] = "";
    fseek(lockTable, 0, SEEK_SET);
    while (fgets(line, sizeof(line), lockTable))
    {
        strcpy(line2, line);
        fputs(line2, fptr);
    }
    fclose(lockTable);
    fclose(fptr);
    remove(fname); // remove the original file
    rename(temp, fname);
    lockTable = fopen("lockTable.txt", "r+");
    fseek(lockTable, 0, SEEK_SET);
    FILE *tempF;
    tempF = fopen(fileName, "r+");
    fclose(tempF);
    printf(YEL "New File Entry Added:%s!\n" RESET, fileName);
}

void adjustLockTable(char *fileName, char *newLine)
{
    FILE *fptr;
    char tempFN[10];
    char temp[] = "temp.txt";
    char fname[100];
    int flag = 1;
    strcpy(fname, "lockTable.txt");

    fptr = fopen("temp.txt", "w"); // open the temporary file in write mode
    if (!fptr)
    {
        printf(RED "Unable to open a temporary file to write!!\n" RESET);
        fclose(fptr);
        return;
    }
    char line[300 + 1] = "";

    char line2[300 + 1] = "";
    fseek(lockTable, 0, SEEK_SET);
    while (fgets(line, sizeof(line), lockTable))
    {
        strcpy(line2, line);
        strcpy(tempFN, strtok(line, " "));
        if (strcmp(tempFN, fileName) != 0)
        {
            fputs(line2, fptr);
        }
        else
        {
            flag = 0;
            fputs(newLine, fptr);
        }
    }
    if (flag == 1)
    {
        addNewFile(fileName);
        flag = 0;
    }
    fclose(lockTable);
    fclose(fptr);
    remove(fname); // remove the original file
    rename(temp, fname);
    lockTable = fopen("lockTable.txt", "r+");
}

void getLockLine(char *newLine, char *fName, char *m, int c, int incarnationNum, char *requestOperation)
{
    strcat(newLine, fName);
    strcat(newLine, " 1 ");
    strcat(newLine, m);
    strcat(newLine, " ");

    char cn[50], in[50];
    snprintf(cn, sizeof(cn), "%d", c);
    snprintf(in, sizeof(in), "%d", incarnationNum);
    strcat(newLine, cn);
    strcat(newLine, " ");
    strcat(newLine, requestOperation);
    strcat(newLine, " ");
    strcat(newLine, in);
    strcat(newLine, "\n");
}

int getFNumber(char *fName)
{ //Get File Number from file name
    char fileName[10];
    strcpy(fileName, fName);
    char remExt[10];
    strcpy(remExt, strtok(fileName, "."));
    char *contents_chopped = fileName + 1;
    //printf("\n%s\n",contents_chopped);

    return std::stoi(contents_chopped);
}

void removeFromLockTableAndCloseFile(struct request req)
{ //Remove from lock table and close files
    char line[300 + 1] = "";
    fseek(lockTable, 0, SEEK_SET);
    char fileNamel[20], fileNamelTemp[20];
    char lockedByMachine[24], lockedForOperation[20];
    int lockedByClient, tempP, isLocked, incarnationNum, fNumber;
    int counter = 0;
    while (fgets(line, sizeof(line), lockTable))
    {

        
        strcpy(fileNamel, strtok(line, " "));
        //{ //////////////// File Entry Found in Lock Table ///////////

        isLocked = std::stoi(strtok(NULL, " "));
        if (isLocked == 1)
        { ////////////// File is locked ///////////////////
            strcpy(lockedByMachine, strtok(NULL, " "));
            lockedByClient = std::stoi(strtok(NULL, " "));
            strcpy(lockedForOperation, strtok(NULL, " "));
            if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
            {
                incarnationNum = std::stoi(strtok(NULL, " "));
                fNumber = getFNumber(fileNamel);
                if (incarnationNum != req.i)
                {
                    counter++;
                }
            }
        }
    }
    if (counter == 0)
    {
        addNewFile(req.fileName);
    }

    char *newLines[counter];

    for (int i = 0; i < counter; i++)
    {
        newLines[i] = (char *)malloc(sizeof(char *));
    }

    int j = 0;
    fseek(lockTable, 0, SEEK_SET);
    while (fgets(line, sizeof(line), lockTable))
    {
        strcpy(fileNamel, strtok(line, " "));

        //{ //////////////// File Entry Found in Lock Table ///////////
        isLocked = std::stoi(strtok(NULL, " "));
        if (isLocked == 1)
        { ////////////// File is locked ///////////////////
            strcpy(lockedByMachine, strtok(NULL, " "));
            lockedByClient = std::stoi(strtok(NULL, " "));
            strcpy(lockedForOperation, strtok(NULL, " "));
            if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
            {
                //printf("\nenter\n");
                incarnationNum = std::stoi(strtok(NULL, " "));
                if (incarnationNum != req.i)
                {

                    fNumber = getFNumber(fileNamel);
                    fclose(F[fNumber]);
                    printf(RED "File Closed due to incarnation check failure!\n" RESET);
                    char newLine[300 + 1] = "";
                    strcat(newLine, fileNamel);
                    strcat(newLine, " 0\n");
                    strcpy(newLines[j], newLine);
                    j++;
                }
            }
        }
    }
    char tem[300 + 1] = "";
    char tempFn[10];
    for (int i = 0; i < counter; i++)
    {

        strcpy(tem, newLines[i]);
        strcpy(tempFn, strtok(tem, " "));
        adjustLockTable(tempFn, newLines[i]);
    }
}

int incarnationCheck(struct request req)
{ //Incarnation check
    char line[300 + 1] = "";
    fseek(clientTable, 0, SEEK_SET);
    char machine[24];
    int incarnationNum, client;
    while (fgets(line, sizeof(line), clientTable))
    {
        strcpy(machine, strtok(line, " "));
        client = std::stoi(strtok(NULL, " "));
        if (strcmp(machine, req.m) == 0 && client == req.c)
        {
            incarnationNum = std::stoi(strtok(NULL, " "));
            if (incarnationNum == req.i)
            {
                return 1;
            }
            else if (req.i < incarnationNum)
            {
                printf(RED "Something Wrong with incarnation number,%d" RESET);
                return 0;
            }
            else
            {
                printf(YEL "Change in Incarnation Number! Adjusting...\n" RESET);
                char nullres[5] = "NULL";
                adjustClientTable(req, nullres);
                removeFromLockTableAndCloseFile(req); ///update client table incarnation number.
                
                return 1;
            }
            printf(RED "Something Wrong! Incarnation Failure!\n" RESET);
            return 0;
        }
    }
    addNewClient(req);
    return 1;
}

char *getResponseOnly(struct request req, char *resp)
{ //Incarnation check
    char line[300 + 1] = "";
    char operation[80 + 1];

    fseek(clientTable, 0, SEEK_SET);
    char machine[24];
    int incarnationNum, client, reqNum;
    while (fgets(line, sizeof(line), clientTable))
    {
        strcpy(machine, strtok(line, " "));
        client = std::stoi(strtok(NULL, " "));

        if (strcmp(machine, req.m) == 0 && client == req.c)
        {
            incarnationNum = std::stoi(strtok(NULL, " "));
            reqNum = std::stoi(strtok(NULL, " "));
            strcpy(operation, strtok(NULL, ":"));
            strcpy(resp, strtok(NULL, ";"));
            fseek(clientTable, 0, SEEK_SET);
            return resp;
        }
    }
    fseek(clientTable, 0, SEEK_SET);
    return NULL;
}

int getReqNum(struct request req)
{ //Incarnation check
    char line[300 + 1] = "";
    fseek(clientTable, 0, SEEK_SET);
    char machine[24];
    int incarnationNum, client, reqNum;
    while (fgets(line, sizeof(line), clientTable))
    {
        strcpy(machine, strtok(line, " "));
        client = std::stoi(strtok(NULL, " "));

        if (strcmp(machine, req.m) == 0 && client == req.c)
        {
            incarnationNum = std::stoi(strtok(NULL, " "));
            reqNum = std::stoi(strtok(NULL, " "));
            fseek(clientTable, 0, SEEK_SET);
            return reqNum;
        }
    }
    req.r = req.r - 1;
    addNewClient(req);
    req.r = req.r + 1;
    return req.r - 1;
}

int openF(struct request req)
{ //open
    for (int i = 0; i < 2; i++)
    {
        char mode[5];
        if (strcmp(req.openMode, "read") == 0)
            strcpy(mode, "r+");
        else if (strcmp(req.openMode, "write") == 0)
            strcpy(mode, "r+");
        else if (strcmp(req.openMode, "readwrite") == 0)
            strcpy(mode, "r+");
        else
        {
            printf(RED "Mode not found for opening.\n" RESET);
            return 0;
        }
        int fNumber = getFNumber(req.fileName);
        char line[300 + 1] = "";
        fseek(lockTable, 0, SEEK_SET);
        char fileNamel[10];
        char lockedByMachine[24], lockedForOperation[20];
        int lockedByClient, isLocked;
        while (fgets(line, sizeof(line), lockTable))
        {
            strcpy(fileNamel, strtok(line, " "));
            if (strcmp(fileNamel, req.fileName) == 0)
            { //////////////// File Entry Found in Lock Table ///////////
                isLocked = std::stoi(strtok(NULL, " "));
                if (isLocked == 1)
                { ////////////// File is locked ///////////////////
                    strcpy(lockedByMachine, strtok(NULL, " "));
                    lockedByClient = std::stoi(strtok(NULL, " "));
                    strcpy(lockedForOperation, strtok(NULL, " "));
                    if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
                    {
                        if ((strcmp(lockedForOperation, "read") == 0 || strcmp(lockedForOperation, "readwrite") == 0) && strcmp(req.openMode, "read") == 0)
                        {
                            char response[20];
                            strcpy(response, "<ALREADY-OPEN>");
                            adjustClientTable(req, response);
                            printf(YEL "File Already Open: %s\n" RESET,req.fileName); //update request number in client table
                            return 1;
                        }
                        else if ((strcmp(lockedForOperation, "write") == 0 || strcmp(lockedForOperation, "readwrite") == 0) && strcmp(req.openMode, "write") == 0)
                        {
                            char response[20];
                            strcpy(response, "<ALREADY-OPEN>");
                            adjustClientTable(req, response);
                            printf(YEL "File Already Open: %s\n" RESET,req.fileName); //update request number in client table
                            return 1;
                        }
                        else if (strcmp(lockedForOperation, "readwrite") == 0 && strcmp(req.openMode, "readwrite") == 0)
                        {
                            char response[20];
                            strcpy(response, "<ALREADY-OPEN>");
                            adjustClientTable(req, response);
                            printf(YEL "File Already Open: %s\n" RESET,req.fileName); //update request number in client table
                            return 1;
                        }
                        else
                        {
                            char response[20];
                            strcpy(response, "<LOCKED>");
                            adjustClientTable(req, response);
                            printf(RED "File is locked for %s operation!\n" RESET, lockedForOperation);
                            return 0;
                        }
                        break;
                    }
                    else
                    {
                        char response[20];
                        strcpy(response, "<LOCKED>");
                        adjustClientTable(req, response);
                        printf(RED "File is locked by someone else!\n" RESET);
                        return 0;
                        break;
                    }
                }
                else
                {
                    F[fNumber] = fopen(req.fileName, mode);
                    char newLine[300 + 1] = "";
                    getLockLine(newLine, req.fileName, req.m, req.c, req.i, req.openMode);
                    adjustLockTable(req.fileName, newLine);
                    //update request number and operation in client table and lock table
                    printf(GRN "%s:File Open Done\n" RESET,req.fileName);
                    char response[20];
                    strcpy(response, "<OPEN-DONE>");
                    adjustClientTable(req, response);
                    return 1;
                }
            }
        }
        addNewFile(req.fileName);
    }
    return 0;
}

int closeF(struct request req)
{ //close
    int fNumber = getFNumber(req.fileName);
    char line[300 + 1] = "";
    fseek(lockTable, 0, SEEK_SET);
    char fileNamel[10];
    char lockedByMachine[24], lockedForOperation[20];
    int lockedByClient, isLocked;
    while (fgets(line, sizeof(line), lockTable))
    {
        strcpy(fileNamel, strtok(line, " "));
        if (strcmp(fileNamel, req.fileName) == 0)
        { //////////////// File Entry Found in Lock Table ///////////
            //printf("\n4\n");

            isLocked = std::stoi(strtok(NULL, " "));
            if (isLocked == 1)
            { ////////////// File is locked ///////////////////

                strcpy(lockedByMachine, strtok(NULL, " "));
                lockedByClient = std::stoi(strtok(NULL, " "));
                strcpy(lockedForOperation, strtok(NULL, " "));
                if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
                {
                    fclose(F[fNumber]);
                    printf(GRN "%s: File Close Done\n" RESET, req.fileName);
                    char newLine[300 + 1] = "";
                    strcat(newLine, req.fileName);
                    strcat(newLine, " 0\n");
                    adjustLockTable(req.fileName, newLine);

                    char response[20];
                    strcpy(response, "<CLOSE-DONE>");
                    adjustClientTable(req, response);
                    //update locktable and update request number in client table
                    return 1;
                    break;
                }
                else
                {
                    printf(RED "File is locked by someone else!\n" RESET);
                    char response[20];
                    strcpy(response, "<LOCKED>");
                    adjustClientTable(req, response);
                    return 0;
                    break;
                }
            }
            else
            {
                char response[20];
                strcpy(response, "<ALREADY-CLOSED>");
                adjustClientTable(req, response);
                printf(YEL "The file is already closed!\n" RESET);
                return 0;
            }
        }
        //printf("\nl\n");
    }
    printf(RED "The file is not open!\n" RESET);
    return 0;
}

char *readF(struct request req)
{ //read
    int fNumber = getFNumber(req.fileName);
    char *readBytes;
    char requestOperation[20];
    strcpy(requestOperation, "read");
    char line[300 + 1] = "";
    fseek(lockTable, 0, SEEK_SET);
    char fileNamel[10];
    char lockedByMachine[24], lockedForOperation[20];
    int lockedByClient, isLocked;
    while (fgets(line, sizeof(line), lockTable))
    {
        strcpy(fileNamel, strtok(line, " "));
        if (strcmp(fileNamel, req.fileName) == 0)
        { //////////////// File Entry Found in Lock Table ///////////
            isLocked = std::stoi(strtok(NULL, " "));
            if (isLocked == 1)
            { ////////////// File is locked ///////////////////
                strcpy(lockedByMachine, strtok(NULL, " "));
                lockedByClient = std::stoi(strtok(NULL, " "));
                strcpy(lockedForOperation, strtok(NULL, " "));
                if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
                {
                    if (strcmp(lockedForOperation, "read") == 0 || strcmp(lockedForOperation, "readwrite") == 0)
                    {
                        //printf("\njust before read,%d,%d\n",fNumber,req.readOrLseek);

                        readBytes = readBlind(F[fNumber], req.readOrLseek);
                        char response[20];
                        strcpy(response, readBytes);
                        adjustClientTable(req, response);
                        printf(GRN "\"%s\": Read from file\n" RESET, readBytes);
                        return readBytes;
                    }
                    else
                    {
                        char response[20];
                        strcpy(response, "<LOCKED>");
                        adjustClientTable(req, response);
                        printf(RED "File is locked for %s operation!\n" RESET, lockedForOperation);
                        return NULL;
                    }
                    break;
                }
                else
                {

                    printf(RED "File is locked by someone else!\n" RESET);
                    char response[20];
                    strcpy(response, "<LOCKED>");
                    adjustClientTable(req, response);
                    return NULL;
                    break;
                }
            }
            else
            {
                printf(RED "The file is not open YET!\n" RESET);
                char response[20];
                strcpy(response, "<FILE-NOT-OPEN>");
                adjustClientTable(req, response);
                return NULL;
            }
        }
    }
    printf("File is not open Yet.");
    char response[20];
    strcpy(response, "<FILE-NOT-OPEN>");
    adjustClientTable(req, response);
    return NULL;
}

int writeF(struct request req)
{ //write
    int fNumber = getFNumber(req.fileName);
    char requestOperation[20];
    strcpy(requestOperation, "write");
    char line[300 + 1] = "";
    fseek(lockTable, 0, SEEK_SET);
    char fileNamel[10];
    char lockedByMachine[24], lockedForOperation[20];
    int lockedByClient, isLocked;
    while (fgets(line, sizeof(line), lockTable))
    {
        strcpy(fileNamel, strtok(line, " "));
        if (strcmp(fileNamel, req.fileName) == 0)
        { //////////////// File Entry Found in Lock Table ///////////
            isLocked = std::stoi(strtok(NULL, " "));
            if (isLocked == 1)
            { ////////////// File is locked ///////////////////
                strcpy(lockedByMachine, strtok(NULL, " "));
                lockedByClient = std::stoi(strtok(NULL, " "));
                strcpy(lockedForOperation, strtok(NULL, " "));
                if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
                {
                    if (strcmp(lockedForOperation, "write") == 0 || strcmp(lockedForOperation, "readwrite") == 0)
                    {
                        writeBlind(F[fNumber], req.writeString);
                        printf(GRN "\"%s\": Written to file\n" RESET, req.writeString);
                        char response[20];
                        strcpy(response, "<WRITE-DONE>");
                        adjustClientTable(req, response); //update request number in client table
                        return 1;
                    }
                    else
                    {
                        printf(RED "File is locked for %s operation!\n" RESET, lockedForOperation);
                        char response[20];
                        strcpy(response, "<LOCKED>");
                        adjustClientTable(req, response);
                        return 0;
                    }
                    break;
                }
                else
                {
                    printf(RED "File is locked by someone else!\n" RESET);
                    char response[20];
                    strcpy(response, "<LOCKED>");
                    adjustClientTable(req, response);
                    return 0;
                    break;
                }
            }
            else
            {
                
                printf(RED "The file is not open YET!\n" RESET);
                char response[20];
                strcpy(response, "<FILE-NOT-OPEN>");
                adjustClientTable(req, response);
                return 0;
            }
        }
    }
    printf(RED "The file is not open YET!\n" RESET);
    char response[20];
    strcpy(response, "<FILE-NOT-OPEN>");
    adjustClientTable(req, response);
    return 0;
}

int lseekF(struct request req)
{ //lseek

    int fNumber = getFNumber(req.fileName);
    char line[300 + 1] = "";
    fseek(lockTable, 0, SEEK_SET);
    char fileNamel[10];
    char lockedByMachine[24], lockedForOperation[20];
    int lockedByClient, isLocked;
    while (fgets(line, sizeof(line), lockTable))
    {
        strcpy(fileNamel, strtok(line, " "));
        if (strcmp(fileNamel, req.fileName) == 0)
        { //////////////// File Entry Found in Lock Table ///////////
            isLocked = std::stoi(strtok(NULL, " "));
            if (isLocked == 1)
            { ////////////// File is locked ///////////////////
                strcpy(lockedByMachine, strtok(NULL, " "));
                lockedByClient = std::stoi(strtok(NULL, " "));
                if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
                {
                    fseek(F[fNumber], req.readOrLseek, SEEK_SET);
                    
                    char response[20];
                    strcpy(response, "<LSEEK-DONE>");
                    printf(GRN "%s: File lseek done!\n" RESET, req.fileName);
                    adjustClientTable(req, response);
                    return 1;
                    break;
                }
                else
                {
                    printf(RED "File is locked by someone else!\n" RESET);
                    char response[20];
                    strcpy(response, "<LOCKED>");
                    adjustClientTable(req, response);
                    return 0;
                    break;
                }
            }
            else
            {
                printf(RED "The file is not open YET!\n" RESET);
                char response[20];
                strcpy(response, "<FILE-NOT-OPEN>");
                adjustClientTable(req, response);
                return 0;
            }
        }
    }
    printf(RED "The file is not open YET!\n" RESET);
    char response[20];
    strcpy(response, "<FILE-NOT-OPEN>");
    adjustClientTable(req, response);
    return 0;
}

void performRequest(struct request req)
{
    //printf("\n1\n");
    if (incarnationCheck(req) == 1)
    {
        if (strcmp(req.operationType, "open") == 0)
        {
            openF(req);
        }
        else if (strcmp(req.operationType, "close") == 0)
        {
            closeF(req);
        }
        else if (strcmp(req.operationType, "read") == 0)
        {
            readF(req);
        }
        else if (strcmp(req.operationType, "write") == 0)
        {
            writeF(req);
        }
        else if (strcmp(req.operationType, "lseek") == 0)
        {
            lseekF(req);
        }
        else
        {
            printf("\nInvalid Request\n");
        }
    }
    else
    {
        return;
    }
}

void getResponse(struct request req, char *responseStr)
{
    char line[300 + 1] = "";
    fseek(clientTable, 0, SEEK_SET);
    char machine[24];
    int incarnationNum, client, reqNum;
    char beforeSColon[300 + 1];
    char res[300 + 1];
    while (fgets(line, sizeof(line), clientTable))
    {
        strcpy(machine, strtok(line, " "));
        client = std::stoi(strtok(NULL, " "));

        if (strcmp(machine, req.m) == 0 && client == req.c)
        {
            strcpy(beforeSColon, strtok(NULL, ":"));
            strcpy(res, strtok(NULL, ";"));
            strcpy(responseStr, res);
            fseek(clientTable, 0, SEEK_SET);
            return;
        }
    }
    printf(RED "Response Not Found! for (%s,%d)\n" RESET, req.m, req.c);
}

void DieWithError(const char *errorMessage) /* External error handling function */
{
    perror(errorMessage);
    exit(1);
}
//
int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    char requestStr[ECHOMAX];        /* Buffer for echo string */
    char responseStr[ECHOMAX];       /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    struct request req;
    
    if (argc != 2) /* Test for correct number of parameters */
    {
        fprintf(stderr, "Usage:  %s <UDP SERVER PORT>\n", argv[0]);
        exit(1);
    }
    clrscr();
    initializeTables();
    printf(MAG "%20s %20s %20s\n" RESET, "", "SERVER", "");
    //liner();

    echoServPort = atoi(argv[1]); /* First arg:  local port */

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
    {
        DieWithError("bind() failed");
    }

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);

        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, requestStr, ECHOMAX, 0, (struct sockaddr *)&echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        requestStr[recvMsgSize] = '\0';
        liner();
        printf(BLU "Client IP:%s\n" RESET, inet_ntoa(echoClntAddr.sin_addr));
        

        ///////// Get Request object /////////

        req = deserializeReq(requestStr);
        printf(BLU "Request: %s\n\n" RESET, req.operation);
        int lastKnownReqNum = getReqNum(req);
        if (lastKnownReqNum > req.r)
        {
            printf(YEL "\nOld Request by %s,%d\n" RESET, req.m, req.c);
            //ignore
        }
        else if (lastKnownReqNum == req.r)
        {
            //resend response
            printf(YEL "Getting Response only!\n" RESET);
            char tempRes[80+1];
            getResponseOnly(req, tempRes);
            strcpy(responseStr,tempRes);
            if(sendto(sock, responseStr, strlen(responseStr), 0, (struct sockaddr *)&echoClntAddr, sizeof(echoClntAddr)) != strlen(responseStr))
                {
                    DieWithError("sendto() sent a different number of bytes than expected");
                }
        }
        else if (lastKnownReqNum < req.r)
        {
            
            //Perform Request
            srand(time(NULL));
            switch (rand() % 3)
            {
            // Drop Request
            case 0:
                performRequest(req); 
                getResponse(req,responseStr);
                // Send received datagram back to the client 
                if(sendto(sock, responseStr, strlen(responseStr), 0, (struct sockaddr *)&echoClntAddr, sizeof(echoClntAddr)) != strlen(responseStr))
                {
                    DieWithError("sendto() sent a different number of bytes than expected");
                }
                break;

            // Perform request but dont send response
            case 1:
                performRequest(req);
                break;

            case 2: 
                
                printf("\nUnlucky! Req Dropped.\n");
                break;
            }
        }
        else
        {
            printf(RED "Request Number Issue by %s,%d\n" RED, req.m, req.c);
        }
    }
    /* NOT REACHED */
}
