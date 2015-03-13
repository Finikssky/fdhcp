#include "queue.h"

void deletehead(int qnum){
int i;
	
	for(i=0;i<qc[qnum]-1;i++)
			 qm[qnum][i]=qm[qnum][i+1];
	
	if(qc[qnum]>0)
		qm[qnum]=realloc(qm[qnum],(--qc[qnum])*sizeof(struct qmessage));
}
