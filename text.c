/* text.c v1.0
 * Text processing methods.
 * Copyright (c) 2009 Carlos E. Torchia
 */

#include <string.h>
#include <ctype.h>

#define	min(x,y)	((x>=y)?y:x)

/* ========= get_last_occurrence ==========
 * Computes the last occurrence array for p
 * Pre: last_occurrence[] is of dimension sizeof(char)*256,
 *      and p is a null-terminated string
 */

void	get_last_occurrence(char *p,int *last_occurrence)
{
  int	i,j;
  
  for(i=0;i<=sizeof(char)*256-1;i++) {
    last_occurrence[i]=-1;
  }

  for(j=0;p[j]!='\0';j++) {
    last_occurrence[(unsigned char)p[j]]=j;
  }
}
 
/* ========= instr =======
 * Returns the index at which p occurs in t, -1 otherwise
 * Note: this uses the Boyer-Moore algorithm. (Algorithm Design. Goodrich,
 *       Tammassia. 2002. NJ: J. Wiley.) But it is modified for case-
 *       insensitivity.
 */

int	instr(char *t,char *p)
{
  int	last_occurrence[sizeof(char)*256];
  int	m=strlen(p),n=strlen(t);
  int	i=m-1,j=m-1;
  int	l;

  get_last_occurrence(p,last_occurrence);

  do {

    if(tolower(t[i])==tolower(p[j])) {	/* compare pattern until mismatch */
      if(j==0) {
        return(i);
      }
      else {
        i--;
	j--;
      }
    }

    else {		/* jump ahead appropriately */
      l=last_occurrence[(unsigned char)t[i]];
      i+=m-min(j,1+l);
      j=m-1;
    }

  } while(i<=n-1);

  return(-1);
}
