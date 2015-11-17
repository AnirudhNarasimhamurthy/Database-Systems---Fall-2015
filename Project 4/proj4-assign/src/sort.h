#ifndef __SORT__
#define __SORT__

#include "minirel.h"
#include "new_error.h"
#include "scan.h"

#define    PAGESIZE    MINIBASE_PAGESIZE
#define    NAMELEN     10

class Sort
{
 public:

  Sort(char*    inFile,        // Name of unsorted heapfile.

	   char*        outFile,    // Name of sorted heapfile.

	   int          len_in,      // Number of fields in input records.

	   AttrType     in[],        // Array containing field types of input records.
	                             // i.e. index of in[] ranges from 0 to (len_in - 1)

	   short        str_sizes[], // Array containing field sizes of input records.

	   int          fld_no,     // The number of the field to sort on.
	   // fld_no ranges from 0 to (len_in - 1).

	   TupleOrder   sort_order,   // ASCENDING, DESCENDING

	   int          amt_of_buf,   // Number of buffer pages available for sorting.

	   Status&     s
       );

  ~Sort(){}



  private:

  unsigned short rec_len;            // length of record.

  char *outFileName;        // name of the output file name.

  // make names for temporary heap files.
  void makeHFname( char *name, int passNum, int HFnum );

  void test(int check);

  //int tupleCmp(const void *pRec1, const void *pRec2);

  // first pass.
  Status firstPass( char *inFile, int bufferNum, int& tempHFnum );

  // pass after the first.
  Status followingPass( int passNum, int oldHFnum, 
						int bufferNum, int& newHFnum );

  // merge.
  Status merge( Scan* scan[], int runNum, HeapFile* outHF );

  // find the "smallest" record from runs.
  Status popup( char* record, int *runFlag, int runNum, int& runId );
};

#endif
