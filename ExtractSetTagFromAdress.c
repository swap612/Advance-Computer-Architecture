#include<stdio.h>
#define L2_SETS 1024
#define L3_SETS 2048
#define L2_WAYS 8
#define L3_WAYS 16
#define L2_ID 0
#define L3_ID 1
unsigned long long L2[L2_SETS][L2_WAYS];
unsigned long long L3[L3_SETS][L3_WAYS];

void extract_TagSet(int cacheID, unsigned long long addr, unsigned long long * tag,unsigned long long * set){
	if(cacheID == 0){
		*set = (addr & 0xFFC0)>>6;
		*tag = (addr & 0xFFFFFFFFFFFF0000)>>16;
	
	}
	else if(cacheID == 1){
		*set = (addr & 0x1FFC0)>>6;
		*tag = (addr & 0xFFFFFFFFFFFE0000)>>17;
	
	}
	printf("Extracated TagSet");
}

int main(){
	unsigned long long addr,tag,set;
	addr = 0xFFFFFFFFF00; //1231312331;	//1001001011001000101010111001011
	//1. check L2 cache- l2
	extract_TagSet(L3_ID, addr, &tag, &set);
	printf("%llx -- %llx",tag,set);	

}
