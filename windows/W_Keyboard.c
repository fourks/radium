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

#ifdef FOR_WINDOWS

#include <windows.h>

#include "../common/nsmtracker.h"
#include "../common/playerclass.h"
#include "../common/eventreciever_proc.h"
#include "../common/player_proc.h"
#include "../common/scancodes_proc.h"
#include "../common/OS_visual_input.h"
#include "../audio/Mixer_proc.h"

#include "../common/OS_system_proc.h"
#include "W_Keyboard_proc.h"


#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))


extern struct TEvent tevent;

static DEFINE_ATOMIC(bool, left_windows_down) = false;
static DEFINE_ATOMIC(bool, right_windows_down) = false;

static unsigned int g_last_keyswitch;

void OS_WINDOWS_set_always_on_top(void *child_handle){
#if 0
  //SetWindowPos(main_handle, hwnd, 100, 100, 1000, 1000, SWP_SHOWWINDOW);
  SetWindowPos(hwnd, HWND_TOP, 100, 100, 1000, 1000, SWP_SHOWWINDOW);
#else
  HWND child_hwnd = (HWND)child_handle;
  HWND parent_hwnd = (HWND)OS_GFX_get_native_main_window();

  SetWindowLongPtr(child_hwnd, -8, (LONG_PTR)parent_hwnd);
#endif
}

static uint32_t get_keyswitch(void){
  uint32_t keyswitch=0;

  bool is_left_ctrl = GetKeyState(VK_LCONTROL) & 0x8000;
  bool is_right_ctrl = GetKeyState(VK_RCONTROL) & 0x8000;
    
  if(is_right_ctrl)
    keyswitch |= EVENT_RIGHTCTRL;
  if(is_left_ctrl)
    keyswitch |= EVENT_LEFTCTRL;

  if(GetKeyState(VK_LSHIFT)&0x8000)
    keyswitch |= EVENT_LEFTSHIFT;
  if(GetKeyState(VK_RSHIFT)&0x8000)
    keyswitch |= EVENT_RIGHTSHIFT;

  bool is_left_alt = GetKeyState(VK_LMENU) & 0x8000;

  if (is_left_alt)
    keyswitch |= EVENT_LEFTALT;

  if(GetKeyState(VK_RMENU)&0x8000)
    keyswitch |= EVENT_RIGHTALT;

  if(ATOMIC_GET(left_windows_down))
    keyswitch |= EVENT_LEFTEXTRA1;
  if(ATOMIC_GET(right_windows_down))
    keyswitch |= EVENT_RIGHTEXTRA1;

  bool is_right_alt =    
    (GetKeyState(VK_RMENU) & 0x8000) ||
    (is_left_alt && is_left_ctrl) || // alt+ctrl is the same as altgr on windows
    (is_left_alt && is_right_ctrl) ||  // alt+ctrl is the same as altgr on windows
    keyswitch == 0xc2 // Don't quite know why. It's just the number that pops up on my windows 8.1 machine.
    ;

  // Some quick hacking. Windows detects right alt as ctrl+left alt. This should probably be programmed properly in the future though.
  if(is_right_alt) {
    keyswitch &= ~EVENT_LEFTALT;
    keyswitch &= ~EVENT_RIGHTCTRL;
    keyswitch &= ~EVENT_LEFTCTRL;
    keyswitch |= EVENT_RIGHTALT;
  }

  return keyswitch;
}

static int keymap[0x100] = {EVENT_NO};

// https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
static void init_keymap(void){
  // alpha
  keymap[0x41] = EVENT_A;
  keymap[0x42] = EVENT_B;
  keymap[0x43] = EVENT_C;
  keymap[0x44] = EVENT_D;
  keymap[0x45] = EVENT_E;
  keymap[0x46] = EVENT_F;
  keymap[0x47] = EVENT_G;
  keymap[0x48] = EVENT_H;
  keymap[0x49] = EVENT_I;
  keymap[0x4a] = EVENT_J;
  keymap[0x4b] = EVENT_K;
  keymap[0x4c] = EVENT_L;
  keymap[0x4d] = EVENT_M;
  keymap[0x4e] = EVENT_N;
  keymap[0x4f] = EVENT_O;
  keymap[0x50] = EVENT_P;
  keymap[0x51] = EVENT_Q;
  keymap[0x52] = EVENT_R;
  keymap[0x53] = EVENT_S;
  keymap[0x54] = EVENT_T;
  keymap[0x55] = EVENT_U;
  keymap[0x56] = EVENT_V;
  keymap[0x57] = EVENT_W;
  keymap[0x58] = EVENT_X;
  keymap[0x59] = EVENT_Y;
  keymap[0x5a] = EVENT_Z;

  // row 1
  keymap[VK_ESCAPE] = EVENT_ESC;
  keymap[VK_F1] = EVENT_F1;
  keymap[VK_F2] = EVENT_F2;
  keymap[VK_F3] = EVENT_F3;
  keymap[VK_F4] = EVENT_F4;
  keymap[VK_F5] = EVENT_F5;
  keymap[VK_F6] = EVENT_F6;
  keymap[VK_F7] = EVENT_F7;
  keymap[VK_F8] = EVENT_F8;
  keymap[VK_F9] = EVENT_F9;
  keymap[VK_F10] = EVENT_F10;
  keymap[VK_F11] = EVENT_F11;
  keymap[VK_F12] = EVENT_F12;

  // row 2
  //keymap[VK_OEM_3] = EVENT_1L1;
  keymap[0x31] = EVENT_1;
  keymap[0x32] = EVENT_2;
  keymap[0x33] = EVENT_3;
  keymap[0x34] = EVENT_4;
  keymap[0x35] = EVENT_5;
  keymap[0x36] = EVENT_6;
  keymap[0x37] = EVENT_7;
  keymap[0x38] = EVENT_8;
  keymap[0x39] = EVENT_9;
  keymap[0x30] = EVENT_0;
  //keymap[VK_OEM_MINUS] = EVENT_0R1;
  //keymap[VK_OEM_PLUS]  = EVENT_0R2;
  keymap[VK_BACK]      = EVENT_BACKSPACE;

  // row 3
  keymap[VK_TAB]    = EVENT_TAB;
  //keymap[VK_OEM_4]  = EVENT_PR1;
  //keymap[VK_OEM_6]  = EVENT_PR2;
  keymap[VK_RETURN] = EVENT_RETURN;

  // row 4
  //keymap[VK_OEM_1] = EVENT_LR1;
  //keymap[VK_OEM_7] = EVENT_LR2;
  //keymap[VK_OEM_5] = EVENT_LR3;

  // row 5
  //keymap[VK_OEM_102]    = EVENT_ZL1;
  //keymap[VK_OEM_COMMA]  = EVENT_MR1;
  //keymap[VK_OEM_PERIOD] = EVENT_MR2;
  //keymap[VK_OEM_2]      = EVENT_MR3;

  // row 6
  keymap[VK_SPACE] = EVENT_SPACE;

  // insert/del/etc.
  keymap[VK_INSERT] = EVENT_INSERT;
  keymap[VK_HOME] = EVENT_HOME;
  keymap[VK_PRIOR] = EVENT_PAGE_UP;
  keymap[VK_NEXT] = EVENT_PAGE_DOWN;
  keymap[VK_DELETE] = EVENT_DEL;
  keymap[VK_END] = EVENT_END;

  keymap[VK_APPS] = EVENT_MENU;
  keymap[VK_VOLUME_MUTE] = EVENT_MUTE;
  keymap[VK_VOLUME_DOWN] = EVENT_VOLUME_DOWN;
  keymap[VK_VOLUME_UP] = EVENT_VOLUME_UP;
  keymap[VK_MEDIA_STOP] = EVENT_STOP;
  keymap[VK_MEDIA_PLAY_PAUSE] = EVENT_PLAY;


  // arrows
  keymap[VK_LEFT]  = EVENT_LEFTARROW;
  keymap[VK_UP]    = EVENT_UPARROW;
  keymap[VK_RIGHT] = EVENT_RIGHTARROW;
  keymap[VK_DOWN]  = EVENT_DOWNARROW;

  // keypad
  // ... is handled in the function get_keypad_subID
}

#if 0
// Didn't work. GetKeyNameText returned something else on XP and windows 7
static int get_keypad_subID(MSG *msg){
  char temp[500];
  GetKeyNameText(msg->lParam,temp,500);

  printf("Keyname: \"%s\"\n",temp);
  fflush(stdout);

  if(strlen(temp)<4 || temp[0]!='K' || temp[1]!='P' || temp[2]!='_')
    return EVENT_NO;

  if(!strcmp(temp,"KP_Insert"))
    return EVENT_KP_0;
  if(!strcmp(temp,"KP_Delete"))
    return EVENT_KP_DOT;
  if(!strcmp(temp,"KP_Enter"))
    return EVENT_KP_ENTER;

  if(!strcmp(temp,"KP_End"))
    return EVENT_KP_1;
  if(!strcmp(temp,"KP_Down"))
    return EVENT_KP_2;
  if(!strcmp(temp,"KP_Next"))
    return EVENT_KP_3;

  if(!strcmp(temp,"KP_Left"))
    return EVENT_KP_4;
  if(!strcmp(temp,"KP_Begin"))
    return EVENT_KP_5;
  if(!strcmp(temp,"KP_Right"))
    return EVENT_KP_6;

  if(!strcmp(temp,"KP_Home"))
    return EVENT_KP_7;
  if(!strcmp(temp,"KP_Up"))
    return EVENT_KP_8;
  if(!strcmp(temp,"KP_Prior"))
    return EVENT_KP_9;

  if(!strcmp(temp,"KP_Add"))
    return EVENT_KP_ADD;
  if(!strcmp(temp,"KP_Subtract"))
    return EVENT_KP_SUB;
  if(!strcmp(temp,"KP_Multiply"))
    return EVENT_KP_MUL;
  if(!strcmp(temp,"KP_Divide"))
    return EVENT_KP_DIV;

  return EVENT_NO;
}
#endif

static int get_keypad_subID2(uint32_t wParam){
  if(wParam==VK_RETURN)
    return EVENT_KP_ENTER;
  if(wParam==VK_DIVIDE)
    return EVENT_KP_DIV;

  switch(wParam){
  case VK_MULTIPLY:
    return EVENT_KP_MUL;
  case 0x6d:
    return EVENT_KP_SUB;
  case 0x24:
    return EVENT_KP_7;
  case 0x26:
    return EVENT_KP_8;
  case 0x21:
    return EVENT_KP_9;
  case 0x25:
    return EVENT_KP_4;
  case 0x0c:
    return EVENT_KP_5;
  case 0x27:
    return EVENT_KP_6;
  case 0x23:
    return EVENT_KP_1;
  case 0x28:
    return EVENT_KP_2;
  case 0x22:
    return EVENT_KP_3;
  case 0x2d:
    return EVENT_KP_0;
  case 0x2e:
    return EVENT_KP_DOT;
  case 0x6b:
    return EVENT_KP_ADD;
  }
  return EVENT_NO;
}

static int get_keypad_subID(MSG *msg){
  if(0x1000000 & msg->lParam){ // i.e. insert/home/pageup/delete/end/pagedown, an arrow key, KP_DIV or KP_ENTER. (strange flag)
    if(msg->wParam==VK_RETURN)
      return EVENT_KP_ENTER;
    if(msg->wParam==VK_DIVIDE)
      return EVENT_KP_DIV;
    return EVENT_NO;
  }

  switch(msg->wParam){
  case VK_MULTIPLY:
    return EVENT_KP_MUL;
  case 0x6d:
    return EVENT_KP_SUB;
  case 0x24:
    return EVENT_KP_7;
  case 0x26:
    return EVENT_KP_8;
  case 0x21:
    return EVENT_KP_9;
  case 0x25:
    return EVENT_KP_4;
  case 0x0c:
    return EVENT_KP_5;
  case 0x27:
    return EVENT_KP_6;
  case 0x23:
    return EVENT_KP_1;
  case 0x28:
    return EVENT_KP_2;
  case 0x22:
    return EVENT_KP_3;
  case 0x2d:
    return EVENT_KP_0;
  case 0x2e:
    return EVENT_KP_DOT;
  case 0x6b:
    return EVENT_KP_ADD;
  }
  return EVENT_NO;
}

int OS_SYSTEM_get_keynum2(uint32_t wParam, bool keypad_pressed){
  if(wParam >= 0x100)
    return EVENT_NO;

  if (keypad_pressed){
    int subID = get_keypad_subID2(wParam);
    if(subID!=EVENT_NO)
      return subID;
  }

  return keymap[wParam];
}

int OS_SYSTEM_get_keynum(void *void_event){
  MSG *msg = (MSG*)void_event;
  
  if(msg->wParam >= 0x100)
    return EVENT_NO;

  int subID = get_keypad_subID(msg);
  if(subID!=EVENT_NO)
    return subID;

  return keymap[msg->wParam];
}

int OS_SYSTEM_get_qwerty_keynum2(uint32_t scancode){
  if (scancode!=0){
    int subID = get_subID_from_scancode(scancode);
    if (subID!=EVENT_NO)
      return subID;
  }

  return EVENT_NO;
}

int OS_SYSTEM_get_qwerty_keynum(void *void_event) {
  MSG *msg = (MSG*)void_event;
  
  int scancode = MapVirtualKey(msg->wParam, MAPVK_VK_TO_VSC);
  if (scancode!=0){
    int subID = get_subID_from_scancode(scancode);
    if (subID!=EVENT_NO)
      return subID;
  }

  return EVENT_NO;
}

int OS_SYSTEM_get_scancode(void *void_event) {
  MSG *msg = (MSG*)void_event;
  
  int scancode = MapVirtualKey(msg->wParam, MAPVK_VK_TO_VSC);
  return scancode;
}

static DEFINE_ATOMIC(bool, g_bWindowActive) = true;

void OS_SYSTEM_EventPreHandler(void *void_event){
  MSG *msg = (MSG*)void_event;

  // https://msdn.microsoft.com/en-us/library/windows/desktop/ff468922(v=vs.85).aspx
  switch(msg->message){
    
    case WM_NCACTIVATE:
      ATOMIC_SET(g_bWindowActive, msg->wParam ? true : false);
      //printf("1. Got NC Activate. wParam: %d. Active: %p\n",(int)msg->wParam,GetForegroundWindow());
      //fflush(stdout);
      //if (msg->wParam==0)
      //call_me_if_another_window_may_have_taken_focus_but_still_need_our_key_events();
      break;
      
    case WM_ACTIVATE:
      ATOMIC_SET(g_bWindowActive, msg->wParam ? true : false);
      //printf("2. Got Activate. wParam: %d. Active: %p\n",(int)msg->wParam,GetForegroundWindow());
      //fflush(stdout);
      //if (msg->wParam==0)
      //call_me_if_another_window_may_have_taken_focus_but_still_need_our_key_events();
      break;
      
    case WM_ACTIVATEAPP:
      ATOMIC_SET(g_bWindowActive, msg->wParam ? true : false);
      //printf("3. Got Activate app. wParam: %d. Active: %p\n",(int)msg->wParam,GetForegroundWindow());
      //fflush(stdout);
      //if (msg->wParam==0)
      //call_me_if_another_window_may_have_taken_focus_but_still_need_our_key_events();
      break;

    case WM_CHILDACTIVATE:
      //printf("4. Child activate Active: %p\n",GetForegroundWindow());
      break;
    case WM_ENTERSIZEMOVE:
      //printf("5. Enter/size/move Active: %p\n",GetForegroundWindow());
      break;
      
      //default:
      //printf("5. Unknown message.                      Active: %p\n",GetForegroundWindow());
  }

  int type = OS_SYSTEM_get_event_type(void_event, true);
  if (type==TR_KEYBOARD || type==TR_KEYBOARDUP){
    g_last_keyswitch = tevent.keyswitch;
    tevent.keyswitch = get_keyswitch();
  }
}

static bool event_is_arrow(WPARAM w){
  return w==VK_DOWN || w==VK_UP || w==VK_LEFT || w==VK_RIGHT || w==VK_PRIOR || w==VK_NEXT;
}

int OS_SYSTEM_get_event_type(void *void_event, bool ignore_autorepeat){
  MSG *msg = (MSG*)void_event;
  switch(msg->message){
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    {
      if (ignore_autorepeat && !event_is_arrow(msg->wParam)){
        if (CHECK_BIT(msg->lParam, 30)) { // autorepeat. https://msdn.microsoft.com/en-us/library/windows/desktop/ms646280(v=vs.85).aspx
          return TR_AUTOREPEAT;
        }
      }
      return TR_KEYBOARD;
    }
  case WM_KEYUP: 
  case WM_SYSKEYUP:
    return TR_KEYBOARDUP;
  default:
    return -1;
  }
}


// Note that OS_SYSTEM_get_modifier is unable to return an EVENT_EXTRA_L event. Several other problems too.
int OS_SYSTEM_get_modifier(void *void_msg){
  MSG *msg = (MSG*)void_msg;
  
  int type = OS_SYSTEM_get_event_type(void_msg, true);
  
  unsigned int keyswitch;
  if (type==TR_KEYBOARD)
    keyswitch = tevent.keyswitch;
  else if (type==TR_KEYBOARDUP)
    keyswitch = g_last_keyswitch;
  else
    return EVENT_NO;
  
  switch(msg->wParam){
  case VK_SHIFT:
    {
      if (keyswitch & EVENT_RIGHTSHIFT)
        return EVENT_SHIFT_R;
      if (keyswitch & EVENT_LEFTSHIFT)
        return EVENT_SHIFT_L;
    }
    break;
  case VK_CONTROL:
    {
      if (keyswitch & EVENT_RIGHTCTRL)
        return EVENT_CTRL_R;
      if (keyswitch & EVENT_LEFTCTRL)
        return EVENT_CTRL_L;
    }
    break;
  case VK_MENU:
    {
      if (keyswitch & EVENT_RIGHTALT)
        return EVENT_ALT_R;
      if (keyswitch & EVENT_LEFTALT)
        return EVENT_ALT_L;
    }
    break;
    //  case VK_APPS:
    //    return EVENT_EXTRA_R;
  }
  
  return EVENT_NO;
}

static HWINEVENTHOOK g_system_foreground_hook = NULL;
static HHOOK g_hKeyboardHook = NULL;

// http://msdn.microsoft.com/en-us/library/windows/desktop/ee416808(v=vs.85).aspx
static LRESULT CALLBACK LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
          
    if (nCode < 0 || nCode != HC_ACTION )  // do not process message 
        return CallNextHookEx( g_hKeyboardHook, nCode, wParam, lParam); 
 
    KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

    if(wParam==WM_KEYDOWN || wParam==WM_KEYUP){
      //printf("^^^^^^^^^^^^^^^^^^ p: %p\n",p);
      //printf("^^^^^^^^^^^^^^^^^^^^^ vkCode: %x\n",(int)p->vkCode);

#if 0
      printf("  %s: vkCode: %x\n",wParam==WM_KEYDOWN ? "Down":"Up", (int)p->vkCode);
#endif
      
      if(p->vkCode==VK_LWIN || p->vkCode==VK_RWIN){// || p->vkCode==VK_RMENU){

        if(p->vkCode==VK_LWIN)
          ATOMIC_SET(left_windows_down, wParam==WM_KEYDOWN);
        else
          ATOMIC_SET(right_windows_down, wParam==WM_KEYDOWN);

#if 0
        if(p->vkCode==VK_LWIN && wParam==WM_KEYDOWN)
          printf("   1. Left down\n");
        else if (p->vkCode==VK_LWIN && wParam==WM_KEYUP)
          printf("   1. Left up\n");
        else if (p->vkCode==VK_RMENU && wParam==WM_KEYDOWN)
          printf("   2. Right down\n");
        else if (p->vkCode==VK_RMENU)
          printf("   2. Right up\n");
#endif
        
        #if 0
        // Don't think this is any point.
        if (left_windows_down)
          tevent.keyswitch |= EVENT_LEFTEXTRA1;
        else
          tevent.keyswitch &= (~EVENT_LEFTEXTRA1);
        #endif
        
        //printf("active: %d, left: %s, right: %s. switch: %x\n",g_bWindowActive, left_windows_down?"down":"up", right_windows_down?"down":"up", );
        if(ATOMIC_GET(g_bWindowActive)==true)
          return 1; // To avoid having the windows menu pop up when the radium window is active and pressing left windows key.
      }
    }
 
    return CallNextHookEx( g_hKeyboardHook, nCode, wParam, lParam );
}

// Code copied from here: https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/f6032ca1-31b8-4ad5-be39-f78dd29952da/hooking-problem-in-windows-7?forum=windowscompatibility
static DWORD WINAPI hookThreadProc(LPVOID lParam)
{
    MSG msg;

    g_hKeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0 );
    
    while(GetMessage(&msg, NULL, 0, 0) != FALSE)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

bool OS_WINDOWS_is_key_window(void *maybewin){
  //printf("a: %p, b: %p. %d\n",maybewin, GetForegroundWindow(), maybewin == GetForegroundWindow());
  return maybewin == GetForegroundWindow();
}

static VOID CALLBACK WinEventProcCallback ( HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
  //fprintf(stderr, "\n\n  =======  WinEventHook.  Foreground window : %p     ========\n\n",GetForegroundWindow());
  //fflush(stderr);
  call_me_if_another_window_may_have_taken_focus_but_still_need_our_key_events();
}
 

void OS_SYSTEM_init_keyboard(void) {
  init_keymap();  

  g_system_foreground_hook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND , 
                                             EVENT_SYSTEM_FOREGROUND ,
                                             NULL, 
                                             WinEventProcCallback, 0, 0, 
                                             WINEVENT_OUTOFCONTEXT //| WINEVENT_SKIPOWNPROCESS);
                                             );


  CreateThread( 
               NULL,                   // default security attributes
               0,                      // use default stack size  
               hookThreadProc,       // thread function name
               NULL,          // argument to thread function 
               0,                      // use default creation flags 
               NULL   // returns the thread identifier 
                );
    
  //g_hKeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0 );
}

void W_KeyboardHandlerShutDown(void){
  //if(g_hKeyboardHook!=NULL)
  //  UnhookWindowsHookEx(g_hKeyboardHook);

  if(g_system_foreground_hook!=NULL)
    UnhookWinEvent(g_system_foreground_hook);
}

#ifdef RUN_TEST
struct Root *root = NULL;
struct TEvent tevent = {0};
PlayerClass *pc = NULL;
int num_users_of_keyboard = 0;
int64_t MIXER_get_time(void){
  return 0;
}
bool EventReciever(struct TEvent *tevent,struct Tracker_Windows *window){
  return true;
}
void PlayBlockFromStart(struct Tracker_Windows *window,bool do_loop){
}

// x86_64-w64-mingw32-g++ -Wall W_Keyboard.c -mconsole -DRUN_TEST -DFOR_WINDOWS -DDEBUG -I../Qt/ -DUSE_QT_REQTYPE=1 ../common/scancodes.c `mingw64-pkg-config --libs --cflags QtGui` && wine64 a.exe 

#include <QApplication>
#include <QPushButton>

class MyApplication : public QApplication{
public:

  MyApplication(int &argc,char **argv)
    : QApplication(argc,argv)
    {
      //setStyleSheet("QStatusBar::item { border: 0px solid black }; ");
    }

protected:

  bool last_key_was_lalt;

  
  bool SystemEventFilter(void *event){
    OS_SYSTEM_EventPreHandler(event);
    
    MSG *msg = (MSG*)event;
    int type = OS_SYSTEM_get_event_type(event, true);
    if (type!=-1){
      int keynum = OS_SYSTEM_get_keynum(event);
      int qwerty = OS_SYSTEM_get_qwerty_keynum(event);
      printf("Got %s event: %d / %d. swiktch: %x. wparam: 0x%x. Modifier: %d. Autorepeat: %s\n",
             type==TR_KEYBOARD?"press":"release",keynum,qwerty,get_keyswitch(),msg->wParam,OS_SYSTEM_get_modifier(event),type==TR_AUTOREPEAT?"true":"false"
             );
    }
    return false;
  }

  bool 	winEventFilter ( MSG * msg, long * result ){
    return SystemEventFilter(msg);
  }

};

int main(int argc, char **argv)
{
  OS_SYSTEM_init_keyboard();

  MyApplication app (argc, argv);
  
  QPushButton button ("Hello world !");
  button.show();
  
  app.exec();
  
  W_KeyboardHandlerShutDown();
  return 0;
}

#endif

#endif // FOR_WINDOWS

