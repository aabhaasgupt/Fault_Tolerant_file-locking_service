#include "defns.h"
#include <iostream>
#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <sys/time.h>

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"
#define ECHOMAX 255 /* Longest string to echo */
#define ITERATIONS 5

FILE *scriptFile;
char machine[24];
int clientNumber;
int requestNumber;
int incarnationNumber;
char nextLeaderList[1000];
struct request req;

void clrscr()
{
    system("@cls||clear");
}
void liner()
{

    printf("\n" MAG);
    for (int i = 0; i < 80; i++)
    {
        printf("=");
    }
    printf(RESET "\n");
}
void initializeClient(int i)
{
    char in[50];
    snprintf(in, sizeof(in), "%d", i);
    char fileName[20];
    strcpy(fileName, "script");
    strcat(fileName, in);
    strcat(fileName, ".txt");

    //printf("\nfileName=%s\n",fileName);
    scriptFile = fopen(fileName, "r");
}

struct request getReqObj(const char *m, int c, int i, int r, const char *operation)
{
    struct request rr;
    strcpy(rr.m, m);
    rr.c = c;
    rr.i = i;
    rr.r = r;
    strcpy(rr.operation, operation);
    return rr;
}
void DieWithError(const char *errorMessage) /* External error handling function */
{
    perror(errorMessage);
    exit(1);
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
    //printf("\nInsideDes::%s,%d,%d,%d,%s,%s,%s,%s,%s\n",rr.m,rr.c,rr.i,rr.c,rr.r,rr.operation,rr.operationType,rr.fileName,rr.openMode);

    return rr;
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
    if (str[strlen(str) - 1] == '\n')
    {
        str[strlen(str) - 1] = ':';
        str[strlen(str)] = '\0';
    }
    else
    {
        strcat(str, ":");
    }
}

void getdashedOperation(char *oper)
{
    char operation[80 + 1];
    char l[80 + 1], operationType[10], fileName[10], writeString[60], openMode[10], fOp[80 + 1], readOrLseek[5];
    int rol;
    strcpy(l, oper);

    strcpy(operationType, strtok(l, " "));
    //printf("\nentered get dashed.\n");
    if (operationType[strlen(operationType) - 1] == '\n')
    {
        operationType[strlen(operationType) - 1] = '\0';
    }
    if (strcmp(operationType, "fail") != 0)
    {
        //printf("\nentered get dashed.|%s|\n",operationType);

        strcpy(fileName, strtok(NULL, " "));
    }
    if (strcmp(operationType, "close") == 0)
    {
        strcpy(fOp, operationType);
        strcat(fOp, "-");
        strcat(fOp, fileName);
    }
    else if (strcmp(operationType, "read") == 0 || strcmp(operationType, "lseek") == 0)
    {

        strcpy(readOrLseek, strtok(NULL, " "));
        strcpy(fOp, operationType);
        strcat(fOp, "-");
        strcat(fOp, fileName);
        strcat(fOp, "-");
        strcat(fOp, readOrLseek);
        rol = std::stoi(readOrLseek);
        req.readOrLseek = rol;
    }
    else if (strcmp(operationType, "write") == 0)
    {
        char tempVar[60];
        strcpy(tempVar, strtok(NULL, " "));
        //int len = strlen(tempVar);
        //tempVar[len - 1] = '\0';
        //char *contents_chopped = tempVar + 1;
        strcpy(writeString, tempVar);
        strcpy(fOp, operationType);
        strcat(fOp, "-");
        strcat(fOp, fileName);
        strcat(fOp, "-");
        strcat(fOp, writeString);
        strcpy(req.writeString, writeString);
    }
    else if (strcmp(operationType, "open") == 0)
    {
        strcpy(openMode, strtok(NULL, " "));
        strcpy(fOp, operationType);
        strcat(fOp, "-");
        strcat(fOp, fileName);
        strcat(fOp, "-");
        strcat(fOp, openMode);

        strcpy(req.openMode, openMode);
    }
    else
    {
        strcpy(fOp, operationType);
        strcpy(operation, fOp);
        strcpy(req.operation, fOp);
        strcpy(req.operationType, operationType);
        return;
    }
    strcpy(operation, fOp);
    strcpy(req.operation, fOp);
    strcpy(req.fileName, fileName);
    strcpy(req.operationType, operationType);
}
void getReq(int r, int c, char *m, int i)
{
    req.r = r;
    req.c = c;
    req.i = i;
    strcpy(req.m, m);
}

int main(int argc, char *argv[])
{
    struct timeval timeout = {5, 0}; //set timeout for 2 seconds

    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *servIP;                    /* IP address of server */
    char *echoString;                /* String to send to echo server */
    char echoBuffer[ECHOMAX + 1];    /* Buffer for receiving echoed string */
    int echoStringLen;               /* Length of string to echo */
    int respStringLen;               /* Length of received response */
    char requestStr[300 + 1], responseStr[300 + 1];
    requestNumber = 0;

    if (argc < 3) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Server IP address> <Echo Port>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */
    echoServPort = atoi(argv[2]); /* Second arg: Use given port, if any */

    clrscr();
    printf(MAG "%20s %20s %20s\n" RESET, "", "CLIENT", "");
    liner();
    printf(BLU "Arguments passed: server IP %s, port %d\n" RESET, servIP, echoServPort);

    printf(YEL "Please Enter Machine Name:" RESET);
    std::cin >> machine;
    printf(YEL "Please Enter Client Number:" RESET);
    scanf("%d", &clientNumber);

    initializeClient(clientNumber);
    /* Create a datagram/UDP socket */

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */

    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet addr family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    echoServAddr.sin_port = htons(echoServPort);      /* Server port */

    /* Pass string back and forth between server 5 times */

    int a = 1;
    char line[300 + 1] = "";
    char operation[80 + 1];
    fseek(scriptFile, 0, SEEK_SET);

    int incarnationNum, client, reqNum;
    while (fgets(line, sizeof(line), scriptFile))
    {
        getdashedOperation(line);

        if (strcmp(req.operationType, "fail") == 0)
        {
            incarnationNumber++;
            liner();
            printf(RED "FATAL SYSTEM FAILURE!!\n" RESET);
            printf(YEL "Incarnation Incremented:%d\n" RESET, incarnationNumber);
            //liner();
            continue;
        }
        //printf("\nIncarnation Number:%d\n",incarnationNumber);
        getReq(++requestNumber, clientNumber, machine, incarnationNumber);

        serializeReq(requestStr, req);
        liner();
        printf(YEL "Request:" RESET);
        printf(RED " %s\n" RESET, requestStr);
        printf(BLU "PRESS ENTER\n" RESET);
        if (a == 1)
        {
            getchar();
            a = 0;
        }
        getchar();
        
        /* Send the struct to the server */
        if (sendto(sock, requestStr, strlen(requestStr), 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != strlen(requestStr))
            DieWithError("sendto() sent a different number of bytes than expected");

        /* Recv a response */

        fromSize = sizeof(fromAddr);
        //////////////////////////////////////////////

        int i = 0;
        respStringLen = -1;
        /* set receive UDP message timeout */

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

        /* Receive UDP message */
        while (respStringLen < 0)
        {
            
            respStringLen = recvfrom(sock, responseStr, ECHOMAX, 0, (struct sockaddr *)&fromAddr, &fromSize);
            if (respStringLen >= 0)
            {
                //Message Received

                responseStr[respStringLen] = '\0';
                if(strcmp(responseStr,"pleasewait")==0)
                {
                    timeout = {25, 0};
                    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
                    respStringLen=-1;
                    strcpy(responseStr,"");
                    continue;
                }
                char responseStrCp[301];
                strcpy(responseStrCp,responseStr);
                char * token;
                char * rest;
                rest=responseStrCp;

                token=strtok_r(rest,"|",&rest);

                if (strcmp(token, "leaderExists") == 0)
                {
                    
                    char leaderIP[20];
                    strcpy(leaderIP,strtok(responseStr,"|"));
                    strcpy(leaderIP,strtok(NULL,"]"));
                    printf(RED "\nThe leader is still alive:%s\n Switching...\n" RESET,leaderIP);
                    strcpy(servIP,leaderIP);
                    if(strcmp(leaderIP,"")!=0)
                    {
                        echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
                        if (sendto(sock, requestStr, strlen(requestStr), 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != strlen(requestStr))
                            {
                                DieWithError("sendto() sent a different number of bytes than expected");
                            }
                        respStringLen=-1;
                    strcpy(responseStr,"");
                    timeout = {5, 0};
                    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
                    }
                }
            }
            else
            {
                    timeout = {5, 0};
                    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
                //printf(BLU "Didn't get response, trying again..." RESET);
                i++;

                if(i==6)
                {
                    i=0;
                    printf("The next leader list is %s\n",nextLeaderList);
                    //Change server;
                    char TnextLeaderList[1000];
                    strcpy(TnextLeaderList,nextLeaderList);
                    if(strcmp(nextLeaderList,"")==0)
                    {
                        continue;
                    }
                    char *token = strtok(TnextLeaderList, ",");
                    if(token!=NULL)
                    {
                        char leaderIP[20];
                        strcpy(leaderIP,token);
                        strcpy(servIP,leaderIP);
                        printf("The next assumed leader %s\n",servIP);
                        if(strcmp(leaderIP,"")!=0)
                        {
                            echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
                            if (sendto(sock, requestStr, strlen(requestStr), 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != strlen(requestStr))
                                {
                                    DieWithError("sendto() sent a different number of bytes than expected");
                                }
                            timeout = {5, 0};
                            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
                            continue;
                        }
                    }

                }
                else{
                    printf(BLU "No response received, trying again...\n" RESET);
                    if (sendto(sock, requestStr, strlen(requestStr), 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != strlen(requestStr))
                    {
                         DieWithError("sendto() sent a different number of bytes than expected");
                    }   
                }
                //Message Receive Timeout or other error
            }
        }

        ///////////////////////////////////////////////

        /*
    	if ((respStringLen = recvfrom(sock, responseStr, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize)) > ECHOMAX )
        	DieWithError("recvfrom() failed");
        */

        if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
        {
            printf(RED);
            fprintf(stderr, "Error: received a packet from unknown source.\n");
            printf(RESET);
            exit(1);
        }

        char *token = strtok(responseStr, ":");
        printf(YEL "Response from server: " RESET);
        printf(GRN "%s\n" RESET, token); /* Print the echoed arg */
        if(token != NULL)
        {
            token = strtok(NULL, ";");
            if(token!=NULL)
            {
                strcpy(nextLeaderList,token);
            }
            //printf(GRN "Next Leader:|%s|\n" RESET, nextLeaderList); /* Print the echoed arg */
        }
    /*
        char realResponse[300];
        strcpy(realResponse,strtok(responseStr,":"));
        strcpy(nextLeaderList,strtok(NULL,";"));
        
        printf(YEL "Response from server: " RESET);
        printf(GRN "%s\n" RESET, realResponse); /* Print the echoed arg 
        printf(GRN "Next Leader:|%s|\n" RESET, nextLeaderList); /* Print the echoed arg */
        
    }

    close(sock);
    exit(0);
}
