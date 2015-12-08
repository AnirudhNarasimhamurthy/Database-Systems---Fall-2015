
#include <string.h>
#include <assert.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include "sortMerge.h"
#include "../include/system_defs.h"
#include "../include/sort.h"
#include "../include/heapfile.h"

// Error Protocall:

enum ErrCodes {};

static const char* ErrMsgs[] = 	{};

static error_string_table ErrTable( JOINS, ErrMsgs );

sortMerge::sortMerge(
    char*           filename1,      // Name of heapfile for relation R
    int             len_in1,        // # of columns in R.
    AttrType        in1[],          // Array containing field types of R.
    short           t1_str_sizes[], // Array containing size of columns in R
    int             join_col_in1,   // The join column of R 

    char*           filename2,      // Name of heapfile for relation S
    int             len_in2,        // # of columns in S.
    AttrType        in2[],          // Array containing field types of S.
    short           t2_str_sizes[], // Array containing size of columns in S
    int             join_col_in2,   // The join column of S

    char*           filename3,      // Name of heapfile for merged results
    int             amt_of_mem,     // Number of pages available
    TupleOrder      order,          // Sorting order: Ascending or Descending
    Status&         s               // Status of constructor
)
{


    //Variable declarations

    char sorted_filename1[25];
    char sorted_filename2[25];
    Status s1,s2,s3;
    RID rid1,rid2,rid3,rid4;
    int len;

    char buffer1[amt_of_mem];
    char buffer2[amt_of_mem];
    char recptr1[PAGESIZE];
    char recptr2[PAGESIZE];
    char recptr3[PAGESIZE];
    //recptr1= buffer1;
    //recptr3= buffer2;

    //First we need to sort the input files filename1 and filename2 based on the join attribute

    //Sorting File1



    Sort sort1(filename1,sorted_filename1,len_in1,in1,t1_str_sizes,join_col_in1,order,amt_of_mem,s1);
    if (s1!=OK)
    {
        minibase_errors.show_errors();
        minibase_errors.clear_errors();
        MINIBASE_CHAIN_ERROR(JOINS,s1);
        return;
    }

    //Obtaining the record count from sorted_filename1

    HeapFile check_1(sorted_filename1,s3);
    if(s3 != OK)
    {
        minibase_errors.show_errors();
        minibase_errors.clear_errors();
        return;
    }

    int record_count1=check_1.getRecCnt();
    //cout <<"\nFile1 Record count is :" <<record_count1;


    //Sorting File2

    Sort sort2(filename2,sorted_filename2,len_in2,in2,t2_str_sizes,join_col_in2,order,amt_of_mem,s2);
    if (s2!=OK)
    {
        MINIBASE_CHAIN_ERROR(HEAPFILE,s1);
        return;
    }

    //Obtaining the record count from sorted_filename2

    HeapFile check_2(sorted_filename2,s3);
    if(s3 != OK)
    {
        MINIBASE_CHAIN_ERROR(HEAPFILE,s1);
        return;
    }

    int record_count2=check_2.getRecCnt();
    //cout <<"\nFile2 Record count is :" <<record_count2;


   // Open scans on sorted_filename1 and sorted_filename2

    Scan*	scan1 = check_1.openScan(s3);
    if(s3 != OK)
    {
        cout <<"\n Error !!";
        MINIBASE_CHAIN_ERROR(HEAPFILE,s3);
        minibase_errors.show_errors();
        minibase_errors.clear_errors();
        return;
    }

    s = scan1->getNext(rid1, recptr1, len);
    if (s != OK)
    {
        cout <<"\n Error getNext 1!!";
        minibase_errors.show_errors();
        minibase_errors.clear_errors();
        return;
    }


    Scan*	scan2 = check_2.openScan(s3);
    if(s3 != OK)
    {
        cout <<"\n Error scan2 !!";
        minibase_errors.show_errors();
        minibase_errors.clear_errors();
        return;

    }

    s = scan2->getNext(rid2, recptr3, len);
    if (s != OK)
    {
        minibase_errors.show_errors();
        minibase_errors.clear_errors();
        return;
    }
   /* s2=scan2->getNext(rid3, recptr2, len);
    if (s2!=OK)
    {
        minibase_errors.show_errors();
        minibase_errors.clear_errors();
        return;
    }*/


    //cout<<"\nSorted File name1 is :"<<sorted_filename1;
    //cout<<"\nSorted File name2 is :"<<sorted_filename2;
    HeapFile o(filename3,s);
    if (s != OK)
    {
        MINIBASE_CHAIN_ERROR(HEAPFILE,s);
        return;
    }


    while ( record_count1 > 0 && record_count2  > 0)
    {

        while (tupleCmp(recptr1,recptr3) == -1) //Record in R is less than S. So move to next tuple of R
        {
            //printf("\nBefore (R<S) %d : %d",recptr1[0],recptr3[0]);
            s = scan1->getNext(rid1, recptr1, len);
            if (s!=OK)
            {
                //cout <<"\nNo more tuples is R first while";
                delete(scan1);
                delete(scan2);
                check_1.deleteFile();
                check_2.deleteFile();
                s=OK; //Setting s=OK to make sure the correct status is passed at the end of the constructor.
                return;
            }
            record_count1=record_count1-1;
           // cout <<"\nRecord count 1(R  < S) is:"<<record_count1;
            //cout <<"\nRecord count 2(R < S) is:"<<record_count2;

        }


        while (tupleCmp(recptr1,recptr3)== 1) //Record in R is more than S. So move to next tuple of S
        {

            //printf("\nBefore (R>S) %d : %d",recptr1[0],recptr3[0]);
            s = scan2->getNext(rid2, recptr3, len);
            if (s!=OK)
            {
                //cout <<"\nNo more tuples is S second while";
                delete(scan1);
                delete(scan2);
                check_1.deleteFile();
                check_2.deleteFile();
                s=OK; //Setting s=OK to make sure the correct status is passed at the end of the constructor.
                return;
            }
            record_count2=record_count2-1;
            //cout <<"\nRecord count 1(R > S) is:"<<record_count1;
            //cout <<"\nRecord count 2(R > S) is:"<<record_count2;

        }


        rid3=rid2;

        int count=0;
        int internal_count=0;
        int outer_flag=0;
        int temp=record_count2;
        int flag=0;

        while (tupleCmp(recptr1,recptr3)==0) //Equal or match found
        {


            //printf("\n%d : %d", recptr1[0], recptr3[0]);
            record_count2=temp;
            s= scan2->position(rid2);
            s2 = scan2->getNext(rid3, recptr2, len);
            if (s2!=OK)
            {
                //cout<<"Inside while match but before inner while match";
                delete(scan1);
                delete(scan2);
                check_1.deleteFile();
                check_2.deleteFile();
                s=OK; //Setting s=OK to make sure the correct status is passed at the end of the constructor.
                return;
            }

            while (tupleCmp(recptr1, recptr2) == 0) {

                count = count + 1;
                internal_count=internal_count+1;


                char out[2*len];
                //Concatenating the values and writing it to the output file

                memcpy(out,recptr1,len);
                memcpy(out+len,recptr2,len);
                o.insertRecord(out,2*len,rid4);

                //printf("\n%d : %d", recptr1[0], recptr2[0]);
                s = scan2->getNext(rid3, recptr2, len);
                if (s != OK)
                {
                    //cout <<"\nNo more tuples is S inside fourth while";
                    delete(scan1);
                    delete(scan2);
                    check_1.deleteFile();
                    check_2.deleteFile();
                    s=OK; //Setting s=OK to make sure the correct status is passed at the end of the constructor.
                    return;
                }
                record_count2=record_count2-1;
                outer_flag=1;


            }

            s = scan1->getNext(rid1, recptr1, len);
            if (s != OK)
            {
                //cout <<"\nNo more tuples is R inside third while";
                delete(scan1);
                delete(scan2);
                check_1.deleteFile();
                check_2.deleteFile();
                s=OK; //Setting s=OK to make sure the correct status is passed at the end of the constructor.
                return;
            }
            record_count1 = record_count1 - 1;

        }

        /*if (internal_count==1)
        {
            cout <<"\nInternal count true";
            record_count2=record_count2-1;
            record_count1=record_count1-1;
        }*/

        if (outer_flag==1)
        {
            s= scan2->position(rid3);
            s2=scan2->getNext(rid2,recptr3,len);
            if (s2!=OK)
            {
                //cout <<"\nNo more tuples is S outside third while";
                delete(scan1);
                delete(scan2);
                check_1.deleteFile();
                check_2.deleteFile();
                s=OK; //Setting s=OK to make sure the correct status is passed at the end of the constructor.
                return;
            }
            //recptr3[0]=recptr2[0];

        }



    }




}

sortMerge::~sortMerge()
{


}


