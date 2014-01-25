/*
  getch_msvc.h
  
  getch/getwch replacement for msvc/mingw32
  (to replace enhenced key prefix 0xE0 with 0x00)
  
  Licence: いわゆる Public Domain ってことで
           (You can use/modify/redistibute it freely BUT NO WARRANTY.)
  
*/

#ifndef GETCH_MSVC_H
#define GETCH_MSVC_H

/* header */
extern int getch_replacement_for_msvc(void);
extern int getche_replacement_for_msvc(void);
#ifdef BUILD_FOR_WCHAR
extern int getwch_replacement_for_msvc(void);
extern int getwche_replacement_for_msvc(void);
#endif

#endif /* GETCH_MSVC_H */
