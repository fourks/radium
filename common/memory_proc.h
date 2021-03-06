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

#ifndef RADIUM_COMMON_MEMORY_PROC_H
#define RADIUM_COMMON_MEMORY_PROC_H


#include <stdlib.h>
#include <string.h>

#ifndef TRACKER_INCLUDE

extern LANGSPEC void Threadsafe_GC_disable(void);
extern LANGSPEC void Threadsafe_GC_enable(void);

extern LANGSPEC void init_memory(void);

extern LANGSPEC void tfree(void *element);

//extern LANGSPEC void *tracker_alloc_clean(int size,void *(*AllocFunction)(int size2));

extern LANGSPEC void *tracker_alloc__(int size,void *(*AllocFunction)(size_t size2), const char *filename, int linenumber);

extern LANGSPEC void *tralloc__(int size, const char *filename, int linenumber);

extern LANGSPEC void *tralloc_atomic__(int size, const char *filename, int linenumber);

extern LANGSPEC void *talloc__(int size, const char *filename, int linenumber);

extern LANGSPEC void *talloc_atomic__(int size, const char *filename, int linenumber);

extern LANGSPEC void *talloc_atomic_uncollectable__(int size, const char *filename, int linenumber);

extern LANGSPEC void *talloc_realloc__(void *v, int new_size, const char *filename, int linenumber);

extern LANGSPEC void *talloc_atomic_clean__(int size, const char *filename, int linenumber);

extern LANGSPEC char *talloc_strdup__(const char *input, const char *filename, int linenumber);

extern LANGSPEC char *talloc_numberstring__(int number, const char *filename, int linenumber);

extern LANGSPEC char *talloc_floatstring__(float number, const char *filename, int linenumber);



#define tracker_alloc(a,b) tracker_alloc__(a,b,__FILE__,__LINE__) 

#define tralloc(a) tralloc__(a,__FILE__,__LINE__) 

#define tralloc_atomic(a) tralloc_atomic__(a,__FILE__,__LINE__)

#define talloc(a) talloc__(a,__FILE__,__LINE__)

#define talloc_atomic(a) talloc_atomic__(a,__FILE__,__LINE__)

#define talloc_atomic_uncollectable(a) talloc_atomic_uncollectable__(a,__FILE__,__LINE__)

#define talloc_realloc(a,b) talloc_realloc__(a,b,__FILE__,__LINE__)

#define talloc_atomic_clean(a) talloc_atomic_clean__(a,__FILE__,__LINE__)

#define talloc_strdup(a) talloc_strdup__(a,__FILE__,__LINE__)

#define talloc_numberstring(a) talloc_numberstring__(a,__FILE__,__LINE__)

#define talloc_floatstring(a) talloc_floatstring__(a,__FILE__,__LINE__)

#define tcopy(mem, size) memcpy(talloc(size), mem, size)
#define tcopy_atomic(mem, size) memcpy(talloc_atomic(size), mem, size)

extern LANGSPEC char *talloc_format(const char *fmt,...);

#endif



#endif
