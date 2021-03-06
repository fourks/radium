/* Copyright 2000 Kjetil S. Matheussen

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

#ifndef RADIUM_COMMON_OS_PLAYER_PROC_H
#define RADIUM_COMMON_OS_PLAYER_PROC_H


//extern LANGSPEC void PLAYER_acquire_same_priority(void);
//extern LANGSPEC void PLAYER_drop_same_priority(void);

// PLAYER_lock is a RT-safe lock. Priority of the calling thread is set to the same value as the player before acquiring the lock.
// Warning, PLAYER_lock will deadlock if called from the player thread.
extern LANGSPEC void PLAYER_lock(void);
extern LANGSPEC void PLAYER_unlock(void);

extern LANGSPEC bool MIXER_is_saving(void);

// Must be called in SoundPlugin::RT_process if doing things there which requires the PLAYER_lock. (separate lock because of multicore processing, PLAYER_lock is always held inside SoundPlugin::RT_process)
extern LANGSPEC void RT_PLAYER_runner_lock(void);
extern LANGSPEC void RT_PLAYER_runner_unlock(void);

extern LANGSPEC bool PLAYER_current_thread_has_lock(void);
extern LANGSPEC bool PLAYER_someone_has_player_lock(void);
extern LANGSPEC bool PLAYER_player_has_player_lock(void);

extern LANGSPEC void StartPlayer(void);

extern LANGSPEC void StopPlayer(void);

extern LANGSPEC void PausePlayer(void);
extern LANGSPEC void StopPausePlayer(void);

extern LANGSPEC void OS_WaitForAShortTime(int milliseconds);
extern LANGSPEC void OS_WaitAtLeast(int milliseconds); // Use this function instead if it is important that we don't return too early.

extern LANGSPEC void PLAYER_volumeUp(float db);
extern LANGSPEC void PLAYER_volumeDown(float db);
extern LANGSPEC void PLAYER_mute(void);

extern LANGSPEC void RT_request_to_start_playing(void);
extern LANGSPEC void RT_request_to_continue_playing(void);
extern LANGSPEC void RT_request_to_stop_playing(void);
extern LANGSPEC void RT_pause_plugins(void);

#ifdef __cplusplus

namespace radium{
  
struct PlayerLock{
  PlayerLock()
  {
    PLAYER_lock();
  }

  ~PlayerLock(){
    PLAYER_unlock();
  }
};
 
struct PlayerRecursiveLock{
  bool gotit;
  
  PlayerRecursiveLock()
  : gotit (!PLAYER_current_thread_has_lock())
  {
    if (gotit)
      PLAYER_lock();
  }

  ~PlayerRecursiveLock(){
    if (gotit)
      PLAYER_unlock();
  }
};
 
}

#endif

#endif

