#include "AnalyseServer.h"

int main(int argc, char * argv[])
{
    CAnalyseServer AnalyseServer;
    
    if(!AnalyseServer.StartAnalyseServer())
    {
        printf("****Error: AnalyseService Start Failed!\n");
        getchar();
    }


    return 0;
}