/******* PROJECT 2 - BUFFER MANAGER ****************
******* Name : Anirudh Narasimhamurthy ************
******* uID: u0941400 ****************************/


/*****************************************************************************/
/*************** Implementation of the Buffer Manager Layer ******************/
/*****************************************************************************/


#include "buf.h"
#include<iostream>
#include <list>
#define EMPTY_FRAME -1

// Define buffer manager error messages here
//enum bufErrCodes  {};

// Define error message here
static const char* bufErrMsgs[] = {

        /*HASH_TBL_ERROR,
        HASH_NOT_FOUND,
        BUFFER_EXCEEDED,
        "Page cannot be pinned. No empty frames available and hate/love queues are empty", //CANNOT_PIN_PAGE
        "Page to be unpinned is not pinned in the first place !", //PAGE_NOT_PINNED
        BAD_BUFFER,
        "Page is pinned. Cannot free a pinned page !", //PAGE_PINNED,
        REPLACER_ERROR,
        BAD_BUF_FRAMENO,
        "Page not found in the hash table !", //PAGE_NOT_FOUND,
        "Frame is empty" //FRAME_EMPTY*/
};

// Create a static "error_string_table" object and register the error messages
// with minibase system 
static error_string_table bufTable(BUFMGR,bufErrMsgs);


BufMgr::BufMgr (int numbuf, Replacer *replacer)
{

  /* Allocating memory for buffer pool and buffer descriptor and setting the initial pageNo to empty to indicate the corresponding
   * frame is available in the buffer pool to be used */

  numBuf=numbuf;
  bufPool=(Page*)malloc(sizeof(Page) * numbuf);
  bufDescr=(BufDescr*)malloc(sizeof(BufDescr) * numbuf);
  bufCnt=0;
  for (int i=0;i<numbuf;i++)
  {
      bufDescr[i].pageNo=EMPTY_FRAME;
      bufDescr[i].pin_count=0;
      bufDescr[i].dirtybit= false;
      bufDescr[i].hateFlag= true;                       //Assuming by default all pages are hated, just for ease of code

  }
  htab.init();                                             //Calling the init to initialize directory for hash table
}


Status BufMgr::pinPage(PageId PageId_in_a_DB, Page*& page, int emptyPage)
{

  /* If the page to be pinned is already there in the buffer pool or hash table */

  int frame_no = htab.search(PageId_in_a_DB);             //Check in the hashtable if the page is present

  if (frame_no != -1)                                     //Page is already there in hash table
  {
    if (bufDescr[frame_no].pin_count == 0)                //If the pin count=0, then page is in Love or Hate Queue, then remove the entry from the Queue
    {
      if (bufDescr[frame_no].hateFlag == true)
      {
        rq.remove_from_hate_queue(PageId_in_a_DB);       //Remove from hate queue
      }
      else
      {
        rq.remove_from_love_queue(PageId_in_a_DB);      //Remove from love queue
      }

      page= &bufPool[frame_no];                         //Set the page pointer to this frame

    }


    bufDescr[frame_no].pin_count += 1;                   //Else just increment the pin count for the page
    page= &bufPool[frame_no];                            // Set the page pointer to this frame

    return OK;

  }

  /* Else if page is not present, check if there are any empty frames available in bufPool and pin the page in the empty frame in
   * buffer pool */

  else if (frame_no == -1 && bufCnt < numBuf)
  {
    for (int i = 0; i < numBuf; i++) {
      if (bufDescr[i].pageNo == EMPTY_FRAME)              //Free frame is available
      {

        Status stat = MINIBASE_DB->read_page(PageId_in_a_DB, &bufPool[i]);   //Call the Disk Manager to read the page
        if (stat != 0)
        {
          return MINIBASE_CHAIN_ERROR(BUFMGR, stat);
        }


        htab.insert(PageId_in_a_DB, i);                         //Insert the page to the hash table list

        page = &bufPool[i];                                    //Set the page pointer
        bufDescr[i].pageNo = PageId_in_a_DB;                   //Update the pageNo
        bufDescr[i].pin_count = bufDescr[i].pin_count + 1;     //Update pincount
        bufDescr[i].dirtybit = false;                          // Update dirty bit values
        bufCnt = bufCnt + 1;
        return OK;

      }
    }
  }

  /* Else if there are no empty frames available, select a frame based on replacement policy */
  else if (frame_no == -1 && bufCnt == numBuf) {

    /* First look into the hate queue for replacing a page */
    int poppedPageHQ = rq.pop_hate_queue();                 //Returns a page no if popped from Hate Queue

    if (poppedPageHQ != -1)                                 // Indicates Page has been popped from queue
    {


      int poppedFrameHQ = htab.search(poppedPageHQ);      //Find the corresponding page's frame no and then check for conditions

      if (bufDescr[poppedFrameHQ].dirtybit == true)      //If the frame has its dirty bit set, write the contents of the frame to memory
      {
        Status stat1 = MINIBASE_DB->write_page(bufDescr[poppedFrameHQ].pageNo, &bufPool[poppedFrameHQ]);
        if (stat1 != 0) {
          return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
        }
      }

      /* Read the page/frame in to the buffer pool. */

      Status stat = MINIBASE_DB->read_page(PageId_in_a_DB, &bufPool[poppedFrameHQ]);
      if (stat != 0) {
        return MINIBASE_CHAIN_ERROR(BUFMGR, stat);
      }

      bufDescr[poppedFrameHQ].pin_count = 0;                    //Initialize the picCount to 0
      bufDescr[poppedFrameHQ].dirtybit = false;                 //Set dirtybit to FALSE



      htab.delete_entry(poppedPageHQ);                      //Delete the entry for the old page from the hash table
      htab.insert(PageId_in_a_DB, poppedFrameHQ);           // Add a new entry for the new page no in the hash table

      page = &bufPool[poppedFrameHQ];                               // Set the address
      bufDescr[poppedFrameHQ].pin_count += 1;                       //Update the frame's pin count
      bufDescr[poppedFrameHQ].pageNo = PageId_in_a_DB;              // Update the frame descriptor's page No
      bufDescr[poppedFrameHQ].hateFlag = true;


      return OK;

    }
    else if (poppedPageHQ == -1)                        //No frames to remove from Hate Queue.So pop from the Love Queue
    {
      int poppedPageLQ = rq.pop_love_queue();
      if (poppedPageLQ != -1)
      {                                                 //A page has been popped from love queue

        int poppedFrameLQ = htab.search(poppedPageLQ);

        if (bufDescr[poppedFrameLQ].dirtybit ==
            true) //If the frame has its dirty bit set, write the contents of the frame to memory
        {
          Status stat1 = MINIBASE_DB->write_page(bufDescr[poppedFrameLQ].pageNo, &bufPool[poppedFrameLQ]);
          if (stat1 != 0) {
            cout << "Error !! " << "\n";
            return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
          }
        }

        Status stat = MINIBASE_DB->read_page(PageId_in_a_DB,
                                             &bufPool[poppedFrameLQ]);     /* Read the page/frame  in to the buffer pool. */
        if (stat != 0)
        {
          return MINIBASE_CHAIN_ERROR(BUFMGR, stat);
        }
        bufDescr[poppedFrameLQ].pin_count = 0;                         //Initialize the pin count to 0
        bufDescr[poppedFrameLQ].dirtybit = false;                     // Initialize the dirty bit to false


        htab.delete_entry(poppedPageLQ);                          //Delete the entry for the old page from the hash table

        htab.insert(PageId_in_a_DB,
                    poppedFrameLQ);                               //Add a new entry for the new page no in the hash table */

        page = &bufPool[poppedFrameLQ];
        bufDescr[poppedFrameLQ].pin_count += 1;                   //Update the pin count
        bufDescr[poppedFrameLQ].pageNo = PageId_in_a_DB;          //Update the frame descriptor's page
        bufDescr[poppedFrameLQ].hateFlag = true;                  // Update the hateFlag

        return OK;
      }

      else                                                      //There are no frames there in both hate or love queue
      {
        //return MINIBASE_CHAIN_ERROR(BUFMGR,CANNOT_PIN_PAGE);
        return FAIL;
      }
    }                                                           //end-if PoppedPageHQ
  }                                                            //End of else-if
}                                                             //end pinPage


Status BufMgr::newPage(PageId& firstPageId, Page*& firstpage, int howmany)
{


    Status stat = MINIBASE_DB->allocate_page(firstPageId,howmany);       //Allocate memory for the pages

    if(stat != 0)
    {
      return MINIBASE_CHAIN_ERROR(BUFMGR,stat);
    }

    Status stat2=pinPage(firstPageId,firstpage);                      //Pin the first page

    if(stat2 != 0)                                                    // If the first page could not be pinned, deallocate pages
    {
      MINIBASE_DB->deallocate_page(firstPageId,howmany);
      return FAIL;
      //return MINIBASE_CHAIN_ERROR(BUFMGR,CANNOT_PIN_PAGE);         //Page cannot be pinned.So deallocate memory.
    }

    return OK;

}

Status BufMgr::flushPage(PageId pageid) {

  int frame_no=htab.search(pageid);
  if(frame_no == -1)
  {
    return FAIL;
    //return MINIBASE_CHAIN_ERROR(BUFMGR,PAGE_NOT_FOUND);
  }

  if (bufDescr[frame_no].dirtybit==TRUE)
  {

    Status stat1 = MINIBASE_DB->write_page(pageid, &bufPool[frame_no]);
    if (stat1 != 0)
    {
      return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
    }

    bufDescr[frame_no].dirtybit= false;

  }
  return OK;


}
    
	  
//*************************************************************
//** This is the implementation of ~BufMgr
//************************************************************
BufMgr::~BufMgr()
{
  // put your code her

  for (int i=0; i<numBuf;i++)                                 //Loop through all the frames in buffer pool
  {
    if(bufDescr[i].dirtybit==TRUE)                           //If dirty bit is set to TRUE, then flush them
    {
      Status stat1 = MINIBASE_DB->write_page(bufDescr[i].pageNo, &bufPool[i]);
      if(stat1==0)
      {
        bufDescr[i].dirtybit=false;
      }
    }
  }

  free(bufDescr);                     //De-allocating memory for bufferDescriptor
  free(bufPool);                     // De-allocating memory for bufferPool
  //Hash table objects and the replacement queue objects would be deallocated automatically at the end of program run

}


//*************************************************************
//** This is the implementation of unpinPage
//************************************************************

Status BufMgr::unpinPage(PageId page_num, int dirty=FALSE, int hate = FALSE) {
  // put your code here


  int frame_no = htab.search(page_num);
  if (frame_no == -1)
    return FAIL;                          //The page to be unpinned is not present in any of the frames. So return FAIL
    //return MINIBASE_CHAIN_ERROR(BUFMGR,PAGE_NOT_FOUND);//Page does not exist in hash table

  if (bufDescr[frame_no].pin_count == 0)
    return FAIL;                       //Must be error. If the page to be unpinned already has a picCount==0 ,return ERROR
    //return MINIBASE_CHAIN_ERROR(BUFMGR,PAGE_NOT_PINNED);//Page to be unpinned is already unpinned.


  else if (bufDescr[frame_no].pin_count > 0)
  {

    if (hate == FALSE)
    {
      bufDescr[frame_no].hateFlag = false;                //Set the hateflag based on the "hate" variable value
    }

    if (dirty == true)
    {
      bufDescr[frame_no].dirtybit = true;               // Set the dirtybit based on the "dirty" variable value
    }

    if (dirty == true)            //If the frame has its dirty bit set, write the contents of the frame to memory
    {
      Status stat1 = MINIBASE_DB->write_page(bufDescr[frame_no].pageNo, &bufPool[frame_no]);
      if (stat1 != 0)
      {
        return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
      }
      bufDescr[frame_no].dirtybit=false;
    }

      bufDescr[frame_no].pin_count = bufDescr[frame_no].pin_count - 1; //Decrement the pin count

    //If the pin count after decrement is not yet zero, then do nothing. Just keep track if it is loved or hated
    if (bufDescr[frame_no].pin_count != 0)
    {
      return OK;
    }
    else if (bufDescr[frame_no].pin_count == 0)
    {

      if (bufDescr[frame_no].hateFlag == false)               //Push the page to the Love Queue
      {
        rq.push_love_queue(page_num);
        /*std::list<PageId>::iterator it;
        for(it=rq.love_queue.begin(); it!=rq.love_queue.end(); it ++)
        {
          cout <<"Love queue" <<*it <<"\n";
        }*/
      }

      else if (bufDescr[frame_no].hateFlag == true)           //Push the page to the hate queue
      {
        rq.push_hate_queue(page_num);
        //std::list<PageId>::iterator it;
      }

      return OK;
    }                                                      //end-else-if
  }                                                       //end-outer-else-if
}                                                        //end-function


//*************************************************************
//** This is the implementation of freePage
//************************************************************

Status BufMgr::freePage(PageId globalPageId)
{

  /* Search for the page in  hash table */

  int frame_no=htab.search(globalPageId);
  if (frame_no == -1)                                    //Page not found in the hash table
  {
    return FAIL;
    //return MINIBASE_CHAIN_ERROR(BUFMGR,PAGE_NOT_FOUND);  //Page-not-found error
  }

  if(bufDescr[frame_no].pin_count==0)                   //Indicates the page is present in the hash table
  {

    htab.delete_entry(globalPageId);                   //Delete the corresponding entry from hash table

    if(bufDescr[frame_no].dirtybit==TRUE)              // Indicates dirty page
    {
      Status stat1 = MINIBASE_DB->write_page(bufDescr[frame_no].pageNo, &bufPool[frame_no]);
      if (stat1 != 0)
      {
        return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
      }

    }

    Status stat2= MINIBASE_DB->deallocate_page(globalPageId,1); //Run-size in this case would be 1 since we are going to delete one page
    /*if (stat2 != 0)
    {
      return MINIBASE_CHAIN_ERROR(BUFMGR, stat2);
    }*/


    bufCnt=bufCnt-1;                                          //Decrementing the bufCnt to indicate a slot is empty
    bufDescr[frame_no].pageNo=EMPTY_FRAME;                  //Setting the buffer descriptor properties to the initial value
    bufDescr[frame_no].dirtybit=false;
    bufDescr[frame_no].pin_count=0;
    bufDescr[frame_no].hateFlag=true;

    return OK;
  }

  return FAIL;
  //return MINIBASE_CHAIN_ERROR(BUFMGR,PAGE_PINNED);       //Page is pinned. Cannot free a pinned page

} //end-function

Status BufMgr::flushAllPages()
{

  for (int i=0; i<numBuf;i++)                                 //Loop through all the frames in buffer pool
  {
    if(bufDescr[i].dirtybit==TRUE)                           //If dirty bit is set to TRUE, then flush them
    {
      Status stat1 = MINIBASE_DB->write_page(bufDescr[i].pageNo, &bufPool[i]);
      if (stat1 != 0)
      {
        //return FAIL;
        return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
      }
      bufDescr[i].dirtybit= false;
    }

  }
  return OK;

}
