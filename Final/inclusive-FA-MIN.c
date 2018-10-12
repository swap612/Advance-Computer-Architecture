/*********** Author: Mainak Chaudhuri **************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// A list of integers
// Used to maintain the list of access timestamps for a particular block
typedef struct integerList_s {
   unsigned long long id;
   struct integerList_s *next;
} integerListEntry;

// This hash table is used by the optimal policy for maintaining the next access stamp
// Each hash table entry has a block address and a list of access timestamps to that block
typedef struct hashTableEntry_s {
   unsigned long long addr;     // Block address
   integerListEntry *ilhead;    // Head of the access timestamp list
   integerListEntry *tail;      // Tail of the access timestamp list
   integerListEntry *currentPtr;// Pointer to the current position in the access list during simulation 
   struct hashTableEntry_s *next;
} hashTableEntry;

typedef struct {
   unsigned long long tag;
   unsigned long long lru;
   hashTableEntry *htPtr;       // A pointer to the corresponding hash table entry
                                // Each block address gets a unique hash table entry
} cacheTag;

#define SIZE 4194304
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
   unsigned long long addr, miss=0, max, l2_miss=0, *uniqueId;
   char dest_file_name[256], input_name[256];
   char type, iord;
   FILE *fp;
   cacheTag **cache2, **cache3;
   unsigned pc;
   int l2way, l3way, updateway;
   hashTableEntry *ht, *prev, *ptr;
   int hash_index;

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

   // Allocate hash table
   ht = (hashTableEntry*)malloc(SIZE*sizeof(hashTableEntry));
   assert(ht != NULL);
   for (j=0; j<SIZE; j++) {
      ht[j].ilhead = NULL;
      ht[j].tail = NULL;
   }

   // The following array is used to find the sequence number for an access to a set
   // This sequence number acts as the timestamp for the access
   uniqueId = (unsigned long long*)malloc(L3_NUMSET*sizeof(unsigned long long));
   assert(uniqueId != NULL);
   for (i=0; i<L3_NUMSET; i++) {
      uniqueId[i] = 0;
   }

   // Build the hash table of accesses
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
            hash_index = (addr >> LOG_L3_BLOCK_SIZE) % SIZE;
            L3setid = (addr >> LOG_L3_BLOCK_SIZE) & (L3_NUMSET - 1);
            if (ht[hash_index].ilhead == NULL) {
               ht[hash_index].addr = addr >> LOG_L3_BLOCK_SIZE;
               ht[hash_index].ilhead = (integerListEntry*)malloc(sizeof(integerListEntry));
               assert(ht[hash_index].ilhead != NULL);
               ht[hash_index].tail = ht[hash_index].ilhead;
               ht[hash_index].ilhead->id = uniqueId[L3setid];
               ht[hash_index].ilhead->next = NULL;
               ht[hash_index].currentPtr = ht[hash_index].ilhead;  // Initialize to point to the beginning of the list
               ht[hash_index].next = NULL;
            }
            else {
               prev = NULL;
               ptr = &ht[hash_index];
               while (ptr != NULL) {
                  if (ptr->addr == (addr >> LOG_L3_BLOCK_SIZE)) {
                     assert(ptr->ilhead != NULL);
                     assert(ptr->tail->next == NULL);
                     ptr->tail->next = (integerListEntry*)malloc(sizeof(integerListEntry));
                     assert(ptr->tail->next != NULL);
                     ptr->tail = ptr->tail->next;
                     ptr->tail->id = uniqueId[L3setid];
                     ptr->tail->next = NULL;
                     break;
                  }
                  prev = ptr;
                  ptr = ptr->next;
               }
               if (ptr == NULL) {
                  assert(prev->next == NULL);
                  ptr = (hashTableEntry*)malloc(sizeof(hashTableEntry));
                  assert(ptr != NULL);
                  ptr->addr = addr >> LOG_L3_BLOCK_SIZE;
                  ptr->ilhead = (integerListEntry*)malloc(sizeof(integerListEntry));
                  assert(ptr->ilhead != NULL);
                  ptr->tail = ptr->ilhead;
                  ptr->tail->id = uniqueId[L3setid];
                  ptr->tail->next = NULL;
                  ptr->next = NULL;
                  ptr->currentPtr = ptr->ilhead;
                  prev->next = ptr;
               }
            }
            uniqueId[L3setid]++;
         }
      }
      fclose(fp);
      printf("Done reading file %d!\n", k);
   }
   printf("Access list prepared.\nStarting simulation...\n"); fflush(stdout);

   // Simulate
   for (k=0; k<numtraces; k++) {
      sprintf(input_name, "%s_%d", argv[1], k);
      fp = fopen(input_name, "rb");
      assert(fp != NULL);

      while (!feof(fp)) {
         fread(&iord, sizeof(char), 1, fp);
         fread(&type, sizeof(char), 1, fp);
         fread(&addr, sizeof(unsigned long long), 1, fp);
         fread(&pc, sizeof(unsigned), 1, fp);
         hash_index = (addr >> LOG_L3_BLOCK_SIZE) % SIZE;
         L2setid = (addr >> LOG_L2_BLOCK_SIZE) & (L2_NUMSET - 1);
         L3setid = (addr >> LOG_L3_BLOCK_SIZE) & (L3_NUMSET - 1);

         if (type != 0) {
         // L2 cache lookup
         for (l2way=0; l2way<L2_ASSOC; l2way++) {
            if (cache2[L2setid][l2way].tag == (addr >> LOG_L2_BLOCK_SIZE)) {
	       // Update access list
               for (j=0; j<L3_ASSOC; j++) {
                  if (cache3[L3setid][j].tag == (addr >> LOG_L3_BLOCK_SIZE)) {
                     assert(cache3[L3setid][j].htPtr != NULL);
                     assert(cache3[L3setid][j].htPtr->addr == cache3[L3setid][j].tag);
                     assert(cache3[L3setid][j].htPtr->currentPtr != NULL);
                     cache3[L3setid][j].htPtr->currentPtr = cache3[L3setid][j].htPtr->currentPtr->next;
                     break;
                  }
               }
               assert(j < L3_ASSOC);  // Inclusion
               break;
            }
         }
         if (l2way==L2_ASSOC) {  // L2 cache miss
            l2_miss++;
            // L3 lookup
            for (l3way=0; l3way<L3_ASSOC; l3way++) {
               if (cache3[L3setid][l3way].tag == (addr >> LOG_L3_BLOCK_SIZE)) {
                  // Update access list
                  assert(cache3[L3setid][l3way].htPtr != NULL);
                  assert(cache3[L3setid][l3way].htPtr->addr == cache3[L3setid][l3way].tag);
                  assert(cache3[L3setid][l3way].htPtr->currentPtr != NULL);
                  cache3[L3setid][l3way].htPtr->currentPtr = cache3[L3setid][l3way].htPtr->currentPtr->next;
                  break;
               }
            }
            if (l3way==L3_ASSOC) {
               // Access list pointer needs to be advanced
               // Search the entry in hash table
               ptr = &ht[hash_index];
               while (ptr != NULL) {
                  if (ptr->addr == (addr >> LOG_L3_BLOCK_SIZE)) break;
                  ptr = ptr->next;
               }
               assert(ptr != NULL);
               assert(ptr->currentPtr != NULL);
               ptr->currentPtr = ptr->currentPtr->next; // Advance to point to the next access

               // Replace
               miss++;
               for (l3way=0; l3way<L3_ASSOC; l3way++) {
                  if (cache3[L3setid][l3way].tag == INVALID_TAG) break;
               }
               if (l3way==L3_ASSOC) {
                  // Find MIN
                  max = 0;
                  for (l3way=0; l3way<L3_ASSOC; l3way++) {
                     if ((cache3[L3setid][l3way].htPtr == NULL) || (cache3[L3setid][l3way].htPtr->currentPtr == NULL)) {     // No future access (eternally dead)
                        maxindex = l3way;
                        break;
                     }
                     if (cache3[L3setid][l3way].htPtr->currentPtr->id >= max) {
                        max = cache3[L3setid][l3way].htPtr->currentPtr->id;
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
               cache3[L3setid][l3way].htPtr = ptr;    // Set up the hash table pointer
            }
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

   // Sanity check terminal state
   // All access lists must have been exhausted
   for (i=0; i<SIZE; i++) {
      if (ht[i].ilhead != NULL) {
         ptr = &ht[i];
         while (ptr != NULL) {
            assert(ptr->currentPtr == NULL);
            ptr = ptr->next;
         }
      }
   }


   sprintf(dest_file_name, "%s.inclusion-FA-MIN.L2%d", argv[1], L2_NUMSET/2);
   fp = fopen(dest_file_name, "w");
   assert(fp != NULL);
   fprintf(fp, "L3: %llu, L2: %llu\n", miss, l2_miss);
   fclose(fp);
   return 0;
}
