/* Copyright 2016 Kjetil S. Matheussen

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



#include <math.h>

#include "nsmtracker.h"
#include "vector_proc.h"
#include "realline_calc_proc.h"
#include "undo.h"
#include "undo_notes_proc.h"
#include "list_proc.h"
#include "notes_proc.h"
#include "velocities_proc.h"
#include "data_as_text_proc.h"
#include "atomic.h"

#include "veltext_proc.h"


static void add_veltext(const struct WBlocks *wblock, vector_t *veltexts, VelText *veltext, struct Notes *note, int velocity){
  int realline = FindRealLineFor(wblock, 0, &veltext->p);      
  vector_t *v = &veltexts[realline];
  
  veltext->value = round(scale_double(velocity, 0, MAX_VELOCITY, 0, 0xff));
  veltext->note = note;
  
  VECTOR_insert_place(v, &veltext->p);
}
                   

static void add_velocity(const struct WBlocks *wblock, vector_t *veltexts, struct Notes *note, struct Velocities *velocity){
  VelText *tr = talloc(sizeof(VelText));
  tr->p = velocity->l.p;
  tr->velocity = velocity;
  tr->logtype = velocity->logtype;
  add_veltext(wblock, veltexts, tr, note, velocity->velocity);
}

static void add_note(const struct WBlocks *wblock, vector_t *veltexts, struct Notes *note){

  int last_velocity = note->velocity;

  {
    VelText *tr = talloc(sizeof(VelText));
    tr->p = note->l.p;
    tr->logtype = note->velocity_first_logtype;
    tr->is_first_velocity = true;
    
    add_veltext(wblock, veltexts, tr, note, note->velocity);
  }

  struct Velocities *velocity = note->velocities;
  while(velocity != NULL){
    add_velocity(wblock, veltexts, note, velocity);
    last_velocity = velocity->velocity;
    velocity = NextVelocity(velocity);
  }

  if (last_velocity != note->velocity_end)  {
    VelText *tr = talloc(sizeof(VelText));
    tr->p = note->end;
    tr->logtype = LOGTYPE_IRRELEVANT;
    tr->is_last_velocity = true;
    add_veltext(wblock, veltexts, tr, note, note->velocity_end);
  }
}


// Returns a pointer to AN ARRAY of vectors (one vector for each realline), not a pointer to a vector (as one would think).
vector_t *VELTEXTS_get(const struct WBlocks *wblock, const struct WTracks *wtrack){
  int num_reallines = wblock->num_reallines;
  vector_t *veltexts = talloc(sizeof(vector_t) * num_reallines);

  struct Notes *note = wtrack->track->notes;
  while(note!=NULL){
    add_note(wblock, veltexts, note);
    note = NextNote(note);
  }

  return veltexts;
}

int VELTEXT_subsubtrack(struct Tracker_Windows *window, struct WTracks *wtrack){
  int curr_track_sub = window->curr_track_sub;

  if (wtrack->veltext_on == false)
    return -1;

  if (wtrack->centtext_on)
    curr_track_sub -= 2;
  
  if (wtrack->chancetext_on)
    curr_track_sub -= 2;
  
  if (curr_track_sub < 0)
    return -1;

  if (curr_track_sub > 2)
    return -1;

  return curr_track_sub;
}



bool VELTEXT_keypress(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, int realline, Place *place, int key){
  int subsubtrack = VELTEXT_subsubtrack(window, wtrack);

  if (subsubtrack==-1)
    return false;
  
  vector_t *veltexts = VELTEXTS_get(wblock, wtrack);

  vector_t *veltext = &veltexts[realline];

  //  if (veltext->num_elements == 0 && val==0)
  //   return true;
  
  ADD_UNDO(Notes_CurrPos(window));  

  if (veltext->num_elements > 1) {

    // MORE THAN ONE ELEMENT
    
    if (key == EVENT_DEL){

      PLAYER_lock();{
        VECTOR_FOR_EACH(VelText *vt, veltext){
          struct Notes *note = vt->note;
          if (vt->velocity != NULL)
            ListRemoveElement3(&note->velocities, &vt->velocity->l);
        }END_VECTOR_FOR_EACH;
      }PLAYER_unlock();
      
    } else {
      
      Undo_CancelLastUndo();
      
    }
    
  } else if (veltext->num_elements == 0){

    // NO ELEMENTS
    
    struct Notes *note = FindNote(wtrack->track, place);

    if (note == NULL){
      
      Undo_CancelLastUndo();
      
    } else {

      data_as_text_t dat = DAT_get_newvalue(subsubtrack, key, NOTE_get_velocity(wtrack->track), 0, MAX_VELOCITY);

      if (dat.is_valid==false)
        return false;

      struct Velocities *velocity = AddVelocity2(dat.value, place, note);
      safe_int_write(&velocity->logtype, dat.logtype);
      
    }

  } else {

    // ONE ELEMENT
    
    VelText *vt = veltext->elements[0];
    struct Notes *note = vt->note;
    struct Velocities *velocity = vt->velocity;
  
    if (key == EVENT_DEL && velocity != NULL) {

      PLAYER_lock();{
        if (subsubtrack == 2)
          velocity->logtype = LOGTYPE_LINEAR;
        else
          ListRemoveElement3(&note->velocities, &velocity->l);
      }PLAYER_unlock();
      
    } else {

      data_as_text_t dat = DAT_get_overwrite(vt->value, vt->logtype, subsubtrack, key, 0, MAX_VELOCITY, true);

      if (dat.is_valid==false)
        return false;

      if (vt->is_first_velocity){
        
        safe_int_write(&note->velocity, dat.value);
        safe_int_write(&note->velocity_first_logtype, dat.logtype);
        
      } else if (vt->is_last_velocity){
        
        safe_int_write(&note->velocity_end, dat.value);
        
      } else {
        
        safe_int_write(&velocity->velocity, dat.value);
        safe_int_write(&velocity->logtype, dat.logtype);
        
      }
      
    }    
  }

  return true;
}
  
