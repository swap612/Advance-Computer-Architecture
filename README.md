# Advance-Computer-Architecture-Assignment1
<pre>
Problem 1 [70 points]: The L1 cache miss traces for six SPEC 2006 applications 
are available in the directory.  These are detailed below.

bzip2: two trace files (bzip2.log_l1misstrace_0, bzip2.log_l1misstrace_1)
gcc: two trace files (gcc.log_l1misstrace_0, gcc.log_l1misstrace_1)
gromace: one trace file (gromacs.log_l1misstrace_0)
h264ref: one trace file (h264ref.log_l1misstrace_0)
hmmer: one trace file (hmmer.log_l1misstrace_0)
sphinx3: two trace files (sphinx3.log_l1misstrace_0, sphinx3.log_l1misstrace_1)

Copy these into your space. These are binary files, so they won't open with
any conventional editors. If you want to read about these applications, please
visit http://www.spec.org/cpu2006/. The cases where you have two trace files,
these are essentially two consecutive parts of the same trace. The applications
with two trace files miss a lot in the L1 caches. As a result, the L1 cache
miss trace is split into two parts to keep the size of each file within limit.
You need to process these trace files in the order they are numbered.

Each trace entry has the following fields in that order.

char i_or_d;	// Instruction or data miss (you can ignore)

char type;	// A data miss or write permission miss
		// A zero value means write permission miss and you can
		// treat these as hits. A non-zero value means a genuine L1
		// cache miss.

unsigned long long addr;	// Miss address

unsigned pc;	// Program counter of load/store instruction that missed

For this problem, you won't require the i_or_d and pc fields.

Here is a code snippet that you can use to read the traces. Assume that you
take the file name prefix (e.g., bzip2.log_l1misstrace) and the number of traces
for the application under question (e.g., 2 for bzip2) from command line.

int numtraces = atoi(argv[2]);
for (k=0; k<numtraces; k++) {
      sprintf(input_name, "%s_%d", argv[1], k);
      fp = fopen(input_name, "rb");
      assert(fp != NULL);

      while (!feof(fp)) {
         fread(&iord, sizeof(char), 1, fp);
         fread(&type, sizeof(char), 1, fp);
         fread(&addr, sizeof(unsigned long long), 1, fp);
         fread(&pc, sizeof(unsigned), 1, fp);

         // Process the entry
      }
      fclose(fp);
      printf("Done reading file %d!\n", k);
}

In this problem, you need to simulate a two-level cache hierarchy and pass the
L1 cache miss trace through them. I will call the two levels of the hierarchy
L2 and L3. The L2 cache should be 512 KB in size, 8-way set associative, and
have 64 bytes line size. The L3 cache should be 2 MB in size, 16-way set
associative, and have 64 bytes line size. You need to report the number of
L2 and L3 cache misses for each of the six applications for the following three cases.

A. L3 is inclusive of L2: An L3 miss fills into L3 and L2. An L3 eviction
invalidates the corresponding block in L2.
B. L3 is non-inclusive-non-exclusive (NINE) of L2: An L3 miss fills into L3 and L2.
An L3 eviction does not invalidate the corresponding block in L2.
C. L3 is exclusive of L2: An L3 miss fills into L2. An L2 eviction allocates
the block in L3. An L3 hit invalidates the block in L3.

In all cases, both L2 and L3 caches should execute LRU replacement. However,
before invoking the replacement policy, you should look for an invalid way in 
the target set. The invalid ways should be filled first before replacing any
valid block. Note that you only need to simulate the tags in the cache to measure 
the number of misses. The data values are not relevant. As a result, each level of
cache is essentially a 2D array of tags where each row can be seen as a set.
You also need to have the necessary LRU states per set.

Prepare a table to summarize your results. Explain any difference in the
number of misses seen across the three configurations.

Problem 2 [30 points]: In the last problem, classify the L3 misses in the case
of an inclusive L3 (case A) into cold, conflict, and capacity misses. For each of 
the six applications, report the number of these misses in a table. 

</pre>
