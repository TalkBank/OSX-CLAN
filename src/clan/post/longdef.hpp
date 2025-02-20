/**********************************************************************
	"Copyright 1990-2025 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/

// 2020-06-11 beg
#ifndef _LONGDEF_H_
#define _LONGDEF_H_

#if defined(POSTCODE) || defined(_MAC_CODE)
  #if __LP64__
	#define long32 int
  #else
	#define long32 long
  #endif
#else
	#define long32 int
#endif /* defined(POSTCODE) || defined(_MAC_CODE) */

#endif /* ifndef _LONGDEF_H_ */
// 2020-06-11 end
