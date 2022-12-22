/* bracecomp.c -- Complete a filename with the possible completions enclosed
   in csh-style braces such that the list of completions is available to the
   shell. */

/* Original version by tromey@cns.caltech.edu,  Fri Feb  7 1992. */

#include <stdio.h>

#if defined (HAVE_STRING_H)
#  include <string.h>
#else /* !HAVE_STRING_H */
#  include <strings.h>
#endif /* !HAVE_STRING_H */

#include "shell.h"
#include <readline/readline.h>

extern char *filename_completion_function ();

/* Find greatest common prefix of two strings. */
static int
string_gcd (s1, s2)
     char *s1, *s2;
{
  register int i;

  if (s1 == NULL || s2 == NULL)
    return (0);

  for (i = 0; *s1 && *s2; ++s1, ++s2, ++i)
    {
      if (*s1 != *s2)
	break;
    }

  return (i);
}

static char *
really_munge_braces (array, real_start, real_end, gcd_zero)
     char **array;
     int real_start, real_end, gcd_zero;
{
  int start, end, gcd;
  char *result, *subterm;
  int result_size, flag;

  flag = 0;

  if (real_start == real_end)
    return (savestring (array[real_start] + gcd_zero));

  result = (char *) xmalloc (result_size = 1);
  *result = '\0';

  for (start = real_start; start < real_end; start = end + 1)
    {
      gcd = strlen (array[start]);
      for (end = start + 1; end < real_end; end++)
	{
	  int temp;

	  temp = string_gcd (array[start], array[end]);

	  if (temp <= gcd_zero)
	    break;

	  gcd = temp;
	}
      end--;

      if (gcd_zero == 0 && start == real_start && end != (real_end - 1))
	{
	  /* In this case, add in a leading '{', because we are at
	     top level, and there isn't a consistent prefix. */
	  result_size += 1;
	  result = (char *) xrealloc (result, result_size);
	  strcpy (result, "{");
	  flag++;
	}

      if (start == end)
	subterm = savestring (array[start] + gcd_zero);
      else
	{
	  /* If there is more than one element in the subarray,
	     insert the prefix and an opening brace. */
	  result_size += gcd - gcd_zero + 1;
	  result = (char *) xrealloc (result, result_size);
	  strncat (result, array[start] + gcd_zero, gcd - gcd_zero);
	  strcat (result, "{");
	  subterm = really_munge_braces (array, start, end + 1, gcd);
	  subterm[strlen (subterm) - 1] = '}';
	}

      result_size += strlen (subterm) + 1;
      result = (char *) xrealloc (result, result_size);
      strcat (result, subterm);
      strcat (result, ",");
      free (subterm);
    }

  if (gcd_zero == 0)
    result[strlen (result) - 1] = flag ? '}' : '\0';
  return (result);
}

static void
hack_braces_completion (names)
     char **names;
{
  register int i;
  char *temp;

  temp = really_munge_braces (names, 1, array_len (names), 0);

  for (i = 0; names[i]; ++i)
    {
      free (names[i]);
      names[i] = NULL;
    }
  names[0] = temp;
}

void
bash_brace_completion ()
{
  Function *orig_ignore_func;
  Function *orig_entry_func;
  CPPFunction *orig_attempt_func;

  orig_ignore_func = rl_ignore_some_completions_function;
  orig_attempt_func = rl_attempted_completion_function;
  orig_entry_func = rl_completion_entry_function;

  rl_completion_entry_function = (Function *) filename_completion_function;
  rl_attempted_completion_function = NULL;
  rl_ignore_some_completions_function = (Function *) hack_braces_completion;

  rl_complete_internal (TAB);

  rl_ignore_some_completions_function = orig_ignore_func;
  rl_attempted_completion_function = orig_attempt_func;
  rl_completion_entry_function = orig_entry_func;
}
