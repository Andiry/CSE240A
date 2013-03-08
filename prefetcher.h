/*
 *
 * File: prefetcher.h
 * Author: Jian Xu <jix024@eng.ucsd.edu> PID: A53026658
 * Description: This is a diffeential Markov prefetcher which submit 3 requests
 * per L1 ache miss.
 *
 */

#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include <malloc.h>
#include <stdint.h>
#include "mem-sim.h"

#define MAX_CAPACITY 512

struct item
{
	int diff;
	int prev;
};

class Prefetcher {
  private:
	bool _ready;
	Request _nextReq;

	/* a 512-entry table, maintain global distance history information */
	struct item *_items;
	int _enqueue;
	int _prev_addr;
	int _next_diff;
	int _count;

  public:
	Prefetcher();
	~Prefetcher();

	// should return true if a request is ready for this cycle
	bool hasRequest(u_int32_t cycle);

	// request a desired address be brought in
	Request getRequest(u_int32_t cycle);

	// this function is called whenever the last prefetcher request was successfully sent to the L2
	void completeRequest(u_int32_t cycle);

	/*
	 * This function is called whenever the CPU references memory.
	 * Note that only the addr, pc, load, issuedAt, and HitL1 should be considered valid data
	 */
	void cpuRequest(Request req);

	/* Update enqueue index of GHB array */
	void update_enq(void); 

	/* Find the previous entry of the same diff */
	int find_prev(int diff);

	/* Find the next candidate entry to prefetch */
	int find_next_req(int diff);
};

#endif
