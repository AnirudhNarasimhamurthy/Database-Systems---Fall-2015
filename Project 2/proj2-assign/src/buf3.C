/*****************************************************************************/
/*************** Implementation of the Buffer Manager Layer ******************/
/*****************************************************************************/


#include "buf.h"
#include<iostream>
#include <list>
#define EMPTY_FRAME -1

// Define buffer manager error messages here
//enum bufErrCodes  {...};

// Define error message here
static const char* bufErrMsgs[] = {};

// Create a static "error_string_table" object and register the error messages
// with minibase system 
static error_string_table bufTable(BUFMGR,bufErrMsgs);


//BufMgr buf(NUMBUF);

BufMgr::BufMgr (int numbuf, Replacer *replacer) {

  /* Allocating memory for buffer pool and buffer descriptor and setting the initial pageNo to empty to indicate the corresponding
   * frame is available in the buffer pool to be used */

  //bufPool[numbuf];


  numBuf=numbuf;
  //cout <<"Numbuf is :" << numBuf <<"\n";
  bufPool=(Page*)malloc(sizeof(Page) * numbuf);
  //bufPool[numbuf];
  bufDescr=(BufDescr*)malloc(sizeof(BufDescr) * numbuf);
  bufCnt=0;
  for (int i=0;i<numbuf;i++)
  {
    bufDescr[i].pageNo=EMPTY_FRAME;
    bufDescr[i].pin_count=0;
    bufDescr[i].dirtybit= false;
    bufDescr[i].hateFlag= true;
  }
  htab.init();
}


Status BufMgr::pinPage(PageId PageId_in_a_DB, Page*& page, int emptyPage) {
  // put your code here

  //cout <<"\Coming here ??";
  //Check in the hashtable if the page is present, if so get its frame no and increment the pin count
  int frame_no = htab.search(PageId_in_a_DB);
  if (frame_no != -1) {
    if (bufDescr[frame_no].pin_count == 0)  //If the page is in Love or Hate Queue, then remove the entry from the Queue
    {
      if (bufDescr[frame_no].hateFlag == true) {
        rq.remove_from_hate_queue(PageId_in_a_DB);
      }
      else {
        rq.remove_from_love_queue(PageId_in_a_DB);
      }

      bufDescr[frame_no].pin_count += 1;
      return OK;
    }


      //Else if page is not present, check if there are any empty frames available in bufPool.Read the page and pin it
    else if (bufCnt < numBuf) {
      // cout <<"\n buCount is :" <<bufCnt;

      for (int i = 0; i < numBuf; i++) {
        if (bufDescr[i].pageNo == EMPTY_FRAME) //Free frame is available
        {

          //Call the Disk Manager to read the page
          Status stat = MINIBASE_DB->read_page(PageId_in_a_DB, &bufPool[i]);
          if (stat != 0) {
            cout << "Error !!" << "\n";
            return MINIBASE_CHAIN_ERROR(BUFMGR, stat);
          }

          //Add the page to the hash table list

          htab.insert(PageId_in_a_DB, i);

          //Update the pageNo,pinCount and dirtyBit values for the frame in bufDescr
          page = &bufPool[i];
          bufDescr[i].pageNo = PageId_in_a_DB;
          bufDescr[i].pin_count = bufDescr[i].pin_count + 1;
          bufDescr[i].dirtybit = FALSE;
          bufCnt = bufCnt + 1;
          return OK;
        }

      }
    }
  }
      //Else if there are no empty frames available, check if any of the existing frames have a frame count of 0
    else {

      //BufCount is full right now and so choose the  page from the hate or love queue for replacement

      int poppedPageHQ = rq.pop_hate_queue();

      if (poppedPageHQ != -1) {

        //Find the corresponding page's frame no and then check for conditions
        int poppedFrameHQ = htab.search(poppedPageHQ);


        if (bufDescr[poppedFrameHQ].dirtybit ==
            true) //If the frame has its dirty bit set, write the contents of the frame to memory
        {
          Status stat1 = MINIBASE_DB->write_page(bufDescr[poppedFrameHQ].pageNo, &bufPool[poppedFrameHQ]);
          if (stat1 != 0) {
            cout << "Error !! " << "\n";
            return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
          }
        }

        /* Read the page/frame  in to the buffer pool. Initialize the picCount to 0 and set
          dirtybit to FALSE */

        Status stat = MINIBASE_DB->read_page(PageId_in_a_DB, &bufPool[poppedFrameHQ]);
        if (stat != 0) {
          cout << "Error !!" << "\n";
          return MINIBASE_CHAIN_ERROR(BUFMGR, stat);
        }
        bufDescr[poppedFrameHQ].pin_count = 0;
        bufDescr[poppedFrameHQ].dirtybit = false;


        /*Delete the entry for the old page from the hash table and Add a new entry
         for the new page no in the hash table */

        htab.delete_entry(poppedPageHQ);
        htab.insert(PageId_in_a_DB, poppedFrameHQ);

        //Update the frame's pin count and pageNo and set the page to its frame location

        page = &bufPool[poppedFrameHQ];
        bufDescr[poppedFrameHQ].pin_count += 1;
        bufDescr[poppedFrameHQ].pageNo = PageId_in_a_DB;
        bufDescr[poppedFrameHQ].hateFlag = true;


        //cout <<"Frame value hate queue:" << poppedFrameHQ;

        return OK;

      }
      else //No frames to remove from Hate Queue.So pop from the Love Queue
      {
        int poppedPageLQ = rq.pop_love_queue();
        //cout <<"\n Popped page from Love queue" << poppedPageLQ;
        if (poppedPageLQ == -1)
          return FAIL;

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
        htab.delete_entry(poppedPageLQ);

        /* Read the page/frame  in to the buffer pool. Initialize the picCount to 0 and set
        dirtybit to FALSE */

        Status stat = MINIBASE_DB->read_page(PageId_in_a_DB, &bufPool[poppedFrameLQ]);
        if (stat != 0) {
          cout << "Error !!" << "\n";
          return MINIBASE_CHAIN_ERROR(BUFMGR, stat);
        }
        bufDescr[poppedFrameLQ].pin_count = 0;
        bufDescr[poppedFrameLQ].dirtybit = false;


        /*Delete the entry for the old page from the hash table and Add a new entry
      for the new page no in the hash table */

        htab.insert(PageId_in_a_DB, poppedFrameLQ);

        //Update the frame's pin count and pageNo
        page = &bufPool[poppedFrameLQ];
        bufDescr[poppedFrameLQ].pin_count += 1;
        bufDescr[poppedFrameLQ].pageNo = PageId_in_a_DB;
        bufDescr[poppedFrameLQ].hateFlag = true;

        //cout <<"Frame value love queue:" << poppedFrameLQ;

        return OK;
      }
  }
}


  //end pinPage //There are no frames available whose pinCount is 0.



Status BufMgr::newPage(PageId& firstPageId, Page*& firstpage, int howmany) {
  // put your code here

    //cout <<"\n Inside newPage" <<"\n";
    cout << "\n runsize:" << howmany;
    /*Status stat = MINIBASE_DB->allocate_page(firstPageId,howmany);
    cout << "here" << "\n";
    if(stat != 0)
    {
      cout << "Error !!" <<"\n";
      return MINIBASE_CHAIN_ERROR(BUFMGR,stat);
    }
  cout << "\n Allocate Status "<<stat <<"\n"; */

  // If a frame exists, call DB object to allocate a run of new pages and and pin it.

    Status stat2=pinPage(firstPageId,firstpage);
    cout << "\n Pin Page Status "<<stat2 <<"\n";

    if(stat2 != 0)
    {
      cout << "Error !!" <<"\n";
      MINIBASE_DB->deallocate_page(firstPageId,howmany);
      return FAIL;
    }
    else
    {
      //frameFound_flag= true;
      return OK;

    }

}

Status BufMgr::flushPage(PageId pageid) {
  // put your code here

  int frame_no=htab.search(pageid);
  cout <<"Page no:" <<pageid << " Frame no:" << frame_no <<"\n";
  cout <<"Inside flush page:" <<"\n";
  cout <<"Dirty bit status :" << bufDescr[frame_no].dirtybit <<"\n";
  if (bufDescr[frame_no].dirtybit==TRUE)
  {
    Status stat1 = MINIBASE_DB->write_page(pageid, &bufPool[frame_no]);
    cout <<"Status is :" << stat1 <<"\n";
    if (stat1 != 0) {
      cout << "Error !! " << "\n";
      return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
    }

    return OK;

  }



}
    
	  
//*************************************************************
//** This is the implementation of ~BufMgr
//************************************************************
BufMgr::~BufMgr(){
  // put your code her
    /*for(int i=0;i < bufCnt;i++)
    {
      if (bufDescr[i].dirtybit== true)
      {
        //Flush dirty pages to disk
      }
    }*/

}


//*************************************************************
//** This is the implementation of unpinPage
//************************************************************

Status BufMgr::unpinPage(PageId page_num, int dirty=FALSE, int hate = FALSE){
  // put your code here

  int frame_no=htab.search(page_num);
  //cout << "\n Page no:" <<page_num << "Frame number:" <<frame_no;
  if(frame_no !=-1)// Frame exists in the buffer pool
  {
    if(bufDescr[frame_no].pin_count==0)
    {
      return FAIL; //Must be error. If the page to be unpinned already has a picCount==0 ,return ERROR
    }
    else if (bufDescr[frame_no].pin_count > 0)
    {
      bufDescr[frame_no].pin_count=bufDescr[frame_no].pin_count-1; //Decrement the pin count
      if(hate==FALSE)
      {
        bufDescr[frame_no].hateFlag= false;
        //cout <<"\n Is hate true ? " << hate;
      }
      if (dirty == true) //If the frame has its dirty bit set, write the contents of the frame to memory
        {
          Status stat1 = MINIBASE_DB->write_page(bufDescr[frame_no].pageNo, &bufPool[frame_no]);
          if (stat1 != 0) {
            cout << "Error !! " << "\n";
            return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
          }
        }
      //If the pin count after decrement is not yet zero, then do nothing. Just keep track if it is loved or hated
      if (bufDescr[frame_no].pin_count != 0 )
      {
        return OK;
      }
      else if (bufDescr[frame_no].pin_count == 0)
      {

        if(bufDescr[frame_no].hateFlag == false)
        {
            //Push the page to the Love Queue
            rq.push_love_queue(page_num);
            std::list<PageId>::iterator it;
            //for (it=rq.love_queue.begin(); it != rq.love_queue.end() ;it ++)
            //cout <<"\n Love queue" << *(it);

        }

        else if(bufDescr[frame_no].hateFlag== true)
        {
          //Push the page to the hate queue

            rq.push_hate_queue(page_num);
            std::list<PageId>::iterator it;
            //for (it=rq.hate_queue.begin(); it != rq.hate_queue.end() ;it ++)
            // cout <<"\n Hate queue" << *(it);
        }
        return OK;
      }//end else-if
    }//end-else-if
  }//end-if
  else
  {
    return FAIL; //The page to be unpinned is not present in any of the frames. So return FAIL
  }

}

//*************************************************************
//** This is the implementation of freePage
//************************************************************

Status BufMgr::freePage(PageId globalPageId){
  // put your code here

  /* Search for the page in  hash table */

  int frame_no=htab.search(globalPageId);
  if(frame_no != -1 && bufDescr[frame_no].pin_count==0) //Indicates the page is present in the hash table
  {
    if(bufDescr[frame_no].dirtybit==TRUE)
    {
      Status stat1 = MINIBASE_DB->write_page(bufDescr[frame_no].pageNo, &bufPool[frame_no]);
      if (stat1 != 0) {
        cout << "Error !! " << "\n";
        return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
      }

    }

    Status stat2=MINIBASE_DB->deallocate_page(globalPageId,1); //Run-size in this case would be 1 since we are going to delete one page
    bufCnt=bufCnt-1; //Decrementing the bufCnt to indicate a slot is empty
    bufDescr[frame_no].pageNo=EMPTY_FRAME;
    bufDescr[frame_no].dirtybit=false;
    bufDescr[frame_no].pin_count=0;
    bufDescr[frame_no].hateFlag=true;

    return OK;
  }
  else
  {
    return FAIL;
  }
}

Status BufMgr::flushAllPages()
{
  //put your code here
  for (int i=0; i<numBuf;i++)
  {
    if(bufDescr[i].dirtybit==TRUE)
    {
      Status stat1 = MINIBASE_DB->write_page(bufDescr[i].pageNo, &bufPool[i]);
      if (stat1 != 0)
      {
        cout << "Error !! " << "\n";
        return MINIBASE_CHAIN_ERROR(BUFMGR, stat1);
      }

    }

  }
  return OK;

}
