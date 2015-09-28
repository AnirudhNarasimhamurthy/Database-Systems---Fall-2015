///////////////////////////////////////////////////////////////////////////////
/////////////  The Header File for the Buffer Manager /////////////////////////
///////////////////////////////////////////////////////////////////////////////


#ifndef BUF_H
#define BUF_H

#include "../include/db.h"
#include "../include/minirel.h"
#include "../include/page.h"
#include <list>

#define NUMBUF 20   
// Default number of frames, artifically small number for ease of debugging.

#define HTSIZE 7
// Hash Table size
//You should define the necessary classes and data structures for the hash table, 
// and the queues for LSR, MRU, etc.

struct HashTable
{

    struct HashMap
    {
        int frame_no;
        PageId page_no;
        HashMap *next;

    };

    HashMap* directory[HTSIZE];
    int a;
    int b;


    void init()
    {
        //directory=(HashMap*)malloc(HTSIZE * sizeof(HashMap));
        for (int i=0;i<HTSIZE;i++)
        {
            directory[i]=NULL;
        }
        a=5;
        b=13;

    }


    int hash_value(PageId pageNo)
    {
        return (a*pageNo +b)% HTSIZE;
    }


    HashMap* createNode(PageId pageNo, int frameNo)
    {
        struct HashMap *temp =new HashMap();
            temp->page_no = pageNo;
            temp->frame_no=frameNo;
            temp->next = NULL;
            return temp;
    }

    void insert(PageId page,int frame)
    {

        /*Using the hash function (a *value + b)% HTSIZE */

        //Finding the hash value for a given page number

        int directory_index= (a*page +b)% HTSIZE;


        struct HashMap *node, *start, *temp;

        start=directory[directory_index];

        node=createNode(page,frame);


        if(start==NULL)
        {
            directory[directory_index]=node;
            //start=node;
            //start->next=NULL;
        }
        else
        {
            temp=start;
            directory[directory_index]=node;
            directory[directory_index]->next=temp;

        }

        /*else
        {
            while (start->next != NULL)
            {
                start=start->next;
            }
            node->next=NULL;
            start->next=node;

        }*/

    }

    void delete_entry(PageId page)
    {

        //Using the hash function (a *value + b)% HTSIZE

        int directory_index= (a*page +b)% HTSIZE;
        struct HashMap *start;
        struct HashMap *temp =NULL; //*pre=NULL;
        start=directory[directory_index];
        if(start->page_no==page) //If the element to be deleted is the first element
        {
            temp=start;
            directory[directory_index]=temp->next;
            delete temp;
            return;
        }

        //pre=start;
        temp=start->next;

        while(temp != NULL) {
            if (temp->page_no == page) //If the element to be deleted is in the middle
            {
                //pre->next=temp->next;
                start->next = temp->next;
               delete temp;
                return;
            }

            //pre=temp;
            start = start->next;
            temp = temp->next;
        }


    }

    int search(PageId page)
    {

        /*Using the hash function (a *value + b)% HTSIZE  and Finding the hash value for a given page number*/
        int directory_index= (a*page +b)% HTSIZE;

        struct HashMap *start;
        start=directory[directory_index];

        if(start==NULL)
        {
            return -1;
        }

        else
        {
            while(start != NULL)
            {

                if(start->page_no==page)
                {
                    return start->frame_no;
                }
                start=start->next;
            }

            return -1; //If not found
        }

    }

} ;



struct replacementQueue
{


    std::list<PageId> love_queue;  //  Love Queue to maintain a list of pages which was marked loved.
    std::list<PageId> hate_queue;  // Hate Queue to maintain a list of pages which was marked hated */
    std::list<PageId>::iterator it;

    void push_love_queue(int pageNo)
    {
        love_queue.push_back(pageNo);

    }

   int pop_love_queue() //Returns the frame no that was popped so that it could be used to determine a free frame to pin a new page
    {
        if (!love_queue.empty())
        {
            it=love_queue.begin();
            int value= *it;
            love_queue.pop_front();
            return value;
        }

        else
        {
            return -1;

        }

    }

    void push_hate_queue(int pageNo)
    {
        hate_queue.push_front(pageNo);

    }

    int pop_hate_queue()
    {
        if (!hate_queue.empty())
        {
            it=hate_queue.begin();
            int value= *it;
            hate_queue.pop_front();
            return value;
        }

        else
        {
            return -1;
        }

    }

    /*int search_love_queue(int pageNo)
    {
        if (!love_queue.empty())
        {
            for (it=love_queue.begin(); it != love_queue.end() ;it ++)
                if(*it == pageNo)
                {
                    return 1; //Frame already present in love queue
                }
                else
                {
                    return -1;
                }
        }
    }

    int search_hate_queue(int pageNo)
    {
        if (!hate_queue.empty())
        {
            for (it=hate_queue.begin(); it != hate_queue.end() ;it ++)
                if(*it == pageNo)
                {
                    return 1; //Frame already present in love queue
                }
                else
                {
                    return -1;
                }
        }
    }*/

    void remove_from_hate_queue(int pageNo)
    {
        if (!hate_queue.empty())
        {
                 hate_queue.remove(pageNo); //Frame already present in love queue
        }
        else
        {
                    return ;
        }
    }

    void remove_from_love_queue(int pageNo)
    {
        if (!love_queue.empty())
        {
            love_queue.remove(pageNo); //Frame already present in love queue
        }
        else
        {
            return ;
        }
    }


};




/*******************ALL BELOW are purely local to buffer Manager********/

// You should create enums for internal errors in the buffer manager.
enum bufErrCodes  {


    };

class Replacer;

class BufMgr {

private:
    // fill in this area

    struct BufDescr
    {

        PageId pageNo;
        unsigned int pin_count;
        bool dirtybit;
        bool hateFlag; //To keep track if a page needs to stay in the loved list or hate list

    } *bufDescr ;

    int bufCnt; //Maintains a track of the number of frames filled in the buffer pool

    int numBuf;

    HashTable htab; //Object for the hastable structure

    replacementQueue rq; //Object for the replacementQueue


public:

    Page* bufPool; // The actual buffer pool

    BufMgr (int numbuf, Replacer *replacer = 0); 
    // Initializes a buffer manager managing "numbuf" buffers.
	// Disregard the "replacer" parameter for now. In the full 
  	// implementation of minibase, it is a pointer to an object
	// representing one of several buffer pool replacement schemes.

    ~BufMgr();           // Flush all valid dirty pages to disk

    Status pinPage(PageId PageId_in_a_DB, Page*& page, int emptyPage=0);
        // Check if this page is in buffer pool, otherwise
        // find a frame for this page, read in and pin it.
        // also write out the old page if it's dirty before reading
        // if emptyPage==TRUE, then actually no read is done to bring
        // the page

    Status unpinPage(PageId globalPageId_in_a_DB, int dirty, int hate);
        // hate should be TRUE if the page is hated and FALSE otherwise
        // if pincount>0, decrement it and if it becomes zero,
        // put it in a group of replacement candidates.
        // if pincount=0 before this call, return error.

    Status newPage(PageId& firstPageId, Page*& firstpage, int howmany=1); 
        // call DB object to allocate a run of new pages and 
        // find a frame in the buffer pool for the first page
        // and pin it. If buffer is full, ask DB to deallocate 
        // all these pages and return error

    Status freePage(PageId globalPageId); 
        // user should call this method if it needs to delete a page
        // this routine will call DB to deallocate the page 

    Status flushPage(PageId pageid);
        // Used to flush a particular page of the buffer pool to disk
        // Should call the write_page method of the DB class

    Status flushAllPages();
	// Flush all pages of the buffer pool to disk, as per flushPage.

    /* DO NOT REMOVE THIS METHOD */    
    Status unpinPage(PageId globalPageId_in_a_DB, int dirty=FALSE)
        //for backward compatibility with the libraries
    {
      return unpinPage(globalPageId_in_a_DB, dirty, FALSE);
    }

};

#endif
