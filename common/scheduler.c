/* Copyright 2012 Kjetil S. Matheussen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */



#include <string.h>

#include "nsmtracker.h"
#include "playerclass.h"
#include "threading.h"
#include "OS_Player_proc.h"
#include "OS_visual_input.h"

#include "scheduler_proc.h"


#define QUEUE_SIZE 2048
#define MAX_ARGS 7


typedef struct _event_t{
  int64_t time;
  int64_t seq_time;
  union SuperType args[MAX_ARGS];
  SchedulerCallback callback;
  struct _event_t *next;
} event_t;


static event_t *g_free_events = NULL;

static event_t g_all_events[QUEUE_SIZE] = {{0}}; // stored as a global variable so the gc can reach the data.

static event_t g_event0 = {0};

static event_t *get_free_event(void){
  event_t *ret = g_free_events;
  if (ret==NULL)
    return NULL;
  g_free_events = g_free_events->next;
  ret->next = NULL;
  return ret;
}

static void release_event(event_t *event){
  memset(event,0,sizeof(event_t)); // for the gc
  event->next = g_free_events;
  g_free_events = event;
}


/*****
 *
 * !!!Note!!!
 *
 * scheduler_t is allocated atomically since g_queue can only point to data in g_all_events.
 *
 ****/
struct _scheduler_t {
  int64_t current_time;

  int queue_size;
  
  event_t *queue[QUEUE_SIZE];
};


static event_t *get_first_event(scheduler_t *scheduler){
  return scheduler->queue[1];
}


// SCHEDULER plugs into SEQ, and since they have different timer formats, we need to convert.
//
// The difference is that SCHEDULER doesn't reset timer when starting to play. The SCHEDULER also
// increases time even when not playing.
//
// Since we know the start and end time of the current block for both SCHEDULER and SEQ, the conversion is simple.

#if 0
static int64_t scheduler_to_seq_time(int64_t scheduler_time){
  //int64_t block_length = pc->end_time - pc->start_time;
  //return scale(scheduler_time,g_current_time,g_current_time + block_length, pc->start_time, pc->end_time);
  //
  //... is the same as:
  return ((int64_t)pc->start_time) + (scheduler_time - g_current_time);
}
#endif

// Warning: It's quite complicated to get the same value back as you started with when convert between seq time and scheduler time.
static int64_t seq_to_scheduler_time(struct SeqTrack *seqtrack, scheduler_t *scheduler, int64_t seq_time){

#if 0
  int64_t block_length = pc->end_time - pc->start_time;
  return scale(seq_time,
               pc->start_time, pc->end_time,
               scheduler->current_time, scheduler->current_time + block_length
               );
#else
  //... is the same as:
  return scheduler->current_time + (seq_time - ((int64_t)pc->start_time)); // (optimization)
#endif
}

static bool schedule_event(struct SeqTrack *seqtrack, event_t *event, int64_t seq_time, enum SchedulerPriority priority){
  scheduler_t *scheduler = seqtrack->scheduler;

  if (seq_time < (int64_t)pc->start_time)
    seq_time = (int64_t)pc->start_time;
  
  int64_t time = seq_to_scheduler_time(seqtrack, scheduler, seq_time);

  //args=NULL; // test crashreporter
  
#if 0
  printf("|||||||||| Adding event at seq_time %d, scheduler_time %d. g_current_time: %d, pc->start_time: %d\n",
         (int)seq_time,(int)time,
         (int)g_current_time,(int)pc->start_time);
#endif

  if(scheduler->queue_size > QUEUE_SIZE-2){
    printf("SCHEDULER: queue full. Skipping.\n"); // Can happen if playing very fast. Should perhaps use RT_message instead.
    return false;
  }
  
  // Add priority bit.
  time = time << SCHEDULER_NUM_PRIORITY_BITS;
  time = time + priority;

  
  event->time = time;
  event->seq_time = seq_time;

  scheduler->queue_size++;

  event_t **queue = &scheduler->queue[0];
    
  int i = scheduler->queue_size;
  int new_i = i >> 1;

  while(time <= queue[new_i]->time){ // '<=' (instead of '<') means that the event will be inserted after events with the same time.
    queue[i] = queue[new_i];
    i = new_i;
    new_i = new_i >> 1;
  }
  
  queue[i] = event;

  return true;
}

void SCHEDULER_add_event(struct SeqTrack *seqtrack, int64_t seq_time, SchedulerCallback callback, union SuperType *args, int num_args, enum SchedulerPriority priority){
  R_ASSERT(PLAYER_current_thread_has_lock());

#if !defined(RELEASE)
  if(num_args>MAX_ARGS){
    fprintf(stderr, "Max %d args allowed for scheduler...\n", MAX_ARGS);
    abort();
  }
#endif
  

  // An event created by an RT_process function needs to run right away.
  // If not it won't be run until the next audio block since SCHEDULER_called_per_block
  // has already been called for this audio block.
  // (Q: why not do this for events genereated by the editor too?
  //  A: Because priority would be lost. Effects could run after notes, "note on" events could run before "note off" events, and so forth)
  if (false==pc->is_treating_editor_events && seq_time < pc->end_time) {
    callback(seqtrack, seq_time, args);
    return;
  }

  event_t *event = get_free_event();
  if (event==NULL){
    RT_message("Unable to schedule event. This means that more things are happening at once than the program was configured to handle.");
    return;
  }
  
  event->callback=callback;
  memcpy(event->args, args, sizeof(union SuperType)*num_args);

  if (schedule_event(seqtrack, event, seq_time, priority)==false)
    release_event(event);
}

static void remove_first_event(scheduler_t *scheduler){
  event_t *last = scheduler->queue[scheduler->queue_size];
  int64_t last_time = last->time;

  int i = 1;
  int child = 2;

  event_t **queue = &scheduler->queue[0];
    
  scheduler->queue_size--;

  int queue_size = scheduler->queue_size;
  
  while((child <= queue_size)){
    if(child != queue_size
       && queue[child+1]->time < queue[child]->time)
      child++;

    if(last_time <= queue[child]->time)
      break;

    queue[i] = queue[child];
    i          = child;
    child      = child * 2;
  }
  queue[i] = last;

  queue[queue_size+1] = NULL; // for the gc
}

static int get_priority(event_t *event){
  return event->time & ( (2<<SCHEDULER_NUM_PRIORITY_BITS) - 1);
}

struct SeqTrack *g_RT_curr_scheduling_seqtrack;

static int called_per_block(struct SeqTrack *seqtrack, int64_t reltime){
  g_RT_curr_scheduling_seqtrack = seqtrack;
  
  scheduler_t *scheduler = seqtrack->scheduler;
  
  int64_t end_time = scheduler->current_time + reltime;
  //printf("  called_per_block. end_time: %d. pc->start_time: %f\n",(int)end_time, pc->start_time);

  //printf("called_per_block. seqtrack: %p\n", seqtrack);
  
  while(scheduler->queue_size>0){
    
    event_t *event = get_first_event(scheduler);
    int64_t event_time = event->time >> SCHEDULER_NUM_PRIORITY_BITS;  // remove priority bits.
    //printf("  SCHEDULER: sched: %d - seq: %d.  First event: %d. pc->start_time: %d, pc->end_time: %d\n",(int)end_time, (int)scheduler_to_seq_time(end_time), (int)scheduler_to_seq_time(event_time),(int)pc->start_time, (int)pc->end_time);

    //printf("   Sched. Now: %d,  first event: %d. Seqtrack: %p\n", (int)end_time, (int)event_time, seqtrack);
    
    if(event_time < end_time){
      
      remove_first_event(scheduler);
      
      int64_t new_seq_time = event->callback(seqtrack, event->seq_time, &event->args[0]); // note that the callback can also schedule new events
      
      if (new_seq_time==DONT_RESCHEDULE){
        release_event(event);
      } else {
        int priority = get_priority(event);
        if (schedule_event(seqtrack, event, new_seq_time, priority)==false)
          release_event(event);
      }
      
    }else {
      
      break;

    }
  }

  scheduler->current_time = end_time;

#if !defined(RELEASE)
  g_RT_curr_scheduling_seqtrack = NULL;
#endif
  
  return scheduler->queue_size;
}

// Calls SCHEDULER_called_per_block for all seqtracks.
//
bool SCHEDULER_called_per_block(int64_t reltime){
  bool is_finished = true;

  if (called_per_block(&root->song->block_seqtrack, reltime) > 0)
    is_finished = false;

  VECTOR_FOR_EACH(struct SeqTrack *seqtrack, &root->song->seqtracks){
    
    if (called_per_block(seqtrack, reltime) > 0)
      is_finished = false;
    
  }END_VECTOR_FOR_EACH;

  return is_finished;
}

// * Must be called when deleting a patch or track. (why?)
// * Can't there be hanging notes, or other undefined behaviors, when the event callback is not called?
// Returns true if everything was cleared.
bool SCHEDULER_clear(scheduler_t *scheduler){
  //printf("TODO: Implermnet SCHEDULER_clear\n");

  const int max_to_remove = 20; // 2048/20 = 102.4. 102.4 * 64 frames / (48000 frames / seconds) = 0.1365 seconds. I.e. we should never wait more than approx. 0.14 seconds for all events to be cleared.
  int num_removed = 0;
  
  while(scheduler->queue_size>0 && num_removed < max_to_remove){
    event_t *event = get_first_event(scheduler);
    remove_first_event(scheduler);
    release_event(event);
    num_removed++;
  }

  if (scheduler->queue_size > 0)
    return false;
  else
    return true;
}

bool SCHEDULER_clear_all(void){
  
  if (SCHEDULER_clear(root->song->block_seqtrack.scheduler)==false)
    return false;
  
  VECTOR_FOR_EACH(struct SeqTrack *seqtrack, &root->song->seqtracks){
    
    if (SCHEDULER_clear(seqtrack->scheduler)==false)
      return false;
    
  }END_VECTOR_FOR_EACH;

  return true;
}

int SCHEDULER_num_events(scheduler_t *scheduler){
  return scheduler->queue_size;
}

void SCHEDULER_init(void){
  int i;
  for(i=0;i<QUEUE_SIZE;i++){
    event_t *event = &g_all_events[i]; //talloc(sizeof(event_t));
    event->next = g_free_events;
    g_free_events = event;
  }
  g_event0.time = -100000;
}

scheduler_t *SCHEDULER_create(void){
  scheduler_t *scheduler = talloc_atomic_clean(sizeof(scheduler_t));

  scheduler->queue[0] = &g_event0;

  return scheduler;
}
