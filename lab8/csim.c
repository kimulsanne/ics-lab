#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

typedef struct
{
	long long addr;
	int valid;
	int order;
}line;

int hit = 0;
int miss = 0;
int eviction = 0;
int s, E, b;

void load(line* cache, char ins, long long addr)
{
	addr = addr >> b;
	long long set = addr & ((1 << s) -1);
	addr = addr >> s;
	int i;
	int j = 0;

	if (ins == 'M')
		hit++;
	//hit
	for (i = E * set; i < E*set+E; i++)
	{
		if (cache[i].addr == addr && cache[i].valid == 1)
		{
			hit++;
			for (j = E*set; j < E*set+E; j++)
			{
				if (cache[j].order > cache[i].order)
					cache[j].order--;
			}
			cache[i].order = 3;
			return;
		}
	}

	//miss
	miss++;
	for (i = E * set; i < E*set+E; i++)
	{
		if (cache[i].valid ==0)
		{
			for (j = E*set; j < E*set+E; j++)
				if (cache[j].order != 0)
					cache[j].order--;	
			cache[i].addr = addr;
			cache[i].order = 3;
			cache[i].valid = 1;		
			return;
		}
	}

	//eviction
	eviction++;
	for (i = E * set; i < E*set+E; i++)
	{
		if (cache[i].order == 4 - E)
		{
			cache[i].addr = addr;
			cache[i].order = 3;
			cache[i].valid = 1;
		}
		else 
			cache[i].order--;
	}			
}

int main(int argc, char **argv)
{
	int i = 0;
	int size;
	long long addr;
	char ins;
	
	char* name;
	char ch;
	
	while((ch = getopt(argc,argv,"s:E:b:t:"))!= -1)
	{
		switch (ch)
		{
			case 's':
	    			s = atoi(optarg);
	    			break;
			case 'E':
	    			E = atoi(optarg);
	    			break;
			case 'b':
	   			b = atoi(optarg);
	    			break;
			case 't':
	    			name = optarg;
	    			break;
			default:
	    			break;
		}
    	}
	
	line *cache = NULL;
	cache = (line *) malloc ((1<<s) * E * sizeof(line));
	for (i = 0; i < (1<<s)*E; i++)
	{
		cache[i].addr = 0;	
		cache[i].valid = 0;
		cache[i].order = 0;
	}

	FILE *fin = fopen(name, "r");
	while (!feof(fin))
	{
		fscanf(fin, " %c %llx,%d", &ins, &addr, &size);
		if (feof(fin)) 
			break;
		
		if (ins != 'I')
			load(cache, ins, addr);
	}
	
 
	fclose(fin);
	printSummary(hit, miss, eviction);
	free(cache);
	return 0;
}
