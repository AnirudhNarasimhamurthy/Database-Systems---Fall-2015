/* Name: Anirudh Narasimhamurthy
   uID: u0941400
   Title: CS 6530 - Project 1 HFPage */



#include <iostream>
#include <stdlib.h>
#include <memory.h>

#include "hfpage.h"
#include "heapfile.h"
#include "buf.h"
#include "db.h"


// **********************************************************
// page class constructor

void HFPage::init(PageId pageNo)
{
  // fill in the body
  curPage=pageNo;
  nextPage=INVALID_PAGE;
  prevPage=INVALID_PAGE;
  slotCnt=0;
  /* To make sure firstRecord() is working correctly */
  slot[0].length=-1;
  usedPtr= MAX_SPACE - DPFIXED;
  /*The free space initially will also include the sizeof(slot_t). At the beginning sizeof(slot_t) in effect belongs to both the DPFIXED and free space. */
  freeSpace= usedPtr + sizeof(slot_t);
  
  
}

// **********************************************************
// dump page utlity
void HFPage::dumpPage()
{
    int i;

    cout << "dumpPage, this: " << this << endl;
    cout << "curPage= " << curPage << ", nextPage=" << nextPage << endl;
    cout << "usedPtr=" << usedPtr << ",  freeSpace=" << freeSpace
         << ", slotCnt=" << slotCnt << endl;
   
    for (i=0; i < slotCnt; i++) {
        cout << "slot["<< i <<"].offset=" << slot[i].offset
             << ", slot["<< i << "].length=" << slot[i].length << endl;
    }
}

// **********************************************************
PageId HFPage::getPrevPage()
{
    
    
    return prevPage;
}

// **********************************************************
void HFPage::setPrevPage(PageId pageNo)
{

    prevPage=pageNo;
}

// **********************************************************
void HFPage::setNextPage(PageId pageNo)
{
   nextPage=pageNo;
}

// **********************************************************
PageId HFPage::getNextPage()
{
    
    return nextPage;
}

// **********************************************************
// Add a new record to the page. Returns OK if everything went OK
// otherwise, returns DONE if sufficient space does not exist
// RID of the new record is returned via rid parameter.
Status HFPage::insertRecord(char* recPtr, int recLen, RID& rid)
{

/* Checking to see if there is space available for inserting the record */
		
      if(freeSpace >= recLen)
      {
       	 
        /* RecordID =<pageId,SlotNo> */    
    
    	rid.pageNo=curPage;
		rid.slotNo=slotCnt;
         
         
         //Update slotOffset,slotLength
         slot[slotCnt].length=recLen;
         slot[slotCnt].offset=usedPtr - recLen;
         
         /*Inserting a record is equivalent to copying the content from *recPtr
    and dumping it into the data array.memcpy() allows us to do this */
      
         memcpy(data + slot[slotCnt].offset,recPtr,recLen);
         
        
         
         //Increment slot count
         
         slotCnt=slotCnt+1;
         
         //Set the length of the next slot to -1
         
         slot[slotCnt].length=EMPTY_SLOT;
         
         //Update usedPtr and freeSpace
         
         usedPtr=usedPtr-recLen;
         freeSpace=freeSpace - sizeof(slot_t)- recLen;
         
         
         return OK;
       }
       else
       {
         return DONE;
       }         
    
   
}

// **********************************************************
// Delete a record from a page. Returns OK if everything went okay.
// Compacts remaining records but leaves a hole in the slot array.
// Use memmove() rather than memcpy() as space may overlap.
Status HFPage::deleteRecord(const RID& rid)
{
    // fill in the body
    
    int record_slot=rid.slotNo;

    
    /*Checking the validity of the slotNo corresponding to the record id */
    
    if(record_slot > slotCnt || record_slot <0)
    return FAIL;
    
    short record_length=slot[record_slot].length;
    
    /* Checking the validity of the record corresponding to rid */
    
    
    if(record_length == EMPTY_SLOT)
    {
    	return FAIL;
    }
    else
    {
       
         short record_offset=slot[record_slot].offset;
         
         /* If the deleted record corresponds to the last slot entry,
         then compacting the slot array */
         if (record_slot +1 >=slotCnt)
         {
            slotCnt=slotCnt-1;
           
            /* Hole in the slot array for the deleted record */
         
         	slot[record_slot].length=EMPTY_SLOT;
         	slot[record_slot].offset=EMPTY_SLOT;
         
         
        	/* Updating the freespace and the usedPtr values */
         	freeSpace=freeSpace + sizeof(slot_t)+record_length;
         	usedPtr=usedPtr+record_length;
           
           	return OK;
           
           
          }
          
         /* If the slot corresponding to the deleted record is not the last slot 
          Updating the slot offset of the records after the deleted record in slot array*/
         
         else
         {
         
         	/*Compact the data array using memmove*/
         	int len = record_offset - usedPtr;
         	int source=usedPtr;
         	int destination=usedPtr+ record_length;
         	memmove(data+destination,data+usedPtr,len);
         
         	for(int i=record_slot+1;i<slotCnt;i++)
         	{
          		slot[i].offset=slot[i].offset+record_length;
         	} 
         
            /* Hole in the slot array for the deleted record */
         	slot[record_slot].length=EMPTY_SLOT;
         	slot[record_slot].offset=EMPTY_SLOT;
         
        	/* Updating the freespace and the usedPtr values */
         	freeSpace=freeSpace + sizeof(slot_t)+record_length;
         	usedPtr=usedPtr+record_length;
         
         	return OK;
        
         }
    }
    
    
}

// **********************************************************
// returns RID of first record on page
Status HFPage::firstRecord(RID& firstRid)
{
    // fill in the body
    
    /* Looping through the slot array to get the first record */
    
    for (int i=0; i < slotCnt ; i++)
    {
      if (slot[i].length != EMPTY_SLOT)
      {
    	firstRid.slotNo= i; //Setting the slotNo of firstRid to the first record's slot no
    	firstRid.pageNo=curPage;
    	return OK;
      }
    }
    
    /* If none of the records have length !=-1 or empty records, return DONE*/
    return DONE;
      	
}

// **********************************************************
// returns RID of next record on the page
// returns DONE if no more records exist on the page; otherwise OK
Status HFPage::nextRecord (RID curRid, RID& nextRid)
{
    // fill in the body
    
    int val=curRid.slotNo;
    int nextval=val +1;
    
    if (nextval > slotCnt || nextval < 0)
    	return FAIL;
    
    for (int i=nextval; i < slotCnt; i++)
    {
      if(slot[i].length !=EMPTY_SLOT)
      {
        nextRid.slotNo=i;
        nextRid.pageNo=curPage;
        return OK;
        
      }
    }
     
     return DONE; 
    
}

// **********************************************************
// returns length and copies out record with RID rid
Status HFPage::getRecord(RID rid, char* recPtr, int& recLen)
{
    // fill in the body
    
    /*Get the rid's corresponding slotNo and then from the slot's offset
    get the corresponding record */
    
    int record_slot = rid.slotNo;
    
    /* Checking if the given slot for the rid is invalid.If so return FAIL */
    
    if (record_slot > slotCnt || record_slot < 0)
    return FAIL;
    
    short record_length=slot[record_slot].length;
    
    if(record_length == EMPTY_SLOT)
    {
    	return FAIL;
    }
    else
    {	
    
    	short record_offset=slot[record_slot].offset;
    	memcpy(recPtr,data +record_offset, record_length);
    	recLen= record_length;
    	return OK;
    }
    
    	
}

// **********************************************************
// returns length and pointer to record with RID rid.  The difference
// between this and getRecord is that getRecord copies out the record
// into recPtr, while this function returns a pointer to the record
// in recPtr.
Status HFPage::returnRecord(RID rid, char*& recPtr, int& recLen)
{
    /*Get the rid's corresponding slotNo and then from the slot's offset
    get the corresponding record */
    
    int record_slot = rid.slotNo;
    
    /* Checking if the given slot for the rid is invalid.If so return FAIL */
    
    if (record_slot > slotCnt || record_slot < 0)
    return FAIL;
    
    short record_length=slot[record_slot].length;
    
    if(record_length == EMPTY_SLOT)
    {
    	return FAIL;
    }
    else
    {	
    
    	short record_offset=slot[record_slot].offset;
    	//memcpy(recPtr,data +record_offset, record_length);
    	recPtr= data+record_offset;
    	recLen= record_length;
    	return OK;
    }
}

// **********************************************************
// Returns the amount of available space on the heap file page
int HFPage::available_space(void)
{
    // fill in the body
    
    /*In our case because our data array starts at the end and slot array is at the beginning, the available space would be the space between first byte of data array - last byte of slot array */
    
    //int availableSpace=usedPtr - slotCnt * sizeof(slot_t);
    int availableSpace=freeSpace -sizeof(slot_t);
    return availableSpace;
}

// **********************************************************
// Returns 1 if the HFPage is empty, and 0 otherwise.
// It scans the slot directory looking for a non-empty slot.
bool HFPage::empty(void)
{
    // fill in the body
    for(int i=0;i <slotCnt; i++)
    {
    	if(slot[i].length != EMPTY_SLOT)
    	return false ;
    }
    
    return true;
}



