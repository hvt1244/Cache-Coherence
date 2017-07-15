/*******************************************************
                          main.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <sstream>

using namespace std;

#include "cache.h"

//#define NUM_PROCESSORS 4



int main(int argc, char *argv[])
{
	
	ifstream fin;
	FILE * pFile;
	string line;
	int i=0;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

	
	/* Print name and unity ID*/
	printf("===== 506 Personal information =====\n");
	printf("Hridayesh Thakur\n");	
	//****************************************************//
	//**printf("===== Simulator configuration =====\n");**//
	//*******print out simulator configuration here*******//
	//****************************************************//
	printf("===== 506 SMP Simulator configuration =====\n");
	printf("L1_SIZE:		%d\n",cache_size);
	printf("L1_ASSOC:		%d\n",cache_assoc);
	printf("L1_BLOCKSIZE:		%d\n",blk_size);
	printf("NUMBER OF PROCESSORS:	%d\n",num_processors);
	if(protocol == 0)
		printf("COHERENCE PROTOCOL:	MSI\n");
	else if(protocol == 1)
		printf("COHERENCE PROTOCOL:	MESI\n");
	else if(protocol == 2)
		printf("COHERENCE PROTOCOL:	Dragon\n");
	printf("TRACE FILE:		%s\n",fname);

 
	//*********************************************//
        //*****create an array of caches here**********//
	//*********************************************//	
	Cache *cache[num_processors];
	for (int i=0;i<num_processors; i++)
	{
		cache[i] = new Cache(cache_size, cache_assoc, blk_size);
	}
	//Cache cachesArray[4]={Cache(cache_size,cache_assoc,blk_size),Cache(cache_size,cache_assoc,blk_size),Cache(cache_size,cache_assoc,blk_size),Cache(cache_size,cache_assoc,blk_size)};

	pFile = fopen (fname,"r");
	if(pFile == 0)
	{   
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//
	int curr_processor_num;
	char operation;
	int address;
			string STRINGS;
		ifstream infile(fname);
	while (getline(infile,STRINGS))
	{


	//	getline(infile,STRINGS); // Saves the line in STRING.
		//if (pFile.eof()) break;

		//cout<<STRINGS<<endl; // Prints our STRING.
		istringstream iss;
		iss.str (STRINGS);
		iss >> curr_processor_num >> operation >> address;
				cache[curr_processor_num]->copyexist = 0;
		//bool copy_exists = 0;

		for (int i=0; i<num_processors; i++){
			if ((cache[i]->findLine(address))&& (i!=curr_processor_num))//implies that other cache has the same block
				cache[curr_processor_num]->copyexist=1;
			}
			
		if (protocol==0)
			cache[curr_processor_num]->MSIAccess(curr_processor_num,num_processors, address,operation, protocol, cache);
		if (protocol==1)
			cache[curr_processor_num]->MESIAccess(curr_processor_num,num_processors, address,operation, protocol, cache);
		if (protocol==0)
			cache[curr_processor_num]->DragonAccess(curr_processor_num,num_processors, address,operation, protocol, cache);

		}
		
	
/*		
		if (protocol == 0)
		{
			if((busop = cachesArray[proc_num].MSIAccess(addr,cmd)))
			{
				for(i=0;i<num_processors ;i++)
				{
					if(i!=proc_num && FLUSH == cachesArray[i].MSIBus(addr,busop))
					{
						cachesArray[proc_num].MSIFlush(addr,busop);
					}
						
				}
			}
		}
		else if(protocol == 1)
		{
			for(i=0;i<num_processors ;i++)
			{
				copy_exists = MESICheckCopies(addr,&cachesArray);
			}
			if((busop = cachesArray[proc_num].MESIAccess(addr,cmd,copy_exists)))
			{
				for(i=0;i<num_processors;i++)
				{
					if(i != proc_num )
					{
						cachesArray[i].MESIBus(addr,busop);
					}
				}
			}
		}
		else if(protocol == 2)
		{
			copy_exists = DragonCheckCopies(addr,&cachesArray);
			if((busop = cachesArray[proc_num].DragonAccess(addr,cmd,copy_exists)))
			{
				for(i=0;i<num_processors;i++)
				{
					if(i != proc_num )
					{
						if(busop == BUSRD_BUSUPD)
						{
							cachesArray[i].DragonBus(addr,BUSRD);
							cachesArray[i].DragonBus(addr,BUSUPD);
						}
						else
							cachesArray[i].DragonBus(addr,busop);
					}
				}
			}
		}
		copy_exists = false;
	}
	*/
	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//
	for(i=0;i<num_processors ;i++){
		printf("------------Cache %d------------\n",i);
		cache[i]->printStats();
	
	}
	return 0;
}
