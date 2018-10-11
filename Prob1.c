#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define L2_SETS 1024
#define L3_SETS 2048
#define L2_WAYS 8
#define L3_WAYS 16
#define L2_ID 0
#define L3_ID 1

//Defined the required Data structure/Variables
unsigned long long L2[L2_SETS][L2_WAYS];
unsigned long long L3[L3_SETS][L3_WAYS];
unsigned long long int Time_Stamp_L2[L2_SETS][L2_WAYS];
unsigned long long Time_Stamp_L3[L3_SETS][L3_WAYS];
unsigned long long Time_Stamp;
unsigned long long Hit_l2, Miss_l2, Hit_l3, Miss_l3;
char ColdHash[201392130];

//initialize/reset the variables used
void flush()
{
  Hit_l2 = 0, Miss_l2 = 0, Hit_l3 = 0, Miss_l3 = 0;
  //clearing L2 cache
  for (int i = 0; i < L2_SETS; i++)
  {
    for (int j = 0; j < L2_WAYS; j++)
    {
      L2[i][j] = 0;
    }
  }

  //clearing L2 Timestamp
  for (int i = 0; i < L2_SETS; i++)
  {
    for (int j = 0; j < L2_WAYS; j++)
    {
      Time_Stamp_L2[i][j] = 0;
    }
  }

  //clearing L3 cache
  for (int i = 0; i < L3_SETS; i++)
  {
    for (int j = 0; j < L3_WAYS; j++)
    {
      L3[i][j] = 0;
    }
  }

  //clearing L3 Timestamp
  for (int i = 0; i < L3_SETS; i++)
  {
    for (int j = 0; j < L3_WAYS; j++)
    {
      Time_Stamp_L3[i][j] = 0;
    }
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

//return WAY from SET where data can be inserted
int LRU_QUERY_L3_IN(int SET)
{
  int i, min = Time_Stamp_L3[SET][0], WAY = 0;
  for (i = 1; i < L3_WAYS; i++)
  {
    if (Time_Stamp_L3[SET][i] <= min)
    {
      min = Time_Stamp_L3[SET][i];
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

//Update L3 LRU Timestamp
void LRU_UPDATE_L3_IN(int SET, int WAY, int opr)
{
  Time_Stamp++;
  //opr 0 mean invalidate
  if (opr == 0)
    Time_Stamp_L3[SET][WAY] = 0;
  else
    //update timestamp
    Time_Stamp_L3[SET][WAY] = Time_Stamp;
}

//Search the TAG in given SET of L2 cache and return pos
int L2_SEARCH_IN(int SET, int TAG)
{
  for (int i = 0; i < L2_WAYS; i++)
  {
    if (L2[SET][i] == TAG && Time_Stamp_L2[SET][i])
      return i;
  }
  return -1;
}

//Search the TAG in given SET of L3 and return pos
int L3_SEARCH_IN(int SET, int TAG)
{
  for (int i = 0; i < L3_WAYS; i++)
  {
    if (L3[SET][i] == TAG && Time_Stamp_L3[SET][i])
      return i;
  }
  return -1;
}

//Insert TAG in L2 cache at given set
void INSERT_L2_IN(int SET, int TAG)
{
  //WAY return the position to be evict, then insert data at WAY and update LRU table
  int WAY = LRU_QUERY_L2_IN(SET);
  L2[SET][WAY] = TAG;
  LRU_UPDATE_L2_IN(SET, WAY, 1);
}

//Insert TAG in L3 cache at given set
void INSERT_L3_IN(int SET, int TAG)
{
  //WAY return the position to be evict, then insert data at WAY and update LRU table
  int WAY = LRU_QUERY_L3_IN(SET);
  L3[SET][WAY] = TAG;
  LRU_UPDATE_L3_IN(SET, WAY, 1);
}

/*Fuction to extract tag and set from given addr */
void extract_TagSet(int cacheID, unsigned long long addr, unsigned long long *tag, unsigned long long *set)
{
  //cacheID is L2
  if (cacheID == 0)
  {
    *set = (addr & 0xFFC0) >> 6;
    *tag = (addr & 0xFFFFFFFFFFFF0000) >> 16;
  }
  //cacheID is L3
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
  unsigned long long addr, cold_addr, TAG, SET, New_addr, cold_count = 0;
  unsigned pc;
  char input_name[100];
  int numtraces = atoi(argv[2]), WAY, R_WAY;

  //Inclusive Cache Policy
  {
    for (int k = 0; k < numtraces; k++)
    {
      sprintf(input_name, "%s_%d", argv[1], k);
      fp = fopen(input_name, "rb");
      //Checking error during file read
      if (fp == NULL)
      {
        printf("Error!");
        exit(1);
      }
      //Reading the file
      while (!feof(fp))
      {
        fread(&iord, sizeof(char), 1, fp);
        fread(&type, sizeof(char), 1, fp);
        fread(&addr, sizeof(unsigned long long), 1, fp);
        fread(&pc, sizeof(unsigned), 1, fp);
        //checking type as L1 miss
        if (type != 0)
        {
          //extract bits according to L2
          extract_TagSet(L2_ID, addr, &TAG, &SET);
          //search in l2
          WAY = L2_SEARCH_IN(SET, TAG);
          //WAY return -1 if not present in L2
          if (WAY != -1)
          {
            //L2 Hit
            Hit_l2++;
            //update lru L2 timestamp
            LRU_UPDATE_L2_IN(SET, WAY, 1);
          }
          else
          {
            //L2 Miss
            Miss_l2++;
            //extract bits according to L3
            extract_TagSet(L3_ID, addr, &TAG, &SET);
            //search in L3
            WAY = L3_SEARCH_IN(SET, TAG);
            //WAY return -1 if not present in L2
            if (WAY != -1)
            {
              //L3 Hit
              Hit_l3++;
              //update LRU L3
              LRU_UPDATE_L3_IN(SET, WAY, 1);
              //find L2 address and extract tag,set
              extract_TagSet(L2_ID, addr, &TAG, &SET);
              //Query L2 LRU for way no., fill L2 cache and Update LRU L2
              INSERT_L2_IN(SET, TAG);
            }
            else
            {
              //L3 Miss
              Miss_l3++;

              //cold Miss calculation
              cold_addr = addr >> 6;
              if (ColdHash[cold_addr] == 0)
              {
                cold_count++;
                ColdHash[cold_addr] = 1;
              }

              //query LRU L3 to get way no.
              WAY = LRU_QUERY_L3_IN(SET);
              //if valid data is not present in L3 at way
              if (Time_Stamp_L3[SET][WAY] == 0)
              {
                //put data in L3 and update L3
                L3[SET][WAY] = TAG;
                LRU_UPDATE_L3_IN(SET, WAY, 1);
                //Extract Tag and set for L2 and insert in L2
                extract_TagSet(L2_ID, addr, &TAG, &SET);
                INSERT_L2_IN(SET, TAG);
              }
              else
              //if conflict happens in L3 at WAY
              {
                //Get the tag of conflicting block and invalidate evicted line from L2
                TAG = L3[SET][WAY];
                New_addr = TAG << 11;
                New_addr = New_addr | SET;
                New_addr <<= 6;
                extract_TagSet(L2_ID, New_addr, &TAG, &SET);
                //get WAY
                WAY = L2_SEARCH_IN(SET, TAG);
                if (WAY != -1)
                {
                  //invalidate WAY in L2---set timestamp 0
                  LRU_UPDATE_L2_IN(SET, WAY, 0);
                }

                //extract tag acc. to L3 and insert in L3
                extract_TagSet(L3_ID, addr, &TAG, &SET);
                INSERT_L3_IN(SET, TAG);

                //extract tag acc. to L2 and insert in L2
                extract_TagSet(L2_ID, addr, &TAG, &SET);
                INSERT_L2_IN(SET, TAG);
              }
            }
          }
        }
      }
      fclose(fp);
    }
    printf("\nCold Misses: %lld", cold_count);
    printf("\n\nUsing INCLUSIVE POLICY");
    printf("\nL2 Hit  = %lld\nL2 Miss = %lld\nL3 Hit  = %lld\nL3 Miss = %lld", Hit_l2, Miss_l2, Hit_l3, Miss_l3);
  }

  //NINE Cache POLICY
  {
    //reset the variables
    flush();
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
        {
          //extract bits according to L2
          extract_TagSet(L2_ID, addr, &TAG, &SET);
          //search in l2
          WAY = L2_SEARCH_IN(SET, TAG);
          if (WAY != -1)
          {
            Hit_l2++;
            //update lru L2 timestamp
            LRU_UPDATE_L2_IN(SET, WAY, 1);
          }
          else
          {
            Miss_l2++;
            //extract bits according to L3
            extract_TagSet(L3_ID, addr, &TAG, &SET);
            //search in L3
            WAY = L3_SEARCH_IN(SET, TAG);
            if (WAY != -1)
            {
              Hit_l3++;
              //update LRU L3
              LRU_UPDATE_L3_IN(SET, WAY, 1);
              //find L2 address and extract tag,set
              extract_TagSet(L2_ID, addr, &TAG, &SET);
              //Query L2 LRU for way no., fill L2 cache and Update LRU L2
              INSERT_L2_IN(SET, TAG);
            }
            else
            {
              Miss_l3++;
              //query LRU L3 to get way no
              INSERT_L3_IN(SET, TAG);

              //extract tag acc. to L2 and insert in L2
              extract_TagSet(L2_ID, addr, &TAG, &SET);
              INSERT_L2_IN(SET, TAG);
            }
          }
        }
      }
      fclose(fp);
    }
    printf("\n\nUsing NINE POLICY");
    printf("\nL2 Hit  = %lld\nL2 Miss = %lld\nL3 Hit  = %lld\nL3 Miss = %lld", Hit_l2, Miss_l2, Hit_l3, Miss_l3);
  }

  //Exclusive
  {
    //reset the variables
    flush();
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
        {
          //extract bits according to L2
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
            extract_TagSet(L3_ID, addr, &TAG, &SET);
            //search in L3
            WAY = L3_SEARCH_IN(SET, TAG);
            if (WAY != -1)
            {
              Hit_l3++;
              //invalidate that way---set timestamp 0
              LRU_UPDATE_L3_IN(SET, WAY, 0);
              extract_TagSet(L2_ID, addr, &TAG, &SET);
              WAY = LRU_QUERY_L2_IN(SET);

              if (Time_Stamp_L2[SET][WAY] == 0)
              {
                //   put data in L2 and update L2
                L2[SET][WAY] = TAG;
                LRU_UPDATE_L2_IN(SET, WAY, 1);
              }
              else
              {
                TAG = L2[SET][WAY];
                New_addr = TAG << 10;
                New_addr = New_addr | SET;
                New_addr <<= 6;
                extract_TagSet(L3_ID, New_addr, &TAG, &SET);
                //search for the tag and get way no
                INSERT_L3_IN(SET, TAG);
                extract_TagSet(L2_ID, addr, &TAG, &SET);
                L2[SET][WAY] = TAG;
                LRU_UPDATE_L2_IN(SET, WAY, 1);
              }
            }
            else
            {
              Miss_l3++;
              extract_TagSet(L2_ID, addr, &TAG, &SET);
              WAY = LRU_QUERY_L2_IN(SET);

              if (Time_Stamp_L2[SET][WAY] == 0)
              {
                //put data in L2 and update L2
                L2[SET][WAY] = TAG;
                LRU_UPDATE_L2_IN(SET, WAY, 1);
              }
              else
              {
                TAG = L2[SET][WAY];
                New_addr = TAG << 10;
                New_addr = New_addr | SET;
                New_addr <<= 6;
                extract_TagSet(L3_ID, New_addr, &TAG, &SET);
                //search for the tag and get way no
                INSERT_L3_IN(SET, TAG);
                extract_TagSet(L2_ID, addr, &TAG, &SET);
                L2[SET][WAY] = TAG;
                LRU_UPDATE_L2_IN(SET, WAY, 1);
              }
            }
          }
        }
      }
      fclose(fp);
    }
    printf("\n\nUsing EXCLUSIVE POLICY");
    printf("\nL2 Hit  = %lld\nL2 Miss = %lld\nL3 Hit  = %lld\nL3 Miss = %lld", Hit_l2, Miss_l2, Hit_l3, Miss_l3);
  }
  return 0;
}
