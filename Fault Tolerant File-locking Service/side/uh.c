
// C program to demonstrate working of wait() 
#include<stdio.h> 
#include<stdlib.h> 
#include <thread> 
#include <iostream> 

void x()
{ 


    printf("\nhii");
  for (int i=10; i>0; --i) {

printf("hii");
if(i==5)
{
    printf("%d\n",i);
}
    //printf("hii\n");
    std::this_thread::sleep_for (std::chrono::seconds(1));
  }

    printf("\nend\n");
} 

int main() 
{ 
x();
} 