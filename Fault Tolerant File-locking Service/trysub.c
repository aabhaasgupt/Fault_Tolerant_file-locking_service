#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include "defns.h"
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <unistd.h>     /* for close() */
#include <sys/types.h>
#include "iostream"
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h> 
#include <sys/time.h>
#include <thread>

#define currentLogTopic       "currentLogTopic"
#define leaderTopic       "leader"
#define attendanceTopic       "attendanceTopic"
#define PAYLOAD     "Hello World!"
#define QOS         2
#define TIMEOUT     10000L
#define attendanceInterval  10

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
 
#define messageRequestType "requestLog"
#define messageAttendance "attendance"
#define nextLeader "nextLeader"
#define backupSeverIP "backupIP"
 
FILE *lockTable;
FILE *clientTable;
FILE *F[numOfFiles];
char CLIENTID[20];
bool isLeader;
volatile MQTTClient_deliveryToken deliveredtoken;
int sock;                        /* Socket */
struct sockaddr_in echoServAddr; /* Local address */
struct sockaddr_in echoClntAddr; /* Client address */
unsigned int cliAddrLen;         /* Length of incoming message */
char requestStr[ECHOMAX];        /* Buffer for echo string */
char responseStr[ECHOMAX];       /* Buffer for echo string */
unsigned short echoServPort;     /* Server port */
int recvMsgSize;                 /* Size of received message */
char ADDRESS[50];    

bool debugMode;
struct request req;
char serverIP[16];
MQTTClient client;
MQTTClient_connectOptions conn_opts;
MQTTClient_message pubmsg;
MQTTClient_message pubmsgAttendance;
MQTTClient_message pubmsgLeader;
MQTTClient_deliveryToken token;
MQTTClient_deliveryToken tokenAttendance;
MQTTClient_deliveryToken tokenLeader;
int rc;
int ch;
char nextLeaderList[1000];
char nextLeaderListOld[1000];
bool flagGlobal1;
bool flagGlobal2;
bool leadExists;
char currLeader[20];

////////////////////////////////////////////////////////////////////////////////////
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
        strcpy(rr.m, strtok(str, "-"));\
        rr.c = atoi(strtok(NULL, "-"));
        rr.i = atoi(strtok(NULL, "-"));
        rr.r = atoi(strtok(NULL, "-"));
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

            rr.readOrLseek = atoi(strtok(NULL, ":"));
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
            tempC = atoi(strtok(NULL, " "));
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

        return atoi(contents_chopped);
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

            isLocked = atoi(strtok(NULL, " "));
            if (isLocked == 1)
            { ////////////// File is locked ///////////////////
                strcpy(lockedByMachine, strtok(NULL, " "));
                lockedByClient = atoi(strtok(NULL, " "));
                strcpy(lockedForOperation, strtok(NULL, " "));
                if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
                {
                    incarnationNum = atoi(strtok(NULL, " "));
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
            isLocked = atoi(strtok(NULL, " "));
            if (isLocked == 1)
            { ////////////// File is locked ///////////////////
                strcpy(lockedByMachine, strtok(NULL, " "));
                lockedByClient = atoi(strtok(NULL, " "));
                strcpy(lockedForOperation, strtok(NULL, " "));
                if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
                {
                    //printf("\nenter\n");
                    incarnationNum = atoi(strtok(NULL, " "));
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
            client = atoi(strtok(NULL, " "));
            if (strcmp(machine, req.m) == 0 && client == req.c)
            {
                incarnationNum = atoi(strtok(NULL, " "));
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
            client = atoi(strtok(NULL, " "));

            if (strcmp(machine, req.m) == 0 && client == req.c)
            {
                incarnationNum = atoi(strtok(NULL, " "));
                reqNum = atoi(strtok(NULL, " "));
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
            client = atoi(strtok(NULL, " "));

            if (strcmp(machine, req.m) == 0 && client == req.c)
            {
                incarnationNum = atoi(strtok(NULL, " "));
                reqNum = atoi(strtok(NULL, " "));
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
                    isLocked = atoi(strtok(NULL, " "));
                    if (isLocked == 1)
                    { ////////////// File is locked ///////////////////
                        strcpy(lockedByMachine, strtok(NULL, " "));
                        lockedByClient = atoi(strtok(NULL, " "));
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

                isLocked = atoi(strtok(NULL, " "));
                if (isLocked == 1)
                { ////////////// File is locked ///////////////////

                    strcpy(lockedByMachine, strtok(NULL, " "));
                    lockedByClient = atoi(strtok(NULL, " "));
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
                isLocked = atoi(strtok(NULL, " "));
                if (isLocked == 1)
                { ////////////// File is locked ///////////////////
                    strcpy(lockedByMachine, strtok(NULL, " "));
                    lockedByClient = atoi(strtok(NULL, " "));
                    strcpy(lockedForOperation, strtok(NULL, " "));
                    if (strcmp(lockedByMachine, req.m) == 0 && lockedByClient == req.c)
                    {
                        if (strcmp(lockedForOperation, "read") == 0 || strcmp(lockedForOperation, "readwrite") == 0)
                        {
                            //printf("\njust before read,%d,%d\n",fNumber,req.readOrLseek);

                            readBytes = readBlind(F[fNumber], req.readOrLseek);
                            char response[20];
                            strcpy(response,"<");
                            strcat(response, readBytes);
                            strcat(response,">");
                            
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
                isLocked = atoi(strtok(NULL, " "));
                if (isLocked == 1)
                { ////////////// File is locked ///////////////////
                    strcpy(lockedByMachine, strtok(NULL, " "));
                    lockedByClient = atoi(strtok(NULL, " "));
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
                isLocked = atoi(strtok(NULL, " "));
                if (isLocked == 1)
                { ////////////// File is locked ///////////////////
                    strcpy(lockedByMachine, strtok(NULL, " "));
                    lockedByClient = atoi(strtok(NULL, " "));
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
            client = atoi(strtok(NULL, " "));

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
////////////////////////////////////////////////////////////////////////////////////

    bool messageUDPLeader(char *servIP)
    {
        printf("\nmessage UDP entered\n");
        struct timeval timeout = {2, 0}; //set timeout for 2 seconds

        int sock;                        /* Socket descriptor */
        struct sockaddr_in echoServAddrT; /* Echo server address */
        struct sockaddr_in fromAddrT;     /* Source address of echo */
        //unsigned short echoServPort;     /* Echo server port */
        unsigned int fromSizeT;           /* In-out of address size for recvfrom() */
        //char *servIP;                    /* IP address of server */
        char *echoString;                /* String to send to echo server */
        char echoBuffer[ECHOMAX + 1];    /* Buffer for receiving echoed string */
        int echoStringLen;               /* Length of string to echo */
        int yesNoLen;               /* Length of received response */
        char there[10], yesNo[10];
        strcpy(there,"<there?>");
        

        //servIP = leaderIP;            /* First arg: server IP address (dotted quad) */
        //echoServPort = atoi(argv[2]); /* Second arg: Use given port, if any */

        /* Create a datagram/UDP socket */
        if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            DieWithError("socket() failed");

        /* Construct the server address structure */

        memset(&echoServAddrT, 0, sizeof(echoServAddrT));   /* Zero out structure */
        echoServAddrT.sin_family = AF_INET;                /* Internet addr family */
        echoServAddrT.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
        echoServAddrT.sin_port = htons(echoServPort);      /* Server port */
        if (sendto(sock, there, strlen(there), 0, (struct sockaddr *)&echoServAddrT, sizeof(echoServAddrT)) != strlen(there))
            DieWithError("sendto() sent a different number of bytes than expected");
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
        printf("/%s/ sent",there);
        /* Recv a response */

        fromSizeT = sizeof(fromAddrT);
        printf("Reached before while\n",there);
        yesNoLen=-1;
        while (yesNoLen < 0)
        {
            yesNoLen = recvfrom(sock, yesNo, ECHOMAX, 0, (struct sockaddr *)&fromAddrT, &fromSizeT);
            if (yesNoLen >= 0)
            {
                //Message Received
            }
            else
            {
                //printf(BLU "Didn't get response, trying again..." RESET);
                printf(BLU "No response received, %s It may be down!\n" RESET, servIP);

                close(sock);
                return false;
                
            }
        }

        ///////////////////////////////////////////////

        /*
    	if ((respStringLen = recvfrom(sock, yesNo, ECHOMAX, 0, (struct sockaddr *) &fromAddrT, &fromSize)) > ECHOMAX )
        	DieWithError("recvfrom() failed");
        */
        yesNo[yesNoLen] = '\0';
        printf("it sent:\"%s\"",yesNo);
        if (echoServAddrT.sin_addr.s_addr != fromAddrT.sin_addr.s_addr)
        {
            printf(RED);
            fprintf(stderr, "eeError: received a packet from unknown source.\n");
            printf(RESET);
        }

        if(strcmp(yesNo,"yes")==0)
        {
            printf("The server:%s is up and running!\n",servIP);

            close(sock);
            return true;
        }


        close(sock);
        return false;
    }

    bool checkIPbool(const char *s)
    {
        int len = strlen(s);

        if (len < 7 || len > 15)
        {
            return false;
        }

        char tail[16];
        tail[0] = 0;

        unsigned int d[4];
        int c = sscanf(s, "%3u.%3u.%3u.%3u%s", &d[0], &d[1], &d[2], &d[3], tail);

        if (c != 4 || tail[0])
        {
            return false;
        }

        for (int i = 0; i < 4; i++)
            if (d[i] > 255)
                {
                    return false;
                }

        return true;
    }

    void delivered(void *context, MQTTClient_deliveryToken dt)
    {
        //printf("Message with token value %d delivery confirmed\n", dt);
        deliveredtoken = dt;
    }

    void backupServerPerformRequest(char *requestStr){
        struct request req = deserializeReq(requestStr);
        liner();
        printf(BLU "Backup Request: %s\n\n" RESET, req.operation);
        performRequest(req);
    }

    bool checkIfBackupIsInList(char *tempLeader,char *tLeaderList)
    {
        if(strcmp(tLeaderList,"")==0) return true;

        char tList[1000];
        strcpy(tList,tLeaderList);
        char * token;
        char * rest;
        rest= tList;
        while((token=strtok_r(rest, ",", &rest)))
        {
            if(strcmp(token,tempLeader)==0) return false;
        }
        return true;
    }

    void messageDeserialize(char *message, char *topicName)
    {
        if(strcmp(topicName,currentLogTopic)==0)
        {

            char messagecopy[ECHOMAX];
            strcpy(messagecopy, message);
            char fromIP[20];
            strcpy(fromIP, strtok(messagecopy, "|"));
            char requestStr[ECHOMAX];
            strcpy(requestStr,strtok(NULL,";"));
            if(strcmp(fromIP,serverIP)!=0)
            {
                backupServerPerformRequest(requestStr);
            }
        }
        else if(strcmp(topicName,attendanceTopic)==0)
        {
            char messagecopy[ECHOMAX];
            strcpy(messagecopy, message);
            char messageType[30];
            strcpy(messageType, strtok(messagecopy, "|"));
            if(strcmp(messageType,messageAttendance)==0)
            {
                char tempSerIP[20];
                strcpy(tempSerIP,strtok(NULL,";"));
                //printf(GRN "\nmessageType:%s, messageAttendance=%s, tempSerIP:%s \n",messageType, messageAttendance, tempSerIP);
                //printf( RESET );
                if(checkIPbool(tempSerIP)){
                    if(strcmp(tempSerIP,serverIP)==0)
                    {
                        
                    }
                    else
                    {
                        strcpy(currLeader,tempSerIP);
                        //isLeader=false;
                        char backupIP[30];
                        strcpy(backupIP,backupSeverIP);
                        strcat(backupIP,"|");
                        strcat(backupIP,serverIP);
                        strcat(backupIP,";");
                        pubmsgAttendance.payload=backupIP;
                        if(debugMode)
                            printf(RED "\nbIP:%s, serIP:%s\n" RESET,backupIP,serverIP);
                        pubmsgAttendance.payloadlen = strlen(backupIP);
                        pubmsgAttendance.qos = QOS;
                        pubmsgAttendance.retained = 0;
                        MQTTClient_publishMessage(client, attendanceTopic, &pubmsgAttendance, &tokenAttendance);
                        
                        /*
                        printf("Waiting for up to %d seconds for publication of %s\n"
                            "on topic %s for client with ClientID: %s\n",
                            (int)(TIMEOUT / 1000), requestStr, attendanceTopic, CLIENTID);
                        */

                        ///////////////////////////////////////////////////////////////rc = MQTTClient_waitForCompletion(client, tokenAttendance, TIMEOUT);
                        //printf("Message with delivery token %d delivered\n", tokenAttendance);
                    }
                }
            }
            else if(strcmp(messageType,backupSeverIP)==0)
            {
                if(isLeader)
                {
                    char tempLeader[20];
                    strcpy(tempLeader,strtok(NULL,";"));
                    if(debugMode)
                        printf(BLU "The Temp Next Leader=/%s/ " RESET,tempLeader);
                    if(checkIfBackupIsInList(tempLeader,nextLeaderList))
                    {
                        strcat(nextLeaderList,tempLeader);
                        strcat(nextLeaderList,",");
                    }

                    if(debugMode)
                        printf(YEL "\nNext Leader List:%s\n" RESET,nextLeaderList);
                    
                    
                    char tempNextLeaderList[1000+30];
                    strcpy(tempNextLeaderList, nextLeader);
                    strcat(tempNextLeaderList, "|");
                    strcat(tempNextLeaderList, nextLeaderList);
                    strcat(tempNextLeaderList, ";");
                    pubmsgAttendance.payload = tempNextLeaderList;
                    pubmsgAttendance.payloadlen = strlen(tempNextLeaderList);
                    pubmsgAttendance.qos = QOS;
                    pubmsgAttendance.retained = 0;
                    MQTTClient_publishMessage(client, attendanceTopic, &pubmsgAttendance, &tokenAttendance);
                    
                    if(debugMode)
                        printf(BLU "Sending next list to others:=%s= \n " RESET,tempNextLeaderList);

                    /*
                        printf("Waiting for up to %d seconds for publication of %s\n"
                            "on topic %s for client with ClientID: %s\n",
                            (int)(TIMEOUT / 1000), requestStr, attendanceTopic, CLIENTID);
                    */

                    ///////////////////////////////////////////////////////////////rc = MQTTClient_waitForCompletion(client, tokenAttendance, TIMEOUT);
                    ////////////////////////////////////////////////////////////////////////////////////
                    /////////////////////// send to all others.....
                    ////////////////////////////////////////////////////////////////////////////////////
                    ////////////////////////////////////////////////////////////////////////////////////
                    ////////////////////////////////////////////////////////////////////////////////////
                }
            }
            else if(strcmp(messageType,nextLeader)==0)
            {
                    strcpy(nextLeaderList,strtok(NULL,";"));

                    if(debugMode)
                        printf(YEL "\nBackup Next Leader List:%s\n" RESET,nextLeaderList);
            }
        }
        else if(strcmp(topicName,leaderTopic)==0)
        {
            char messagecopy[ECHOMAX];
            strcpy(messagecopy, message);
            char messageType[30];
            strcpy(messageType, strtok(messagecopy, "|"));
            if(strcmp(messageType,leaderTopic)==0)
            {
                char tempSerIP[20];
                strcpy(tempSerIP,strtok(NULL,";"));
                if(checkIPbool(tempSerIP)){
                    if(strcmp(tempSerIP,serverIP)==0)
                    {
                        
                    }
                    else
                    {
                        if(isLeader)
                        {
                            char backupIP[30];
                            strcpy(backupIP,"leaderExists|");
                            strcat(backupIP,serverIP);
                            strcat(backupIP,";");
                            
                            
                            pubmsg.payload = backupIP;
                            pubmsg.payloadlen = strlen(backupIP);
                            pubmsg.qos = QOS;
                            pubmsg.retained = 0;
                            MQTTClient_publishMessage(client, leaderTopic, &pubmsg, &token);
                        }
                    }
                }
            }
            else if(strcmp(messageType,"leaderExists")==0)
            {
                char tempSerIP[20];
                strcpy(tempSerIP,strtok(NULL,";"));
                if(checkIPbool(tempSerIP)){
                    if(flagGlobal2==false)
                    {
                        if (strcmp(tempSerIP, serverIP) == 0)
                        {

                        }
                        else
                        {
                            
                            strcpy(currLeader,tempSerIP);
                            
                            if(debugMode)
                                printf(RED "Leader Exists :( so I am not\n" RESET);
                            leadExists=true;
                            isLeader = false;
                        }
                    }
                }
            }
        }
        else if(strcmp(topicName,serverIP)==0)
        {

            //printf("The message I got is %s\n",message);
            char messagecopy[ECHOMAX];
            strcpy(messagecopy, message);
            char messageType[30];
            strcpy(messageType, strtok(messagecopy, "|"));
            //printf("The message I got is %s\n",messageType);
            if(strcmp(messageType,"<There?>")==0)
            {
                char responseIP[20];
                strcpy(responseIP,strtok(NULL,";"));
                char Yes[5];
                strcpy(Yes,"Yes|");
                pubmsg.payload=Yes;
                pubmsg.payloadlen = strlen(Yes);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, responseIP, &pubmsg, &token);
                if(debugMode)
                    printf("sending Yes!!");
            }
            else if(strcmp(messageType,"Yes")==0)
            {
                flagGlobal1=true;
            }
        }
    }

    int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
    {
        int i;
        char* payloadptr;
        char myMessage[200];
        //printf("\nMessage arrived\n");
        //printf("Message Len=%d\n",message->payloadlen);
        //printf("     topic: %s\n", topicName);
        //printf("   message: ");
        payloadptr = (char*) message->payload;
        int  len=message->payloadlen;
        for(i=0; i<=len; i++)
        {
            //printf("%d\n",i);
            //putchar(*payloadptr);
            myMessage[i]=*payloadptr++;
            myMessage[i+1]='\0';
            //putchar(myMessage[i]);
            if(i==len)
            {
                //a();
            }
        }
        messageDeserialize(myMessage,topicName);
        //printf("My message =.%s.",myMessage);
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }

    void connlost(void *context, char *cause)
    {
        printf("\nConnection lost\n");
        printf("     cause: %s\n", cause);
        conn_opts = MQTTClient_connectOptions_initializer;
        pubmsg = MQTTClient_message_initializer;
        pubmsgAttendance = MQTTClient_message_initializer;
        
        MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
        if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }
    
    void checkIP(const char *s)
    {
        int len = strlen(s);

        if (len < 7 || len > 15)
        {
            printf("Usage: <SERVER  IP ADDRESS> <UDP SERVER PORT>\n");
            printf("Please enter a valid IP address\n");
            exit(0);

        }

        char tail[16];
        tail[0] = 0;

        unsigned int d[4];
        int c = sscanf(s, "%3u.%3u.%3u.%3u%s", &d[0], &d[1], &d[2], &d[3], tail);

        if (c != 4 || tail[0])
        {
            printf("Usage: <SERVER  IP ADDRESS> <UDP SERVER PORT>\n");
            printf("Please enter a valid IP address\n");
        exit(0);
        }

        for (int i = 0; i < 4; i++)
            if (d[i] > 255)
                {
                    printf("Usage: <SERVER  IP ADDRESS> <UDP SERVER PORT>\n");
                    printf("Please enter a valid IP address\n");
                    exit(0);
                }

        return;
    }
    

    bool messageMQTTLeader(char * assumedLeaderIP)
    {

        flagGlobal1=false;
        char there[10];
        strcpy(there,"<There?>");
        strcat(there,"|");
        strcat(there,serverIP);
        strcat(there,";");

        pubmsg.payload = there;
        pubmsg.payloadlen = strlen(there);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, assumedLeaderIP, &pubmsg, &token);

        /*printf("Waiting for up to %d seconds for publication of %s\n"
                "on topic %s for client with ClientID: %s\n",
                (int)(TIMEOUT/1000), PAYLOAD, leaderTopic, CLIENTID);*/
        
        ///////////////////////////////////////////////////////////////rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        if(flagGlobal1)
        {
            printf("flagGlobal1=yes\n");
        }
        else if(!flagGlobal1)
        {
            printf("flagGlobal1=no");
        }
        else{
            printf("flagGlobal1=Null");
        }
        for(int x=0;x<(attendanceInterval*5);x++)
        {
            printf("%d\n",x);
            sleep(1);
            if(flagGlobal1==true)
            {
                printf("sending true;\n");
                flagGlobal1=false;
                return true;

                printf("sending true too much;\n");
            }
        }

        if(flagGlobal1)
        {
            printf("now flagGlobal1=yes\n");
        }
        else if(!flagGlobal1)
        {
            printf("now flagGlobal1=no");
        }
        else{
            printf("now flagGlobal1=Null");
        }
        if(flagGlobal1==true)
        {

            flagGlobal1=false;
            return true;
        }

        flagGlobal1=false;
        return false;
    }
    
    char *getNextAssumedLeader( char *nextLeaderListRef)
    {
        char *assumedLeader;
        int asLen;
        int nLen;
        char nextList[1000];
        char * n;
        n=nextList;
        //printf("i got /%s/,/%s/\n",assumedLeader,nextLeaderListRef);
        strcpy(nextList,nextLeaderListRef);
        
        assumedLeader=strtok_r(n, ",", &n);
        //printf("hii --%s--\n",assumedLeader);
        if(strcmp(assumedLeader,"")!=0)
        {
            asLen=strlen(assumedLeader)+1;
            nLen=strlen(nextLeaderListRef);
            //printf("as=%d,n=%d\n",asLen,nLen);
            
            for(int x=0;x<nLen-asLen;x++)
            {
                nextLeaderListRef[x]=nextLeaderListRef[x+asLen];
            }
            nextLeaderListRef[nLen-asLen]='\0';
        }
        return assumedLeader;
    }
    
    /*
        bool checkIfLeader()
        {
            printf("checkifLeader Entered\n");
            if (isLeader)
            {

                printf("I am the Leader and am performaing the request.\n");
                return true;
            }
            char tNextLeaderList[1000];
            strcpy(tNextLeaderList,currLeader);
            strcat(tNextLeaderList,",");
            strcat(tNextLeaderList, nextLeaderList);
            if (strcmp(tNextLeaderList, "") == 0)
            {
                printf("The tnextLeaderList is empty:\" \",=%s=\n", tNextLeaderList);
                isLeader = true;
                return true;
            }
            char *assumedLeader;
            
            while (strcmp(tNextLeaderList, "") != 0 && strcmp(tNextLeaderList, ",")!= 0 && strcmp(tNextLeaderList, ";")!= 0 && strcmp(tNextLeaderList, ",;") != 0)
            {   
                assumedLeader=getNextAssumedLeader(tNextLeaderList);
                printf("while entered! assumed leader=|%s|\n", assumedLeader);
                if (strcmp(assumedLeader, serverIP) == 0)
                {
                    isLeader=true;
                    return true;
                }

                
                flagGlobal1 = false;
                char there[10];
                strcpy(there, "<There?>");
                strcat(there, "|");
                strcat(there, serverIP);
                strcat(there, ";");

                pubmsg.payload = there;
                pubmsg.payloadlen = strlen(there);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, assumedLeader, &pubmsg, &token);
                printf("sentThere\n");
                

                rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
                                    if (flagGlobal1)
                                    {
                                        printf("flagGlobal1=yes\n");
                                    }
                                    else if (!flagGlobal1)
                                    {
                                        printf("flagGlobal1=no");
                                    }
                                    else
                                    {
                                        printf("flagGlobal1=Null");
                                    }
                for (int x = 0; x <= (attendanceInterval * 5); x++)
                {
                    printf("%d\n", x);
                    sleep(1);
                    if (flagGlobal1 == true)
                    {
                        printf("sending true;\n");
                        break;
                    }
                }

                    printf("assumed leader=/%s/\n", assumedLeader);
                    if (flagGlobal1 == true)
                    {
                        printf("inserted");
                        printf(RED "Leader exists! " RESET);
                        return false;
                    }
                    printf("not inserted");
                    //strcpy(nextAssumedLeader, strtok(NULL, ","));
                    //printf("now the next assumed leader is %s\n", nextAssumedLeader);
                    flagGlobal1 = false;
            }
            return false;


        }
    
    
        bool checkIfLeader()
        {
            /////////////////// Make connection via udp
            printf("checkifLeader Entered\n");
            if (isLeader)
            {
                printf("I am the Leader and am performaing the request.\n");
                return true;
            }
            char tNextLeaderList[1000];
            strcpy(tNextLeaderList,currLeader);
            strcat(tNextLeaderList,",");
            strcat(tNextLeaderList, nextLeaderList);
            char* assumedLeader; 
            char* rest = tNextLeaderList;
            printf("the entire list is %s/\n",rest);
            if (strcmp(rest, "") == 0)
            {
                printf("The tnextLeaderList is empty:\" \",=%s=\n", tNextLeaderList);
                isLeader = true;
                return true;
            }

            while((assumedLeader = strtok_r(rest, ",", &rest)))
            {
                if (strcmp(assumedLeader, serverIP) != 0)
                {
                    printf("while entered! assumed leader=%s\n", assumedLeader);
                    flagGlobal1 = false;
                    char there[10];
                    strcpy(there, "<There?>");
                    strcat(there, "|");
                    strcat(there, serverIP);
                    strcat(there, ";");

                    pubmsg.payload = there;
                    pubmsg.payloadlen = strlen(there);
                    pubmsg.qos = QOS;
                    pubmsg.retained = 0;
                    MQTTClient_publishMessage(client, assumedLeader, &pubmsg, &token);

                    /*printf("Waiting for up to %d seconds for publication of %s\n"
                        "on topic %s for client with ClientID: %s\n",
                        (int)(TIMEOUT/1000), PAYLOAD, leaderTopic, CLIENTID);

                    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
                            if (flagGlobal1)
                            {
                                printf("flagGlobal1=yes\n");
                            }
                            else if (!flagGlobal1)
                            {
                                printf("flagGlobal1=no");
                            }
                            else
                            {
                                printf("flagGlobal1=Null");
                            }
                    for (int x = 0; x < (attendanceInterval * 5); x++)
                    {
                        printf("%d\n", x);
                        sleep(1);
                        if (flagGlobal1 == true)
                        {
                            printf("sending true;\n");
                            break;
                        }
                    }
                    printf("assumed leader=/%s/\n", assumedLeader);
                    if (flagGlobal1 == true)
                    {
                        printf("inserted");
                        printf(RED "Leader exists! " RESET);
                        return false;
                    }
                    printf("not inserted");
                    //strcpy(nextAssumedLeader, strtok(NULL, ","));
                    flagGlobal1 = false;
                    
                }
                else
                {
                    isLeader = true;
                    //tell evreyone;
                    return true;
                }
            }
            return false;
            /*
            char assumedLeader[20];
            strcpy(assumedLeader, currLeader);
            char nextAssumedLeader[20];
            strcpy(nextAssumedLeader, strtok(tNextLeaderList, ","));
            while (strcmp(tNextLeaderList, "") != 0 || strcmp(tNextLeaderList, ",") != 0)
            {
                printf("while entered! assumed leader=%s\n", assumedLeader);
                flagGlobal1 = false;
                char there[10];
                strcpy(there, "<There?>");
                strcat(there, "|");
                strcat(there, serverIP);
                strcat(there, ";");

                pubmsg.payload = there;
                pubmsg.payloadlen = strlen(there);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, assumedLeader, &pubmsg, &token);

                /*printf("Waiting for up to %d seconds for publication of %s\n"
                    "on topic %s for client with ClientID: %s\n",
                    (int)(TIMEOUT/1000), PAYLOAD, leaderTopic, CLIENTID);

                rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
                                    if (flagGlobal1)
                                    {
                                        printf("flagGlobal1=yes\n");
                                    }
                                    else if (!flagGlobal1)
                                    {
                                        printf("flagGlobal1=no");
                                    }
                                    else
                                    {
                                        printf("flagGlobal1=Null");
                                    }
                for (int x = 0; x < (attendanceInterval * 5); x++)
                {
                    printf("%d\n", x);
                    sleep(1);
                    if (flagGlobal1 == true)
                    {
                        printf("sending true;\n");
                        break;
                    }
                }

                if (strcmp(assumedLeader, serverIP) != 0)
                {
                    printf("assumed leader=/%s/\n", assumedLeader);
                    if (flagGlobal1 == true)
                    {
                        printf("inserted");
                        printf(RED "Leader exists! " RESET);
                        return false;
                    }
                    printf("not inserted");
                    //strcpy(nextAssumedLeader, strtok(NULL, ","));
                    printf("now the next assumed leader is %s\n", nextAssumedLeader);
                    flagGlobal1 = false;
                    strcpy(assumedLeader, nextAssumedLeader);
                    if (strcmp(nextLeaderList, "") == 0 || strcmp(nextLeaderList, ",") == 0)
                    {
                        strcpy(nextAssumedLeader, strtok(NULL, ","));
                    }
                    else
                    {
                        isLeader = true;
                        //tell evreyone;
                        return true;
                    }
                }
                else
                {
                    isLeader = true;
                    //tell evreyone;
                    return true;
                }
            }
            return false;
        }
    */
        ///best
        bool checkIfLeader()
        {

            /////////////////// Make connection via udp
            //printf("checkifLeader Entered\n");
            if (isLeader)
            {

                //printf("I am the Leader and am performaing the request.\n");
                return true;
            }
            char tNextLeaderList[1000];
            strcpy(tNextLeaderList, nextLeaderList);
            if (strcmp(tNextLeaderList, "") == 0)
            {
                if(debugMode)
                printf("The tnextLeaderList is empty:\" \",=%s=\n", tNextLeaderList);
                isLeader = true;
                return true;
            }

            char assumedLeader[20];
            strcpy(assumedLeader, currLeader);
            char nextAssumedLeader[20];
            strcpy(nextAssumedLeader, strtok(tNextLeaderList, ","));
            while (strcmp(tNextLeaderList, "") != 0 || strcmp(tNextLeaderList, ",") != 0)
            {
                if(debugMode)
                printf("Sending \"There?\" while entered! assumed leader=%s\n", assumedLeader);
                flagGlobal1 = false;
                char there[10];
                strcpy(there, "<There?>");
                strcat(there, "|");
                strcat(there, serverIP);
                strcat(there, ";");

                pubmsg.payload = there;
                pubmsg.payloadlen = strlen(there);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, assumedLeader, &pubmsg, &token);

                //printf("Waiting for up to %d seconds for publication of %s\n"
                  //  "on topic %s for client with ClientID: %s\n",
                  //  (int)(TIMEOUT/1000), PAYLOAD, leaderTopic, CLIENTID);

                ///////////////////////////////////////////////////////////////rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
                                    if (flagGlobal1)
                                    {
                                        if(debugMode)
                                            printf("flagGlobal1=yes\n");
                                    }
                                    else if (!flagGlobal1)
                                    {
                                        if(debugMode)
                                            printf("flagGlobal1=no");
                                    }
                                    else
                                    {
                                        if(debugMode)
                                            printf("flagGlobal1=Null");
                                    }
                for (int x = 0; x < (attendanceInterval * 2); x++)
                {
                    if(debugMode)
                        printf("%d\n", x);
                    sleep(1);
                    if (flagGlobal1 == true)
                    {
                        //printf("sending true;\n");
                        break;
                    }
                }
                if(flagGlobal1==true) 
                {
                    return true;
                }

                isLeader = true;

                return false;
                
            }
            return false;
        }
    
    int mainFuctionThread()
    {

        ////// List of topics to subscribe ////////
        MQTTClient_subscribe(client, currentLogTopic, QOS);
        MQTTClient_subscribe(client, leaderTopic, QOS);
        MQTTClient_subscribe(client, "elseT", QOS);
        MQTTClient_subscribe(client, attendanceTopic, QOS);
        MQTTClient_subscribe(client, serverIP, QOS);
        ///////////////////////////////////////////
        //getchar();
        
        //////////////////////////// Iff Leader, then this will be done.... //////////////////////////////////
            
            clrscr();
            initializeTables();

            clrscr();
            printf(MAG "%20s %20s %20s\n" RESET, "", "SERVER", "");
            printf(MAG"\nSERVER IP=%s | Port=%d" RESET,serverIP,echoServPort);
            liner();
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
                
                    if(!isLeader)
                    {
                        char pleaseWait[20];
                        strcpy(pleaseWait, "pleasewait");
                        
                        printf(RED "\nPlease wait while old leader is checked." RESET);
                        if (sendto(sock, pleaseWait, strlen(pleaseWait), 0, (struct sockaddr *)&echoClntAddr, sizeof(echoClntAddr)) != strlen(pleaseWait))
                        {
                            DieWithError("sendto() sent a different number of bytes than expected");
                        }

                    }
                    checkIfLeader();
                    if(flagGlobal1==false)
                    {
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
                            strcat(responseStr,":");
                            strcat(responseStr,nextLeaderList);
                            strcat(responseStr,";");
                            
                            if(sendto(sock, responseStr, strlen(responseStr), 0, (struct sockaddr *)&echoClntAddr, sizeof(echoClntAddr)) != strlen(responseStr))
                                {
                                    DieWithError("sendto() sent a different number of bytes than expected");
                                }
                        }
                        else if (lastKnownReqNum < req.r)
                        {

                            char backupRequestStr[ECHOMAX+30];
                            //Perform Request
                            srand(time(NULL));
                            switch (rand() % 3)
                            {
                                // Perform request and send response
                                case 0:
                                    performRequest(req);
                                    getResponse(req,responseStr);

                                    strcat(responseStr,":");
                                    strcat(responseStr,nextLeaderList);
                                    strcat(responseStr,";");
                                    // Send received datagram back to the client 
                                    if(sendto(sock, responseStr, strlen(responseStr), 0, (struct sockaddr *)&echoClntAddr, sizeof(echoClntAddr)) != strlen(responseStr))
                                    {
                                        DieWithError("sendto() sent a different number of bytes than expected");
                                    }
                                    strcpy(backupRequestStr,"");
                                    strcpy(backupRequestStr,serverIP);
                                    strcat(backupRequestStr,"|");
                                    strcat(backupRequestStr,requestStr);
                                    strcat(backupRequestStr,";");
                                    pubmsg.payload = backupRequestStr;
                                    pubmsg.payloadlen = strlen(backupRequestStr);
                                    pubmsg.qos = QOS;
                                    pubmsg.retained = 0;
                                    MQTTClient_publishMessage(client, currentLogTopic, &pubmsg, &token);
                                    /*
                                    printf("Waiting for up to %d seconds for publication of %s\n"
                                            "on topic %s for client with ClientID: %s\n",
                                            (int)(TIMEOUT/1000), requestStr, currentLogTopic, CLIENTID);
                                    */
                                    ///////////////////////////////////////////////////////////////rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
                                    //printf("Message with delivery token %d delivered\n", token); 
                                    
                                    break;

                                // Perform request but dont send response
                                case 1:
                                    performRequest(req);
                                    strcpy(backupRequestStr,"");
                                    strcpy(backupRequestStr,serverIP);
                                    strcat(backupRequestStr,"|");
                                    strcat(backupRequestStr,requestStr);
                                    strcat(backupRequestStr,";");
                                    pubmsg.payload = backupRequestStr;
                                    pubmsg.payloadlen = strlen(backupRequestStr);
                                    pubmsg.qos = QOS;
                                    pubmsg.retained = 0;
                                    MQTTClient_publishMessage(client, currentLogTopic, &pubmsg, &token);
                                    /*
                                    printf("Waiting for up to %d seconds for publication of %s\n"
                                            "on topic %s for client with ClientID: %s\n",
                                            (int)(TIMEOUT/1000), requestStr, currentLogTopic, CLIENTID);
                                    */
                                    ///////////////////////////////////////////////////////////////rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
                                    //printf("Message with delivery token %d delivered\n", token);
                                    break;

                                // Drop Request
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
                    else
                    {
                        char pleaseWait[20];
                        strcpy(pleaseWait, "leaderExists|");
                        strcat(pleaseWait, currLeader);
                        strcat(pleaseWait, "]");
                        printf("sending <%s>\n",pleaseWait);
                        if (sendto(sock, pleaseWait, strlen(pleaseWait), 0, (struct sockaddr *)&echoClntAddr, sizeof(echoClntAddr)) != strlen(pleaseWait))
                        {
                            DieWithError("sendto() sent a different number of bytes than expected");
                        }
                    }
                    flagGlobal1=false;
                
                
            }

        /////////////////////////////////////////////////////////////////////////////////////////////////////

        do
        {
            ch = getchar();
        } while(ch!='Q' && ch != 'q');
        MQTTClient_disconnect(client, 10000);
        MQTTClient_destroy(&client);
        return rc;
    }

    void *mainFuctionThread(void *vargp) 
    { 
        mainFuctionThread();
        return NULL;
    }

    void *attendanceFunction(void *vargp) 
    {
        flagGlobal2=false;
        //getchar();
        if(debugMode)
            printf(YEL "\nWho is the leader?\n" RESET);
        char leadercheck[30];
        strcpy(leadercheck,"");
        strcpy(leadercheck,leaderTopic);
        strcat(leadercheck,"|");
        strcat(leadercheck,serverIP);
        strcat(leadercheck,";");
        if(debugMode)
            printf("leaderCheck={%s}",leadercheck);
        pubmsg.payload = leadercheck;
        pubmsg.payloadlen = strlen(leadercheck);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, leaderTopic, &pubmsg, &token);

        /*printf("Waiting for up to %d seconds for publication of %s\n"
                "on topic %s for client with ClientID: %s\n",
                (int)(TIMEOUT/1000), PAYLOAD, leaderTopic, CLIENTID);*/
        
        ///////////////////////////////////////////////////////////////rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);

        char alive[20];
        strcpy(alive, messageAttendance);
        strcat(alive, "|");
        strcat(alive, serverIP);
        strcat(alive, ";");
        for(int f=0;f<attendanceInterval*2;f++)
        {
            std::this_thread::sleep_for (std::chrono::seconds(1));

            if(debugMode)
                printf(RED "%d\n" RESET,f);

            if(leadExists)
            {
                break;
            }
            //sleep(1);
        }
        if(debugMode)
            printf("\nwaited %d Seconds\n",attendanceInterval);

        flagGlobal2=true;
        if(isLeader) 
        {   
            if(debugMode)
                printf("isLeader?=yes");
        }
        else if(!isLeader) 
        {
            if(debugMode)
                printf("isLeader?=No");
            clrscr();
            printf(RED "%20s %20s %20s\n" RESET, "", "BACKUP SERVER", "");
            printf(MAG"\nSERVER IP=%s | Port=%d | Leader IP=%s " RESET,serverIP,echoServPort,currLeader);
            liner();
        }
        while(1)
        {
            while(isLeader)
            {

                strcpy(currLeader,serverIP);
                strcpy(nextLeaderListOld,""); 
                strcpy(nextLeaderListOld,nextLeaderList);
                strcpy(nextLeaderList,"");
                if(debugMode)
                printf(YEL "\nRequesting Attendance %s: %s\n" RESET,serverIP, alive);
                //printf( RESET );
                pubmsgAttendance.payload=alive;
                pubmsgAttendance.payloadlen = strlen(alive);
                pubmsgAttendance.qos = QOS;
                pubmsgAttendance.retained = 0;
                MQTTClient_publishMessage(client, attendanceTopic, &pubmsgAttendance, &tokenAttendance);
                /*printf("Waiting for up to %d seconds for publication of %s\n"
                    "on topic %s for client with ClientID: %s\n",
                    (int)(TIMEOUT / 1000), alive, attendanceTopic, CLIENTID);
                */
                ///////////////////////////////////////////////////////////////rc = MQTTClient_waitForCompletion(client, tokenAttendance, TIMEOUT);
                //printf("Message with delivery token %d delivered\n", tokenAttendance);
                for(int l=0;l<attendanceInterval;l++)
                {
                    //sleep(1);
                    std::this_thread::sleep_for (std::chrono::seconds(1));
    
                    if(debugMode)
                        printf(YEL "%d\n" RESET,l);
                }
            }
        }
        return NULL;
    }

    int main(int argc, char* argv[])
    {
        isLeader=true;
        leadExists=false;
        if (argc < 4) /* Test for correct number of parameters */
        {
            fprintf(stderr, "Usage:  %s <SERVER  IP ADDRESS> <UDP SERVER PORT> <MQTT BROKER IP ADDRESS> <option>\n", argv[0]);
            exit(1);
        }

        if (argc == 5) /* Test for correct number of parameters */
        {
                printf("\nDebug Mode. Please press Enter\n");
                getchar();
            
            if(strcmp(argv[4],"-f")==0)
            {
                debugMode=true;
            }
            
        }

        strcpy(ADDRESS,"tcp://");
        strcat(ADDRESS,argv[3]);
        strcat(ADDRESS,":1883");
        
        strcpy(serverIP,argv[1]);
        echoServPort = atoi(argv[2]);
        checkIP(argv[1]);
        strcpy(CLIENTID, serverIP);
        conn_opts = MQTTClient_connectOptions_initializer;
        pubmsg = MQTTClient_message_initializer;
        pubmsgAttendance = MQTTClient_message_initializer;
        MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
        if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
        /*
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
            "Press Q<Enter> to quit\n\n", currentLogTopic, CLIENTID, QOS);
        */



        pthread_t main_thread_id; 
        pthread_t attendance_thread_id; 
        
        if(debugMode)
        printf("Before Thread\n"); 

        pthread_create(&main_thread_id, NULL, mainFuctionThread, NULL); 
        pthread_create(&attendance_thread_id, NULL, attendanceFunction, NULL); 
        pthread_join(main_thread_id, NULL); 
        pthread_join(attendance_thread_id, NULL); 
    }
