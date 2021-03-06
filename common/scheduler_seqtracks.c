
#include "nsmtracker.h"
#include "time_proc.h"
#include "playerclass.h"
#include "placement_proc.h"

#include "scheduler_proc.h"

#define DO_DEBUG 0


static int64_t RT_scheduled_seqblock(struct SeqTrack *seqtrack, int64_t time, union SuperType *args);

static void RT_schedule_new_seqblock(struct SeqTrack *seqtrack,
                                     struct SeqBlock *seqblock,
                                     int64_t start_time,
                                     int64_t block_start_time,
                                     const Place *place,
                                     int playtype,
                                     int playlistpos)
{

  seqtrack->curr_seqblock = seqblock; // bang!

  struct Blocks *block = seqblock->block;

  if (playtype==PLAYBLOCK)
    seqblock->time = block_start_time; // We can not set seqblock->time before scheduling since seqblock->time is used various places when playing a block.

#if DO_DEBUG
  printf("  RT_schedule_new_seqblock called. place: %d + %d/%d\n", place->line,place->counter,place->dividor);
#endif
  
  // Schedule notes, fx, etc.
  {
    atomic_pointer_write((void**)&pc->block, block);

    // Update playlist
    //
    pc->playpos = playlistpos;
    if(pc->playtype==PLAYSONG)
      ATOMIC_SET(root->curr_playlist, playlistpos);
    
    if (pc->block != block) {
      ATOMIC_DOUBLE_SET(block->player_time, -100.0); // Any value less than -10 will delay rendering the new block. Instead we wait until player.c is called and a proper player_time value is calculated, to avoid jumpy graphics.
    }

    // GFX
    ATOMIC_SET(root->curr_blocknum, pc->block->l.num);
    ATOMIC_SET(root->setfirstpos, true);

    // Signature
    RT_schedule_Signature_newblock(seqtrack, seqblock, *place);

    // Beats
    RT_schedule_Beats_newblock(seqtrack, seqblock, *place);

    // LPB
    RT_schedule_LPBs_newblock(seqtrack, seqblock, *place);
    
    // Reallines
    RT_schedule_reallines_in_block(seqtrack, seqblock, *place);

    // Send new track pan values to patches
    //
    if(pc->block!=NULL){
      struct Tracks *track=pc->block->tracks;
      while(track!=NULL){
        if(track->panonoff && track->patch!=NULL){
          (*track->patch->changeTrackPan)(track->pan,track);
        }
        track=NextTrack(track);
      }
    }

    // fx
    RT_schedule_fxs_newblock(seqtrack, seqblock, start_time, *place);
    
    // notes
    RT_schedule_notes_newblock(seqtrack, seqblock, start_time, *place);
  }
  
  // Schedule next block, and set various data
  {
    int64_t seqblock_duration = getBlockSTimeLength(block);
    
    int64_t next_time;
    struct SeqBlock *next_seqblock;

    if (playtype==PLAYBLOCK) {
      
      next_seqblock = seqblock;
      next_time = seqblock->time + seqblock_duration;
            
    } else {
      
      next_seqblock = get_next_seqblock(seqtrack, seqblock->time);
      if (next_seqblock != NULL)
        next_time = next_seqblock->time;
      else
        next_time = 0; // To avoid faulty gcc warning.
    }

    // Schedule next block
    //
    if (next_seqblock != NULL) {

      static const Place first_place = {0,0,1};

      const int num_args = 5;
  
      union SuperType args[num_args];

      args[0].pointer       = next_seqblock;
      args[1].int_num       = next_time;
      args[2].const_pointer = &first_place;
      args[3].int32_num     = playtype;
      args[4].int32_num     = playlistpos+1;

#if DO_DEBUG
      printf("  2. Scheduling RT_scheduled_seqblock at %d\n",(int)next_time);
#endif
      SCHEDULER_add_event(seqtrack, next_time, RT_scheduled_seqblock, &args[0], num_args, SCHEDULER_INIT_BLOCK_PRIORITY);

    } else {
      
#if DO_DEBUG
      printf("    3. seqtrack finished.\n");
#endif
      
    }

  }
  

}

static int64_t RT_scheduled_seqblock(struct SeqTrack *seqtrack, int64_t time, union SuperType *args){
  struct SeqBlock       *seqblock         = args[0].pointer;
  int64_t                block_start_time = args[1].int_num;
  const Place           *place            = args[2].const_pointer;
  int                    playtype         = args[3].int32_num;
  int                    playlistpos      = args[4].int32_num;

#if DO_DEBUG
  printf("     RT_scheduled_seqblock called. time: %d\n", (int)time);
#endif
  RT_schedule_new_seqblock(seqtrack, seqblock, time, block_start_time, place, playtype, playlistpos);

  return DONT_RESCHEDULE;
}


static void RT_schedule_seqtracks(struct SeqTrack *seqtrack_notused,
                                  int64_t start_time,
                                  int playtype,
                                  int playlistpos,
                                  const Place *place
                                  )
{
  
  VECTOR_FOR_EACH(struct SeqTrack *seqtrack, &root->song->seqtracks){

    // Schedule the first seqblock in each seqtrack.
    VECTOR_FOR_EACH(struct SeqBlock *seqblock, &seqtrack->seqblocks){

      int64_t seqblock_start_time = seqblock->time;
      int64_t seqblock_end_time   = seqblock_start_time + getBlockSTimeLength(seqblock->block);
      
      if (start_time >= seqblock_start_time && start_time < seqblock_end_time){
        const int num_args = 5;
    
        union SuperType args[num_args];

        args[0].pointer       = seqblock;
        args[1].int_num       = seqblock->time;
        args[2].const_pointer = place;
        args[3].int32_num     = playtype;
        args[4].int32_num     = playlistpos; // FIX
            
#if DO_DEBUG
        printf("  Song: Scheduling RT_scheduled_seqblock at %d. pc->start_time: %d\n",(int)start_time, (int)pc->start_time);
#endif

        SCHEDULER_add_event(seqtrack, start_time, RT_scheduled_seqblock, &args[0], num_args, SCHEDULER_INIT_BLOCK_PRIORITY);

        break;
      }

    }END_VECTOR_FOR_EACH;
    
  }END_VECTOR_FOR_EACH;
}


static int64_t RT_scheduled_startplaying(struct SeqTrack *seqtrack, int64_t time, union SuperType *args){
  int64_t      start_time  = args[0].int_num;
  int          playtype    = args[1].int32_num;
  int          playlistpos = args[2].int32_num;
  const Place *place       = args[3].const_pointer;

#if DO_DEBUG
  printf("  RT_scheduled_startplaying called. playtype: %d %d\n", playtype, PLAYBLOCK);
#endif
  
  if (playtype==PLAYBLOCK){

    R_ASSERT(seqtrack==&root->song->block_seqtrack);
    
    static struct SeqBlock seqblock = {0};
    seqblock.block = pc->block;
    seqblock.time = 0;
 
    const int num_args = 5;
    
    union SuperType args[num_args];
    
    args[0].pointer = &seqblock;
    args[1].int_num = seqblock.time;
    args[2].const_pointer = place;
    args[3].int32_num = playtype;
    args[4].int32_num = 0;

#if DO_DEBUG
    printf("  Scheduling RT_scheduled_seqblock at %d. pc->start_time: %d\n",(int)start_time, (int)pc->start_time);
#endif
    SCHEDULER_add_event(seqtrack, start_time, RT_scheduled_seqblock, &args[0], num_args, SCHEDULER_INIT_BLOCK_PRIORITY);

  } else {
  
    RT_schedule_seqtracks(seqtrack, start_time, playtype, playlistpos, place);
  }

  return DONT_RESCHEDULE;
}


void start_seqtrack_scheduling(int64_t global_start_time, int64_t block_time, Place place, int playtype, int playlistpos){
  static Place static_place;
  static_place = place;

  const int num_args = 4;
  
  union SuperType args[num_args];
  args[0].int_num = global_start_time;
  args[1].int32_num = playtype;
  args[2].int32_num = playlistpos;
  args[3].const_pointer = &static_place;

  R_ASSERT(ATOMIC_GET(pc->player_state)==PLAYER_STATE_STOPPED);

  struct Blocks *block = pc->block;
  if (block!=NULL)
    ATOMIC_DOUBLE_SET(block->player_time, -100.0); // Stop gfx rendering since we are soon going to change the values of pc->end_time and friends.
    
  PLAYER_lock();{
    
    R_ASSERT(SCHEDULER_num_events(RT_get_curr_seqtrack()->scheduler)==0);

    pc->start_time = global_start_time;  // Set play "cursor" position
    ATOMIC_DOUBLE_SET(pc->start_time_f, global_start_time);
    
    pc->end_time = global_start_time; // Must set end_time too, since this is the value used for setting next pc->start_time in the player.
    pc->end_time_f = global_start_time;

#if DO_DEBUG
    printf("  Scheduling start-playing event at 0. Seqtrack: %p\n", RT_get_curr_seqtrack());
#endif
    
    SCHEDULER_add_event(RT_get_curr_seqtrack(), 0, RT_scheduled_startplaying, &args[0], num_args, SCHEDULER_INIT_PRIORITY);
    
  }PLAYER_unlock();
}
