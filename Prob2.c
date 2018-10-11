#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

//Defined the constants
#define MAXX 16000000
#define HASH_SIZE 201392130
#define L3_SIZE 32768
#define L2_SETS 1024
#define L2_WAYS 8
#define L2_ID 0
#define L3_ID 1

//Defined Variables used
char Hash_hit[HASH_SIZE];
signed long long L3[L3_SIZE];
unsigned long long L2[L2_SETS][L2_WAYS];
unsigned long long Time_Stamp;
unsigned long long int Time_Stamp_L2[L2_SETS][L2_WAYS];
char ColdHash[201392130];
unsigned long long Hit_l2, Miss_l2, Hit_l3, Miss_l3;

//Struct to store the total occurences
struct node
{
  unsigned long long index;
  struct node *next;
};
struct node *Hash[HASH_SIZE];

//Search the next occurence of the addr in tracefile
int HashSearch(unsigned long long addr, unsigned long long j)
{
  unsigned long long prev = 0;
  struct node *temp = Hash[addr];
  if (temp)
  {
    prev = temp->index;
    if (j >= prev)
    {
      return MAXX;
    }

    while ((temp != NULL) && (temp->index) > j)
    {

      prev = temp->index;
      if (temp->next != NULL)
        temp = temp->next;
    }
    return prev;
  }
  return MAXX;
}

//Search element from L3 which will be farther
int search(unsigned long long addr, unsigned long long j)
{
  unsigned long long max = 0, max_index1 = 0;
  unsigned long long index1, index2;
  for (int i = 0; i < L3_SIZE; i++)
  {
    if (L3[i] == -1)
      return i;
    index2 = HashSearch(L3[i], j);
    if (max_index1 < index2)
    {
      max = i;
      max_index1 = index2;
    }
  }
  return max;
}

//initialize L3 cache with -1
void init_L3()
{
  for (int i = 0; i < L3_SIZE; i++)
  {
    L3[i] = -1;
  }
}

//return WAY from SET where data can be inserted
int LRU_QUERY_L2_IN(int SET)
{
  int i, min = Time_Stamp_L2[SET][0], WAY = 0;
  for (i = 1; i < L2_WAYS; i++)
  {
    if (Time_Stamp_L2[SET][i] <= min)
    {
      min = Time_Stamp_L2[SET][i];
      WAY = i;
    }
  }
  return WAY;
}

//Update L2 LRU Timestamp
void LRU_UPDATE_L2_IN(int SET, int WAY, int opr)
{
  Time_Stamp++;
  if (opr == 0)
    Time_Stamp_L2[SET][WAY] = 0;
  else
    Time_Stamp_L2[SET][WAY] = Time_Stamp;
}

//Search the TAG in given SET of L2 cache and return pos
int L2_SEARCH_IN(int SET, int TAG)
{
  int i;
  for (i = 1; i < L2_WAYS; i++)
  {
    if (L2[SET][i] == TAG && Time_Stamp_L2[SET][i])
      return i;
  }
  return -1;
}

//Insert TAG in L2 cache at given set
void INSERT_L2_IN(int SET, int TAG)
{
  int WAY = LRU_QUERY_L2_IN(SET);
  L2[SET][WAY] = TAG;
  LRU_UPDATE_L2_IN(SET, WAY, 1);
}

/*Fuction to extract tag and set from given addr */
void extract_TagSet(int cacheID, unsigned long long addr, unsigned long long *tag, unsigned long long *set)
{
  if (cacheID == 0)
  {
    *set = (addr & 0xFFC0) >> 6;
    *tag = (addr & 0xFFFFFFFFFFFF0000) >> 16;
  }
  else if (cacheID == 1)
  {
    *set = (addr & 0x1FFC0) >> 6;
    *tag = (addr & 0xFFFFFFFFFFFE0000) >> 17;
  }
}

//Main Function
int main(int argc, char *argv[])
{
  FILE *fp;
  char iord, type;
  unsigned long long addr, TAG, SET, New_addr, addr_miss = 0, count = 0, i = 0;
  struct node *new;
  unsigned pc;
  char input_name[100];
  int numtraces = atoi(argv[2]), WAY, R_WAY;
  init_L3();

  // PASS1: Generating Hash
  for (int k = 0; k < numtraces; k++)
  {
    sprintf(input_name, "%s_%d", argv[1], k);
    fp = fopen(input_name, "rb");

    if (fp == NULL)
    {
      printf("Error!");
      exit(1);
    }

    while (!feof(fp))
    {
      fread(&iord, sizeof(char), 1, fp);
      fread(&type, sizeof(char), 1, fp);
      fread(&addr, sizeof(unsigned long long), 1, fp);
      fread(&pc, sizeof(unsigned), 1, fp);
      //removing block offset (6 bits)
      addr >>= 6;
      if (type != 0)
      {
        new = (struct node *)malloc(sizeof(struct node));
        new->index = i;
        new->next = NULL;

        if (Hash[addr] == NULL)
        {
          Hash[addr] = new;
        }
        else
        {
          new->next = Hash[addr];
          Hash[addr] = new;
        }
        i++;
      }
    }
    fclose(fp);
  }
  {

    // PASS2: Trace processing
    i = 0;
    for (int k = 0; k < numtraces; k++)
    {
      sprintf(input_name, "%s_%d", argv[1], k);
      fp = fopen(input_name, "rb");

      if (fp == NULL)
      {
        printf("Error!");
        exit(1);
      }

      while (!feof(fp))
      {
        fread(&iord, sizeof(char), 1, fp);
        fread(&type, sizeof(char), 1, fp);
        fread(&addr, sizeof(unsigned long long), 1, fp);
        fread(&pc, sizeof(unsigned), 1, fp);
        if (type != 0)
        { //extract bits according to L2
          extract_TagSet(L2_ID, addr, &TAG, &SET);

          //search in l2
          WAY = L2_SEARCH_IN(SET, TAG);
          if (WAY != -1)
          {
            Hit_l2++;
            //update lru L2
            LRU_UPDATE_L2_IN(SET, WAY, 1);
          }
          else
          {
            Miss_l2++;
            //extract bits according to L3
            addr_miss = addr >> 6;
            //search in L3
            if (Hash_hit[addr_miss])
            {
              Hit_l3++;
              //find L2 address and extract tag,set
              extract_TagSet(L2_ID, addr, &TAG, &SET);
              //Query L2 LRU for way no.fill L2 cache Update LRU L2
              INSERT_L2_IN(SET, TAG);
            }
            else
            {
              Miss_l3++;
              //Calculating Cold Misses
              addr_miss = addr >> 6;

              if (ColdHash[addr_miss] == 0)
              {
                count++;
                ColdHash[addr_miss] = 1;
              }
              //query LRU L3 to get way no.
              WAY = search(addr_miss, i);

              if (L3[WAY] == -1)
              {
                //put data in L3 and update L2
                L3[WAY] = addr_miss;
                Hash_hit[addr_miss] = 1;
                extract_TagSet(L2_ID, addr, &TAG, &SET);
                INSERT_L2_IN(SET, TAG);
              }
              else
              {
                //Replace block in L3
                TAG = L3[WAY];
                Hash_hit[TAG] = 0;
                New_addr = TAG << 6;
                L3[WAY] = addr_miss;
                Hash_hit[addr_miss] = 1;
                extract_TagSet(L2_ID, New_addr, &TAG, &SET);
                //search for the tag and get way no
                WAY = L2_SEARCH_IN(SET, TAG);
                if (WAY != -1)
                {
                  //invalidate that way---set timestamp 0
                  LRU_UPDATE_L2_IN(SET, WAY, 0);
                }

                //extract tag acc. to L2 and insert in L2
                extract_TagSet(L2_ID, addr, &TAG, &SET);
                INSERT_L2_IN(SET, TAG);
              }
            }
          }

          i++;
        }
      }

      fclose(fp);
    }
  }
  printf("\nHit L2=%lld\nMiss L2=%lld\nHit L3=%lld\nMiss L3=%lld\ncold misses=%lld", Hit_l2, Miss_l2, Hit_l3, Miss_l3, count);
  return 0;
}
