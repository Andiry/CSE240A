/*
 *
 * File: prefetcher.C
 * Author: Jian Xu <jix024@eng.ucsd.edu> PID: A53026658
 * Description: This is a diffeential Markov prefetcher which submit 3 requests
 * per L1 ache miss.
 *
 */

#include "prefetcher.h"
#include <stdio.h>

Prefetcher::Prefetcher() {
	int i;

	_ready = false;
	_nextReq.addr = 0;
	_enqueue = 0;
	_prev_addr = 0;
	_next_diff = 0;
	_count = 0;
	_items = (struct item *)malloc(sizeof(struct item) * MAX_CAPACITY);
	if (_items) {
		for (i = 0; i < MAX_CAPACITY; i++) {
			_items[i].diff = 0;
			_items[i].prev = -1;
		}
	} else {
		printf("ERROR: items initialization failed");
	}
}

Prefetcher::~Prefetcher() {
	free(_items);
}

bool Prefetcher::hasRequest(u_int32_t cycle)
{
	_count++;
	if (_count > 3)
		_ready = false;
	else
		_ready = true;

	return _ready;
}

Request Prefetcher::getRequest(u_int32_t cycle)
{
	int next_diff;

	if (_count == 1)
		next_diff = _next_diff;
	else if (_count == 2)
		next_diff = _next_diff - 32;
	else
		next_diff = _next_diff + 32;

	_nextReq.addr = _prev_addr + next_diff;
	return _nextReq;
}

void Prefetcher::completeRequest(u_int32_t cycle)
{
	return;
}

void Prefetcher::update_enq(void)
{
	_enqueue++;
	if (_enqueue == MAX_CAPACITY)
		_enqueue = 0;
}

/* find the previos entry in the global distance table with the same distance */
int Prefetcher::find_prev(int diff)
{
	int cur_enq = _enqueue;
	int i;
	bool found = false;

	for (i = 0; i < MAX_CAPACITY - 1; i++) {
		if (cur_enq == 0)
			cur_enq = MAX_CAPACITY - 1;
		else
			cur_enq--;

		if (_items[cur_enq].diff == diff) {
			found = true;
			break;
		}
	}

	if (found)
		return cur_enq;
	else
		return -1;
}

/*
 * Search the global table, with the current distance, find the next possible
 * distance based on global history.
 * We will add the most possible next distance to current request address,
 * formatting the next request address.
 */
int Prefetcher::find_next_req(int diff)
{
	int cur_enq = _enqueue;
	int candidate;
	int i;
	bool found = false;
	struct item array[10];
	int num = 0;
	int flip = 0;
	int max = 0;
	int index = 0;
	int next_pos = 0;
	int temp_diff = 0;
	bool first_cycle = true;

	/* Initialize the candidates array with 10 entrys */
	for (i = 0; i < 10; i++) {
		array[i].diff = 0;
		array[i].prev = 0; //use prev field as counter
	}

	while(1) {
		if (!first_cycle) {
			candidate = cur_enq + 1;
			if (candidate == MAX_CAPACITY)
				candidate = 0;

			/* Insert the addr of candidate entry to candidates array */
			temp_diff = _items[candidate].diff;
			if (temp_diff) {
				for (i = 0; i < num; i++) {
					if (array[i].diff == temp_diff) {
						array[i].prev++;
						break;
					}
				}

				if (i == num && num < 10) { //add new candidates
					array[num].diff = temp_diff;
					array[num].prev++;
					num++;
				}
			}
		} else {
			first_cycle = false;
		}

		next_pos = _items[cur_enq].prev;
		if (next_pos == -1)
			break;
		if (next_pos >= cur_enq && flip)
			break;
		if (next_pos >= cur_enq)
			flip = 1;
		if (_items[next_pos].diff != diff)
			break;

//		printf("diff %d, enqueue %d, next_pos %d, cur_enq %d\n", diff, _enqueue, next_pos, cur_enq);
		cur_enq = next_pos;
	}

	for (i = 0; i < num; i++) {
//		printf(" candidate %d, 0x%x, %d\n", i, array[i].addr, array[i].prev);
		if (array[i].prev > max) {
			max = array[i].prev;
			index = i;
		}
	}

	if (max && array[index].diff)
		return array[index].diff;
	else
		return 0;
}

void Prefetcher::cpuRequest(Request req)
{
	int diff;
	int next_diff;

	if(!_ready && !req.HitL1) {
		diff = req.addr - _prev_addr;

		_items[_enqueue].diff = diff;
		_items[_enqueue].prev = find_prev(diff);
//		printf("enqueue 0x%x, %d at %d\n", req.addr, _items[_enqueue].prev, _enqueue);
		update_enq();
		next_diff = find_next_req(diff);

		/* align next_diff to 64 */
		if (next_diff >= 0 && next_diff < 64) {
//			printf("find candidate diff %d for 0x%x\n", next_diff, req.addr);
			next_diff = 64;
		} else if (next_diff < 0 && next_diff > -64){
			next_diff = -64;
		}

		_next_diff = next_diff;
		_ready = true;
		_prev_addr = req.addr;
		_count = 0;
	}
}
