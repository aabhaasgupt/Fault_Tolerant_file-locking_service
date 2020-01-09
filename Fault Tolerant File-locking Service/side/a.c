#include <stdio.h>
#include <string.h>



char *getNextAssumedLeader( char *nextLeaderListRef)
    {
        printf("got inside");
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


int main()
{
char str[] = "a,b,c,d,e,f,"; 
char *res;
int d=0;
 //   res=getNextAssumedLeader(str);
    printf("nextList initially is |%s|\n",str);
while(strcmp(str,"")!=0 && d<10)
{
    res=getNextAssumedLeader(str);
    printf("assumedLeader is |%s|\n",res);
    printf("nextList now is |%s|\n",str);
    d++;
}
/*
strcpy(str,"a,b,n,f,n");
    char* token; 
    char* rest = str; 
  
    while ((token = strtok_r(rest, ",", &rest))) 
    {
        if (strcmp(token, "n") == 0)
        {
            printf("%s\n", token);
        }
    }
*/
    printf("ended\n");

    return 0;
}