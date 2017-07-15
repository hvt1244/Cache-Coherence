/*******************************************************
  cache.h
  Ahmad Samih & Yan Solihin
  2009
  {aasamih,solihin}@ece.ncsu.edu
 ********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum{
	INVALID = 0,
	VALID,
	MODIFIED,//dirty
	SHARED,
	EXCLUSIVE,
	SHARED_MODIFIED,
	SHARED_CLEAN
};



class cacheLine 
{
	protected:
		ulong tag;
		ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
		ulong seq; 

	public:
		cacheLine()            { tag = 0; Flags = 0; }
		ulong getTag()         { return tag; }
		ulong getFlags()			{ return Flags;}
		ulong getSeq()         { return seq; }
		void setSeq(ulong Seq)			{ seq = Seq;}
		void setFlags(ulong flags)			{  Flags = flags;}
		void setTag(ulong a)   { tag = a; }
		void invalidate()      { tag = 0; Flags = INVALID; }//useful function
		bool isValid()         { return ((Flags) != INVALID); }
};

class Cache
{
	protected:
		ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
		ulong reads,readMisses,writes,writeMisses,writeBacks;

		//******///
		//add coherence counters here///
		//******///
	ulong flushes,invalidations,mem_trans,interventions,c2ctransfers,busrdxes;


		cacheLine **cache;
		ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
		ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
		ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}

	public:
		ulong currentCycle;  
		// Bus command flags
		int busrd, busrdx, busupgr, busupd;
		bool copyexist;
		//ulong flush_opts;

		Cache(int,int,int);
		~Cache() { delete cache;}
		Cache(const Cache& copy);

		cacheLine *findLineToReplace(ulong addr);
		cacheLine *fillLine(ulong addr);
		cacheLine *DragonfillLine(ulong addr);
		cacheLine * findLine(ulong addr);
		cacheLine * getLRU(ulong);

		ulong getRM(){return readMisses;} ulong getWM(){return writeMisses;} 
		ulong getReads(){return reads;}ulong getWrites(){return writes;}
		ulong getWB(){return writeBacks;}
	   ulong getC2C(){ return c2ctransfers; }
	   ulong getInterventions(){ return interventions; }
	   ulong getInvalidations(){ return invalidations; }
	   ulong getFlushes(){ return flushes; }
	   ulong getBusRdX(){ return busrdxes; }
	   
		void writeBack(ulong)   {writeBacks++;}
		void MSIAccess(int, int, ulong, uchar, int, Cache **);
		void MESIAccess(int, int, ulong, uchar, int, Cache **);
		void DragonAccess(int, int, ulong, uchar, int, Cache **);
		void printStats();
		void updateLRU(cacheLine *);

		//******///
		//add other functions to handle bus transactions///
		//******///
   void BusRdX(bool, ulong);
   void BusUpgrade(bool, ulong);
   void BusUpdate(bool, ulong);
   void MSIBusRd(bool, ulong);
   void MESIBusRd(bool, ulong);
   void DragonBusRd(bool, ulong);

};

#endif
