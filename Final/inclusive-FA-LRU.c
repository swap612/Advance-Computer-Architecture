/*********** Author: Mainak Chaudhuri **************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
   unsigned long long tag;
   unsigned long long lru;
} cacheTag;

#define INVALID_TAG 0xfffffffffffffffULL

#define L2_NUMSET 1024
#define L2_ASSOC 8
#define L2_BLOCK_SIZE 64
#define LOG_L2_BLOCK_SIZE 6

#define L3_NUMSET 1
#define L3_ASSOC (1 << 15)
#define L3_BLOCK_SIZE 64
#define LOG_L3_BLOCK_SIZE 6

int main (int argc, char **argv)
{
   int i, j, k, m, L3setid, L2setid, INVsetid, maxindex, numtraces;
   unsigned long long addr, miss=0, max, l2_miss=0;
   char dest_file_name[256], input_name[256];
   char type, iord;
   FILE *fp;
   cacheTag **cache2, **cache3;
   unsigned pc;
   int l2way, l3way, updateway;

   if (argc != 3) {
      printf("Need two arguments: input file prefix, number of traces. Aborting...\n");
      exit (1);
   }

   numtraces = atoi(argv[2]);

   cache2 = (cacheTag**)malloc(L2_NUMSET*sizeof(cacheTag*));
   assert(cache2 != NULL);
   for (i=0; i<L2_NUMSET; i++) {
      cache2[i] = (cacheTag*)malloc(L2_ASSOC*sizeof(cacheTag));
      assert(cache2[i] != NULL);
      for (j=0; j<L2_ASSOC; j++) {
         cache2[i][j].tag = INVALID_TAG;
      }
   }

   cache3 = (cacheTag**)malloc(L3_NUMSET*sizeof(cacheTag*));
   assert(cache3 != NULL);
   for (i=0; i<L3_NUMSET; i++) {
      cache3[i] = (cacheTag*)malloc(L3_ASSOC*sizeof(cacheTag));
      assert(cache3[i] != NULL);
      for (j=0; j<L3_ASSOC; j++) {
         cache3[i][j].tag = INVALID_TAG;
      }
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
         L2setid = (addr >> LOG_L2_BLOCK_SIZE) & (L2_NUMSET - 1);
         L3setid = (addr >> LOG_L3_BLOCK_SIZE) & (L3_NUMSET - 1);

         if (type != 0) {
         // L2 cache lookup
         for (l2way=0; l2way<L2_ASSOC; l2way++) {
            if (cache2[L2setid][l2way].tag == (addr >> LOG_L2_BLOCK_SIZE)) {
               break;
            }
         }
         if (l2way==L2_ASSOC) {  // L2 cache miss
            l2_miss++;
            // L3 lookup
            for (l3way=0; l3way<L3_ASSOC; l3way++) {
               if (cache3[L3setid][l3way].tag == (addr >> LOG_L3_BLOCK_SIZE)) {
                  break;
               }
            }
            if (l3way==L3_ASSOC) {
               // Replace
               miss++;
               for (l3way=0; l3way<L3_ASSOC; l3way++) {
                  if (cache3[L3setid][l3way].tag == INVALID_TAG) break;
               }
               if (l3way==L3_ASSOC) {
                  // Find LRU
                  max = 0;
                  for (l3way=0; l3way<L3_ASSOC; l3way++) {
                     if (cache3[L3setid][l3way].lru >= max) {
                        max = cache3[L3setid][l3way].lru;
                        maxindex = l3way;
                     }
                  }
                  l3way = maxindex;
               }
               assert (l3way < L3_ASSOC);
               // Invalidate L3 tag in L2 cache
               if (cache3[L3setid][l3way].tag != INVALID_TAG) {
                  INVsetid = ((cache3[L3setid][l3way].tag << LOG_L3_BLOCK_SIZE) >> LOG_L2_BLOCK_SIZE) & (L2_NUMSET - 1);
                  for (updateway=0; updateway<L2_ASSOC; updateway++) {
                     if (cache2[INVsetid][updateway].tag == ((cache3[L3setid][l3way].tag << LOG_L3_BLOCK_SIZE) >> LOG_L2_BLOCK_SIZE)) {
                        cache2[INVsetid][updateway].tag = INVALID_TAG;
                        break;
                     }
                  }
               }
               cache3[L3setid][l3way].tag = (addr >> LOG_L3_BLOCK_SIZE);
            }
            for (j=0; j<L3_ASSOC; j++) {
               cache3[L3setid][j].lru++;
            }
            cache3[L3setid][l3way].lru = 0;
            // Now fill in L2 cache
            for (l2way=0; l2way<L2_ASSOC; l2way++) {
               if (cache2[L2setid][l2way].tag == INVALID_TAG) break;
            }
            if (l2way==L2_ASSOC) {
               // Find LRU
               max = 0;
               for (l2way=0; l2way<L2_ASSOC; l2way++) {
                  if (cache2[L2setid][l2way].lru >= max) {
                     max = cache2[L2setid][l2way].lru;
                     maxindex = l2way;
                  }
               }
               l2way = maxindex;
            }
            assert(l2way < L2_ASSOC);
            cache2[L2setid][l2way].tag = (addr >> LOG_L2_BLOCK_SIZE);
         }
         assert(l2way < L2_ASSOC);
         for (j=0; j<L2_ASSOC; j++) {
            cache2[L2setid][j].lru++;
         }
         cache2[L2setid][l2way].lru = 0;
         }
      }
      fclose(fp);
      printf("Done reading file %d!\n", k);
   }
   sprintf(dest_file_name, "%s.inclusion-FA-LRU.L2%d", argv[1], L2_NUMSET/2);
   fp = fopen(dest_file_name, "w");
   assert(fp != NULL);
   fprintf(fp, "L3: %llu, L2: %llu\n", miss, l2_miss);
   fclose(fp);
   return 0;
}
