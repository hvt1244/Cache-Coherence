/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/


#include <stdlib.h>
#include <assert.h>
#include "cache.h"
#include <stdio.h>

using namespace std;

Cache::Cache(int s,int a,int b )
{
	ulong i, j;
	reads = readMisses = writes = 0; 
	writeMisses = writeBacks = currentCycle = 0;
	flushes = invalidations = mem_trans = interventions = c2ctransfers = busrdxes = 0;
	busrd = busrdx=0;
	
	size       = (ulong)(s);
	lineSize   = (ulong)(b);
	assoc      = (ulong)(a);   
	sets       = (ulong)((s/b)/a);
	numLines   = (ulong)(s/b);
	log2Sets   = (ulong)(log2(sets));   
	log2Blk    = (ulong)(log2(b));   

	//*******************//
	//initialize your counters here//
	//*******************//

	tagMask =0;
	for(i=0;i<log2Sets;i++)
	{
		tagMask <<= 1;
		tagMask |= 1;
	}

	/**create a two dimentional cache, sized as cache[sets][assoc]**/ 
	cache = new cacheLine*[sets];
	for(i=0; i<sets; i++)
	{
		cache[i] = new cacheLine[assoc];
		for(j=0; j<assoc; j++) 
		{
			cache[i][j].invalidate();
		}
	}      

}

/* Access with MSI protocol */
void Cache::MSIAccess(int processor, int num_processors, ulong addr, uchar op, int protocol, Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			 among cache ways, updated on every cache access*/

	if(op == 'w') writes++;
	else          reads++;

	cacheLine * line = findLine(addr);
	if(line == NULL || (line && line->getFlags()== INVALID))
	{
		//If other copies exist, just fetch from the memory
		mem_trans++;
		if(op == 'w') writeMisses++;
		else if(op == 'r') readMisses++;

		//Writing new block
		cacheLine *newline = fillLine(addr);
		
		//change the state and give the bus command
		if(op == 'w') 
		{
			newline->setFlags(MODIFIED);    
			if(protocol==0){busrdx=1; busrdxes++;};
		}
		if(op == 'r') 
		{
			newline->setFlags(SHARED);
			
			if(line && line->getFlags()== INVALID)
				interventions++;
		busrd = 1;
		}
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		//only shared to modified will have BUSRDX and memory_transactions
		if(line->getFlags() == SHARED && op =='w') 
		{
			/*Send BusRdX */
			busrdx=1;
			line->setFlags(MODIFIED);
			mem_trans++;
			busrdxes++;
		}
	}
	//return NO_OP;
	for(int i=0; i<num_processors;i++){
		if(i!=processor){
			cache[i]->MSIBusRd(busrd,addr);
			cache[i]->BusRdX(busrdx,addr);
			//there wil not be any bus update or upgrade for MSI
		}
	}
}


void Cache::MESIAccess(int processor, int num_processors, ulong addr, uchar op, int protocol, Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			 among cache ways, updated on every cache access*/
	
	busrd = busrdx = busupgr = busupd = 0;

	if(op == 'w') writes++;
	else          reads++;
	

	cacheLine * line = findLine(addr);
	if(line == NULL || (line && line->getFlags()== INVALID))
	{
		/* Block not found!! Start a memory access*/
		if(op == 'w') writeMisses++;
		else if(op == 'r') readMisses++;

		if(copyexist) c2ctransfers++;
		else mem_trans++;
		//Overwrite new block
		cacheLine *newline = fillLine(addr);
		
		//change state and give bus commands
		if(op == 'w')
		{
			newline->setFlags(MODIFIED);
			busrdx = 1;
			busrdxes++;
		}
		if(op == 'r') 
		{
			busrd=1;
			if(copyexist == true)
			{
				/* Wait!! Found it. Cancel that expensive memory access */
				newline->setFlags(SHARED);    
				if(line && line->getFlags()== INVALID)
					interventions++;
				/* Somebody is going to flush the cache. */	
				c2ctransfers++;
			}
			else
			{
				newline->setFlags(EXCLUSIVE);
				mem_trans++;
			}
		}

	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(line->getFlags() == SHARED && op =='w') 
		{
			busupgr=1;
			line->setFlags(MODIFIED);
		}
		if(line->getFlags() == EXCLUSIVE && op =='w') 
		{
			line->setFlags(MODIFIED);
		}
	}
	//Other caches looking at the bus commands
	for(int i=0; i<num_processors; i++){
		if(i!=processor){
			cache[i]->MESIBusRd(busrd,addr);
			cache[i]->BusRdX(busrdx,addr);
			cache[i]->BusUpgrade(busupgr,addr);
		}
	}
}

void Cache::DragonAccess(int processor, int num_processors, ulong addr, uchar op, int protocol, Cache **cache)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			 among cache ways, updated on every cache access*/

	busrd = busrdx = busupgr = busupd = 0;

	if(op == 'w') writes++;
	else          reads++;

	cacheLine * line = findLine(addr);
	if(line == NULL )
	{
		/* Block not found!! Start a memory access*/
		if(op == 'w') writeMisses++;
		else if(op == 'r') readMisses++;
		mem_trans++;
//Overwrite the new block
		cacheLine *newline = fillLine(addr);
		if(op == 'w')
		{
			busrd=1;
			newline->setFlags(MODIFIED);
			if(copyexist == true)
			{
				newline->setFlags(SHARED_MODIFIED);
				busupd=1;
			}
			else
			{
				newline->setFlags(MODIFIED);
			}
		}
		if(op == 'r') 
		{
			if(copyexist == true)
			{
				newline->setFlags(SHARED_CLEAN); 
				busrd = 1;
			}
			else
			{
				newline->setFlags(EXCLUSIVE);
				busrd = 1;
			}
		}

	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(line->getFlags() == SHARED_CLEAN && op =='w') 
		{
			if(copyexist == true)
				line->setFlags(SHARED_MODIFIED);
			else
				line->setFlags(MODIFIED);
			busupd = 1;
		}
		if(line->getFlags() == SHARED_MODIFIED && op =='w') 
		{	
			if (copyexist == false)
				line->setFlags(MODIFIED);
			busupd=1;
		}
		if(line->getFlags() == EXCLUSIVE && op =='w') 
		{
			line->setFlags(MODIFIED);
		}
	}
	
	for (int i = 0; i < num_processors; i++)
	{
		if (i != processor)
		{
		cache[i]->DragonBusRd(busrd, addr);
		cache[i]->BusUpdate(busupd, addr);
		}
	}
}
//Bus operations
void Cache::MSIBusRd(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line != NULL)
	{               
		if (line->getFlags() == MODIFIED)
		{
			writeBacks++;
			mem_trans++;
			flushes++;
			line->setFlags(SHARED);
			interventions++;
		}
	}
}
}
void Cache::MESIBusRd(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line != NULL)
	{               
		if (line->getFlags() == MODIFIED)
		{
			writeBacks++;
			mem_trans++;
			flushes++;
			line->setFlags(SHARED);
			interventions++;
			return;
		}
		if (line->getFlags() == EXCLUSIVE)
		{
			line->setFlags(SHARED);
			interventions++;
		}
	}
}
}
void Cache::DragonBusRd(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line != NULL)
	{               
		if (line->getFlags() == MODIFIED)
		{
			line->setFlags(SHARED_MODIFIED);
		//	flushes++;
		//	writeBacks++;
		//	mem_trans++;
		//why ????????
			interventions++;
		}
		if (line->getFlags() == EXCLUSIVE)
		{
			line->setFlags(SHARED_CLEAN);
			interventions++;
		}

		if (line->getFlags() == SHARED_MODIFIED)
		{
			flushes++;//even no state transaction, still add flushes counter.
		//	writeBacks++;
		//	mem_trans++;
		//	return;
		}
	}
}
}
void Cache::BusRdX(bool a, ulong addr)
{
if(a){
	cacheLine * line = findLine(addr);
	if (line)
	{
		if (line->getFlags() == MODIFIED)
		{
			writeBacks++;
			mem_trans++;
			flushes++;
		}
		line->invalidate();
		invalidations++;
	}
}
}
void Cache::BusUpgrade(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line)
	{
		if (line->getFlags() == SHARED)
		{
			line->invalidate();
			invalidations++;
		}
	}
}
}

void Cache::BusUpdate(bool a, ulong addr)
{
if(a){
	cacheLine *line = findLine(addr);
	if (line)
	{
		if (line->getFlags() == SHARED_MODIFIED)
			line->setFlags(SHARED_CLEAN);
	}
}
}


/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
	ulong i, j, tag, pos;

	pos = assoc;
	tag = calcTag(addr);
	i   = calcIndex(addr);

	for(j=0; j<assoc; j++)
		if(cache[i][j].isValid())
			if(cache[i][j].getTag() == tag)
			{
				pos = j; break; 
			}
	if(pos == assoc)
		return NULL;
	else
		return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
	line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
	ulong i, j, victim, min;

	victim = assoc;
	min    = currentCycle;
	i      = calcIndex(addr);

	for(j=0;j<assoc;j++)
	{
		if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
	}   
	for(j=0;j<assoc;j++)
	{
		if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
	} 
	assert(victim != assoc);

	return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
	cacheLine * victim = getLRU(addr);
	updateLRU(victim);

	return (victim);
}



/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
	ulong tag;

	cacheLine *victim = findLineToReplace(addr);
	assert(victim != 0);
	//if(victim->getFlags() == MODIFIED || victim->getFlags()==SHARED_MODIFIED)

	if(victim->getFlags() == MODIFIED)
		writeBack(addr);

	tag = calcTag(addr);   
	victim->setTag(tag);
	victim->setFlags(VALID);    
	/**note that this cache line has been already 
	  upgraded to MRU in the previous function (findLineToReplace)**/

	return victim;
}

void Cache::printStats()
{ 
	printf("============ Simulation results ============\n");

/*

	ulong memory_transactions = memory;
	ulong num_writebacks = writeBacks+flushes;
	if (protocol == 0)
	{
		num_writebacks = writeBacks;
		memory_transactions = writeBacks + memory;
	}
	if (protocol == 1)
	{
		num_writebacks = writeBacks;
		memory_transactions = writeBacks + memory;
	}
	if (protocol == 2)
		memory_transactions = memory+writeBacks+flushes;*/
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
	printf("01. number of reads: 				%lu\n", getReads());
	printf("02. number of read misses: 			%lu\n", getRM());
	printf("03. number of writes: 				%lu\n", getWrites());
	printf("04. number of write misses:			%lu\n", getWM());
	printf("05. total miss rate: 				%.2f%%\n", (double)(getRM() + getWM()) * 100 / (getReads() + getWrites()));
	printf("06. number of writebacks: 			%lu\n", getWB());
	printf("07. number of cache-to-cache transfers: 	%lu\n", getC2C());
	printf("08. number of memory transactions: 		%lu\n", mem_trans);
	printf("09. number of interventions: 			%lu\n", getInterventions());
	printf("10. number of invalidations: 			%lu\n", getInvalidations());
	printf("11. number of flushes: 				%lu\n", getFlushes());
	printf("12. number of BusRdX: 				%lu\n", getBusRdX());

}
