#include "queue.h"
#include <string.h>

struct qmessage reversemessage(struct qmessage in){
int i,j;
int len;
char *temp;
struct qmessage ret;

ret=in;
len=strlen(ret.text);
temp=malloc(len*sizeof(char));
	
	for(i=len-1,j=0;i>=0;i--,j++) 
		temp[j]=ret.text[i];
	
	temp[j]='\0';
	strcpy(ret.text,temp);

return ret;
}
