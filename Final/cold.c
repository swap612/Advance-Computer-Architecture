/*********** Author: Mainak Chaudhuri **************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct hashTableEntry_s {
   unsigned long long addr;
   struct hashTableEntry_s *next;
} hashTableEntry;

#define INVALID_TAG 0xfffffffffffffffULL
#define SIZE 1048576
#define LOG_LLC_BLOCK_SIZE 6

int main (int argc, char **argv)
{
   hashTableEntry *ht, *prev, *ptr;
   int j, k, ht_index, numtraces;
   unsigned long long addr, miss=0;
   char dest_file_name[256], input_name[256];
   char type, iord;
   FILE *fp;
   unsigned pc;

   if (argc != 3) {
      printf("Need two arguments: input file prefix, number of traces. Aborting...\n");
      exit (1);
   }

   numtraces = atoi(argv[2]);

   ht = (hashTableEntry*)malloc(SIZE*sizeof(hashTableEntry));
   assert(ht != NULL);
   for (j=0; j<SIZE; j++) {
      ht[j].addr = INVALID_TAG;
   }

   for (k=0; k<numtraces; k++) {
      sprintf(input_name, "%s_%d", argv[1], k);
      fp = fopen(input_name, "rb");
      assert(fp != NULL);

      while (!feof(fp)) {
         fread(&iord, sizeof(char), 1, fp);
         fread(&type, sizeof(char), 1, fp);
         fread(&addr, sizeof(unsigned long long), 1, fp);
         fread(&pc, sizeof(unsigned), 1, fp);
         if (type != 0) {
            ht_index = (addr >> LOG_LLC_BLOCK_SIZE) & (SIZE - 1);
            if (ht[ht_index].addr == INVALID_TAG) {
               miss++;
               ht[ht_index].addr = addr >> LOG_LLC_BLOCK_SIZE;
               ht[ht_index].next = NULL;
            }
            else {
               prev = NULL;
               ptr = &ht[ht_index];
               while (ptr != NULL) {
                  if (ptr->addr == (addr >> LOG_LLC_BLOCK_SIZE)) {
                     break;
                  }
                  prev = ptr;
                  ptr = ptr->next;
               }
               if (ptr == NULL) {
                  miss++;
                  assert(prev->next == NULL);
                  ptr = (hashTableEntry*)malloc(sizeof(hashTableEntry));
                  assert(ptr != NULL);
                  ptr->addr = addr >> LOG_LLC_BLOCK_SIZE;
                  ptr->next = NULL;
                  prev->next = ptr;
               }
            }
         }
      }
      fclose(fp);
      printf("Done reading file %d!\n", k);
   }
   sprintf(dest_file_name, "%s.COLD", argv[1]);
   fp = fopen(dest_file_name, "w");
   assert(fp != NULL);
   fprintf(fp, "COLD: %llu\n", miss);
   fclose(fp);
   return 0;
}
