// file sort.C  --  implementation of Sort class.

#include "sort.h"
#include "heapfile.h"
#include "system_defs.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

// Add you error message here
// static const char *sortErrMsgs[] {
// };
// static error_string_table sortTable(SORT, sortErrMsgs);


/* Member variables required to be used in the tupleCmp function */

int offset;
int fieldLength;
AttrType attrType;
TupleOrder sortOrder1;
int run_no;
int followingpass_run_no;


// constructor.
Sort::Sort ( char* inFile, char* outFile, int len_in, AttrType in[],
	     short str_sizes[], int fld_no, TupleOrder sort_order,
	     int amt_of_buf, Status& s )
{

	/* Setting the values for the member variables used inside tupleCmp function */

	int temp_offset=0;
	int temp_len=0;
	for (int i=0; i < len_in; i++)
	{
		if (i==fld_no) {
			offset = temp_offset;
			break;
		}
		else
		{
		  temp_offset=temp_offset+str_sizes[i];
		}

	}
	fieldLength=str_sizes[fld_no];
	attrType=in[fld_no];
	sortOrder1=sort_order;

	/*cout << "\noffset: " << offset;
	cout << "\nfieldLength: " <<fieldLength;
	cout << "\nattrType: " << attrType;
	cout << "\nsortOrder1: " <<sortOrder1 <<"\n";*/

	/* Computing the record length   */

	for (int i=0;i < len_in;i++)
	{
		temp_len = temp_len + str_sizes[i];
	}
	rec_len=temp_len;

	/* Setting the outFileName value */

	outFileName=outFile;

	firstPass(inFile,amt_of_buf,run_no);
	//cout <<"\nThe number of runs after First Pass is : " <<run_no<<"\n";
	followingPass(1,run_no,amt_of_buf,followingpass_run_no);

}

int tupleCmp(const void *pRec1, const void *pRec2)
{
	int result;

	char *rec1 = (char *)pRec1;
	char *rec2 = (char *)pRec2;

	if (attrType == attrInteger)
		result = *(int *)(rec1 + offset) - *(int *)(rec2 + offset);
	else
		result = (strncmp(&rec1[offset], &rec2[offset], fieldLength));

	if (result < 0)
	if (sortOrder1 == Ascending)
		return -1;
	else
		return 1;
	else
	if (result > 0)
	if (sortOrder1 == Ascending)
		return 1;
	else
		return -1;
	else
		return 0;
}


void Sort::test(int number)
{
	cout <<"Ok this works !!" << number;
}

// make names for temporary heap files.
void  Sort::makeHFname( char *name, int passNum, int HFnum )
{

	//Naming convention followed for the file name is :<My uId>_<Pass> pass no_<Run> run no

	sprintf (name, "u0941400_Pass_%d_Run_%d",passNum,HFnum);
	//cout << "\n" << name;

}


Status Sort::firstPass( char *inFile, int bufferNum, int& tempHFnum )
{

	/* Variables which are required to be passed as parameters to different functions */
	Status s;
	RID rid;
	char res; //Used to pass values to the getNext method of scan
	int len; // Used to pass values to the getNext method of scan and also used to move the recPtr

	run_no=0;

	char file_name[25];

	int buffer_size=PAGESIZE * bufferNum;
	int available_memory= buffer_size;
	int rec_count_tracker=0;

	//cout <<"\n Buffer size is : " <<buffer_size;
	char temp_buffer[buffer_size]; //Temp_buffer to store all the records

	/* Pointers to the buffer array used for keeping track of the records that need to be */

	char *recptr;
	char *outPtr;
	char *heapPtr;

	recptr=temp_buffer;
	outPtr=temp_buffer;
	heapPtr=temp_buffer;

	HeapFile f(inFile,s); //Creating HeapFile object and calling th constructor

	if(s != OK)
		return  MINIBASE_CHAIN_ERROR( HEAPFILE, s );

	// initiate a sequential scan
	Scan *sc= f.openScan(s);

	if (s != OK)
		return  MINIBASE_CHAIN_ERROR( HEAPFILE, s );
	int record_count=f.getRecCnt();

	if (record_count==0)    // File to be sorted has no records. Simply return OK
		return OK;

	int max_no_records_in_buffer=record_count/bufferNum;

	bool isEqual_dist = (record_count%bufferNum == 0);

	for (int i=0;i < record_count;i++)
	{
		s = sc->getNext(rid, recptr, len);
		if (s != OK)
			return  MINIBASE_CHAIN_ERROR( HEAPFILE, s );

		if (available_memory > len and rec_count_tracker != record_count -1)
		//Read the record into the temp buffer, decrement the available memory, increment the record count tracker
		{
			recptr = recptr + len;
			available_memory = available_memory - len;
			rec_count_tracker=rec_count_tracker+1;

			if (available_memory - len < 0) {

				// Sort the records using qsort
				qsort(temp_buffer, rec_count_tracker, rec_len, tupleCmp);
				for (int i = 0; i < rec_count_tracker; i++) {
					//cout << temp_buffer[i] << "  ";
				}
				//Increment the run no
				run_no = run_no + 1;

				// Write the records to a temporary heap file and as well as outFile

				makeHFname(file_name, 1, run_no);
				HeapFile w(file_name, s);
				if (s != OK)
					return MINIBASE_CHAIN_ERROR(HEAPFILE, s);

				//Writing the sorted records to the temp heap file

				for (int i = 0; i < rec_count_tracker; i++) {
					s = w.insertRecord(heapPtr, rec_len, rid);
					heapPtr = heapPtr + rec_len;
					//cout << "\n Inserting record " << i + 1 << *heapPtr << " into temporary heap file";
					if (s != OK) {
						return MINIBASE_CHAIN_ERROR(HEAPFILE, s);
					}
				}

				// Re-set the available memory to the initial limit and make the pointers to point to start of temp_buffer

				available_memory=PAGESIZE*bufferNum;
				recptr = temp_buffer;
				heapPtr = temp_buffer;
				outPtr = temp_buffer;
				rec_count_tracker=0;
			}
		}
		else if (available_memory > len and i==record_count-1)
		{

			recptr = recptr + len;
			available_memory = available_memory - len;
			rec_count_tracker=rec_count_tracker+1;

			qsort(temp_buffer,rec_count_tracker,rec_len,tupleCmp);
			run_no=run_no+1;
			tempHFnum=run_no; // Setting the value of tempHFnum which is passed as reference.
			makeHFname(file_name,1,run_no);

			HeapFile w(file_name, s);

			if (s!=OK)
				return MINIBASE_CHAIN_ERROR( HEAPFILE, s );

			//Writing the sorted records to the temp heap file

			for (int i=0; i < rec_count_tracker  ; i++)
			{

				//cout <<"\n Inserting record " <<i+1 << *heapPtr <<" into temporary heap file";
				s = w.insertRecord(heapPtr, rec_len, rid);
				heapPtr=heapPtr+rec_len;
				if ( s!=OK)
				{
					return MINIBASE_CHAIN_ERROR( HEAPFILE, s );
				}
			}

			//Writing the sorted records to the  outFile

			HeapFile o(outFileName, s);

			if (s!=OK)
				return MINIBASE_CHAIN_ERROR( HEAPFILE, s );

			for (int i=0; i < rec_count_tracker ; i++)
			{

				//cout <<"\n Inserting record " <<i+1 << "\t" <<*outPtr <<" " <<*(outPtr+1)<<" into outFile";
				s = o.insertRecord(outPtr, rec_len, rid);
			 	outPtr=outPtr+rec_len;
				if ( s!=OK)
				{
					return MINIBASE_CHAIN_ERROR( HEAPFILE, s );
				}
			}
			//Destroy the output file object when we have more than one run because the actual output would be produced in the following pass.

			if(run_no > 1)
			{
				Status stat = o.deleteFile();
				if (stat !=OK)
					return MINIBASE_CHAIN_ERROR( HEAPFILE, stat );

			}

			delete(sc);
			return OK;

		}
	}

}


// pass after the first.
Status Sort::followingPass( int passNum, int oldHFnum,
					  int bufferNum, int& newHFnum )

{

	int no_of_input_buffers = bufferNum -1;
	int no_of_output_buffer = 1;
	int oldpassNum=passNum;
	int iterative_hfnum;
	int run_no;
	Scan *scan[no_of_input_buffers];
	int scan_count;
	int heap_files_processed;
	char file[25];
	Status s1;
	//Setting the newHFnum initially to zero ans then updating it accordingly inside the function.

	newHFnum=0;

	//Read in bufferNum -1 files and open scans on each of them

	if (oldHFnum <=1)
		return OK;

	while(newHFnum != 1)
	{
		scan_count= 0;
		heap_files_processed=0;
		run_no=0;

		for (int i = 0; i < oldHFnum; i++)
		{
				makeHFname(file,passNum,i+1);
				HeapFile f1(file, s1);

				if (s1 != OK) {
					cout << "\n Error here in opening heap file";
					return MINIBASE_CHAIN_ERROR(HEAPFILE, s1);
				}

				//Open scans on each of them

				Scan *sc1 = f1.openScan(s1);
				if (s1 != OK) {
					cout << "\n Error here in opening scan";
					return MINIBASE_CHAIN_ERROR(HEAPFILE, s1);
				}

				scan[scan_count] = sc1;

				scan_count = scan_count + 1;
				heap_files_processed = heap_files_processed + 1;

			  if (scan_count < bufferNum - 1  && heap_files_processed != oldHFnum)
				  continue;

			//After setting up scan array and run number call the merge function

			  if(scan_count==bufferNum-1 && newHFnum==bufferNum-1)
			  {

				  run_no = run_no + 1;
				  //cout <<"\n Run no is:" <<run_no;
				  HeapFile o(outFileName,s1);
				  if(s1!=OK)
					  return MINIBASE_CHAIN_ERROR(HEAPFILE, s1);

				  merge(scan,scan_count,&o);

				  //Re-set the scan_count for processing the next set of (numBuffer-1) heap files
				  scan_count = 0;
				  return OK;

			  }

			  if(scan_count == bufferNum-1)
			  {
				  //Set up a name for the heap file
				  //cout<<"\nWriting the results to temp heapfile";
				  run_no = run_no + 1;
				  makeHFname(file,passNum+1,run_no);
				  //cout <<"\nRun no is:" <<run_no;

				  HeapFile w(file,s1);
				  if(s1!=OK)
					  return MINIBASE_CHAIN_ERROR(HEAPFILE, s1);

				  merge(scan,scan_count,&w);

				  //Re-set the scan_count for processing the next set of (numBuffer-1) heap files
				  scan_count = 0;
			  }
		}

		//The end of for-loop indicates the processing of all the files for that pass. The run count now will tell us the number
		//of runs produced from the following pass. We should set that to the newHFnum variable.

		passNum = passNum + 1;
		newHFnum = run_no;
		oldHFnum=run_no;
		//cout <<"\nPass no is: " <<passNum;
		//cout << "\nFinal Number of runs produced:" << newHFnum;

	}
	return OK;


}

// merge.
Status Sort::merge( Scan* scan[], int runNum, HeapFile* outHF )
{
	//Member  variables declaration


	int len;
	RID rid;
	int runId;

	Status s;
	char record[runNum*rec_len];
	char *recptr;
	recptr=record;

	int runflag[runNum];

	/* By default setting  all the runFlags to be 1 */
	for (int i=0;i<runNum;i++)
		runflag[i]=1;


	char file[25];

	// Construct the record array by using the pointers from the scan array

	for (int i=0;i <runNum;i++)
	{
		s = scan[i]->getNext(rid, recptr, len);

		//Create or set the run flags for each of the arrays
		if (s != OK)
		{
			//cout <<"\nNo more records.Setting run flag to 0";
			runflag[i] = 0;
		}
		recptr = recptr + len;
	}

	int count=0;
	while(1)
	{
		//Call popup function after constructing the record array

		Status s1= popup(record,runflag,runNum,runId);
		if (s1==DONE)
		{
			//cout <<"\nIs this true ? Are we done ?";
			break;
		}

		recptr=record+ runId*rec_len;
		count=count+1;

		//cout <<"\n Inserting record " << count<< "\t" <<*recptr<<" " <<*(recptr+1)<<" into outFile";
		s = outHF->insertRecord(recptr, rec_len, rid);
		if (s!= OK)
			return MINIBASE_CHAIN_ERROR(HEAPFILE, s);

		//Get the next record from the file which had the lowest record and so the pointer keeps moving.
		s = scan[runId]->getNext(rid, recptr, len);
		if (s!= OK)
			runflag[runId]=0;

	}

	return OK;

}

// find the "smallest" record from runs.
Status Sort::popup( char* record, int *runFlag, int runNum, int& runId )
{

	/*Set the first element with non-zero run Flag as minimum. If none of them have a non-zero runFlag then we are done*/

	char *minimum_ele;
	int zero_flag=0;
	for (int i=0;i<runNum;i++)
	{
		if (runFlag[i]==1)
		{
			//cout <<"\nRun id:"<<runId;
			runId=i;
			zero_flag=1;
			break;
		}

	}

	if(zero_flag==0)
		return DONE;

	minimum_ele=record + runId*rec_len;

	for (int j=runId+1;j<runNum;j++)
	{
		if (runFlag[j] != 0)
		{
			char *record_to_compare= record + (j*rec_len);

			int result =tupleCmp(minimum_ele,record_to_compare);
			if (result == 1)
			{
				/*Set the minimum element and the corresponding runID */
				minimum_ele = record_to_compare;
				runId=j;
			}

		}

	}

	return OK;

}