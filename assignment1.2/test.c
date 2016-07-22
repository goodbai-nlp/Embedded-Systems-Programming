#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <sys/time.h>
#include <linux/ioctl.h>
#include <linux/rtc.h>*/
#define random(x) (rand()%x)
#include <time.h>
typedef struct msg{
    int seqnum;
    int source,destination;
    char str[85];
}MSG;
void enumstr(char *s,int len){
    for(int i=0;i<len;++i){
        int flag = rand() % 3;
        switch (flag){
            case 0:
                s[i] = 'A' + rand() % 26;
                break;
            case 1:
                s[i] = 'a' + rand() % 26;
                break;
            case 2:
                s[i] = '0' + rand() % 10;
                break;
            default:
                s[i] = 'x';
                break;
        }
    }
    s[len - 1] = '\0';
}
int main(int argc, char **argv)
{
	int fd, res;
	MSG buff[10];
	int i = 0;
	srand( (unsigned)time(NULL));
	if(argc < 2){
		printf("\nGmem_test: A helper program for the driver gmem\nPossible usage is gmem_test [option] <string>\n\n");
		printf("Option\t\t\tDescription\n-----\t\t\t-----------\n");
		printf(
			"-read/show\t\tShow the content of the entire content (256 bytes)\n");
		printf("-write <string>\t\twrite a string to the driver \n\n");

		return -1;
	}

	/* open devices */
	fd = open("/dev/bus_out1", O_RDWR);
	if (fd < 0 ){
		printf("Can not open device file.\n");		
		return 0;
	}else{
		if(strcmp("show", argv[1]) == 0){
			// int len = 10+random(70);
			// MSG mm = {1,1,random(3)+1,""};
	  //       //char * str = (char *)malloc(sizeof(char)*len);
	  //       enumstr(mm.str,len);
	  //       //MSG *mm = (MSG *)malloc(sizeof(MSG));
			// if(write(fd, &mm, 1)){
			// 	printf("write succeed!\n");		
			// }
			MSG *m = (MSG *)malloc(sizeof(MSG));
			if(read(fd,m,1)){
				printf("seqnum %d sender: %d destination %d\n",(*m).seqnum,(*m).source,(*m).destination);
				printf("%s\n",(*m).str);
				return 0;
			}
			printf("show failed\n");
		}else if(strcmp("write", argv[1]) == 0){
			int len = 10+random(70);
			MSG mm;
			MSG *m = (MSG *)malloc(sizeof(MSG));
			(*m).seqnum = 1;
			(*m).source = 1;
			(*m).destination =1;
	        //char * str = (char *)malloc(sizeof(char)*len);
	        enumstr((*m).str,len);
			if(write(fd,m,1)){
				printf("write succeed!\n");		
				return 0;
			}
			printf("full\n");
		}
		/* close devices */
		close(fd);
	}
	return 0;
}
