/* subst.h -- Names of externally visible functions in subst.c. */

#if !defined (_SUBST_H_)
#define _SUBST_H_

/* Cons a new string from STRING starting at START and ending at END,
   not including END. */
extern char *substring ();

/* Remove backslashes which are quoting backquotes from STRING.  Modifies
   STRING, and returns a pointer to it. */
extern char * de_backslash ();

/* Replace instances of \! in a string with !. */
extern void unquote_bang ();

/* Extract the $( construct in STRING, and return a new string.
   Start extracting at (SINDEX) as if we had just seen "$(".
   Make (SINDEX) get the position just after the matching ")". */
extern char *extract_command_subst ();

/* Extract the $[ construct in STRING, and return a new string.
   Start extracting at (SINDEX) as if we had just seen "$[".
   Make (SINDEX) get the position just after the matching "]". */
extern char *extract_arithmetic_subst ();

#if defined (PROCESS_SUBSTITUTION)
/* Extract the <( or >( construct in STRING, and return a new string.
   Start extracting at (SINDEX) as if we had just seen "<(".
   Make (SINDEX) get the position just after the matching ")". */
extern char *extract_process_subst ();
#endif /* PROCESS_SUBSTITUTION */

/* Extract the name of the variable to bind to from the assignment string. */
extern char *assignment_name ();

/* Return a single string of all the words present in LIST, separating
   each word with a space. */
extern char *string_list ();

/* Return a single string of all the words present in LIST, obeying the
   quoting rules for "$*", to wit: (P1003.2, draft 11, 3.5.2) "If the
   expansion [of $*] appears within a double quoted string, it expands
   to a single field with the value of each parameter separated by the
   first character of the IFS variable, or by a <space> if IFS is unset." */
extern char *string_list_dollar_star ();

/* Perform quoted null character removal on each element of LIST.
   This modifies LIST. */
extern void word_list_remove_quoted_nulls ();

/* This performs word splitting and quoted null character removal on
   STRING. */
extern WORD_LIST *list_string ();

extern char *get_word_from_string ();

/* Given STRING, an assignment string, get the value of the right side
   of the `=', and bind it to the left side.  If EXPAND is true, then
   perform parameter expansion, command substitution, and arithmetic
   expansion on the right-hand side.  Perform tilde expansion in any
   case.  Do not perform word splitting on the result of expansion. */
extern int do_assignment ();

/* Append SOURCE to TARGET at INDEX.  SIZE is the current amount
   of space allocated to TARGET.  SOURCE can be NULL, in which
   case nothing happens.  Gets rid of SOURCE by free ()ing it.
   Returns TARGET in case the location has changed. */
extern char *sub_append_string ();

/* Append the textual representation of NUMBER to TARGET.
   INDEX and SIZE are as in SUB_APPEND_STRING. */
extern char *sub_append_number ();

/* Return the word list that corresponds to `$*'. */
extern WORD_LIST *list_rest_of_args ();

/* Make a single large string out of the dollar digit variables,
   and the rest_of_args.  If DOLLAR_STAR is 1, then obey the special
   case of "$*" with respect to IFS. */
extern char *string_rest_of_args ();

/* Expand STRING by performing parameter expansion, command substitution,
   and arithmetic expansion.  Dequote the resulting WORD_LIST before
   returning it, but do not perform word splitting.  The call to
   remove_quoted_nulls () is in here because word splitting normally
   takes care of quote removal. */
extern WORD_LIST *expand_string_unsplit ();

/* Expand STRING just as if you were expanding a word.  This also returns
   a list of words.  Note that filename globbing is *NOT* done for word
   or string expansion, just when the shell is expanding a command.  This
   does parameter expansion, command substitution, arithmetic expansion,
   and word splitting.  Dequote the resultant WORD_LIST before returning. */
extern WORD_LIST *expand_string ();

/* Expand WORD, performing word splitting on the result.  This does
   parameter expansion, command substitution, arithmetic expansion,
   word splitting, and quote removal. */
extern WORD_LIST *expand_word ();

/* Expand WORD, but do not perform word splitting on the result.  This
   does parameter expansion, command substitution, arithmetic expansion,
   and quote removal. */
extern WORD_LIST *expand_word_no_split (), *expand_word_leave_quoted ();

/* Return the value of a positional parameter.  This handles values > 10. */
extern char *get_dollar_var_value ();

/* Perform quote removal on STRING.  If QUOTED > 0, assume we are obeying the
   backslash quoting rules for within double quotes. */
extern char *string_quote_removal ();

extern char *dequote_string ();

/* Perform quote removal on word WORD.  This allocates and returns a new
   WORD_DESC *. */
extern WORD_DESC *word_quote_removal ();

/* Perform quote removal on all words in LIST.  If QUOTED is non-zero,
   the members of the list are treated as if they are surrounded by
   double quotes.  Return a new list, or NULL if LIST is NULL. */
extern WORD_LIST *word_list_quote_removal ();

/* This splits a single word into a WORD LIST on $IFS, but only if the word
   is not quoted.  list_string () performs quote removal for us, even if we
   don't do any splitting. */
extern WORD_LIST *word_split ();

/* Do all of the assignments in LIST up to a word which isn't an
   assignment. */
extern WORD_LIST *get_rid_of_variable_assignments ();

/* Check and handle the case where there are some variable assignments
   in LIST which go into the environment for this command. */
extern WORD_LIST *get_rid_of_environment_assignments ();

/* Take the list of words in LIST and do the various substitutions.  Return
   a new list of words which is the expanded list, and without things like
   variable assignments. */
extern WORD_LIST *expand_words ();

/* Same as expand_words (), but doesn't hack variable or environment
   variables. */
extern WORD_LIST *expand_words_no_vars ();

/* PATHNAME can contain characters prefixed by CTLESC;; this indicates
   that the character is to be quoted.  We quote it here in the style
   that the glob library recognizes.  If CONVERT_QUOTED_NULLS is non-zero,
   we change quoted null strings (pathname[0] == CTLNUL) into empty
   strings (pathname[0] == 0).  If this is called after quote removal
   is performed, CONVERT_QUOTED_NULLS should be 0; if called when quote
   removal has not been done (for example, before attempting to match a
   pattern while executing a case statement), CONVERT_QUOTED_NULLS should
   be 1. */
extern char *quote_string_for_globbing ();

/* Call the glob library to do globbing on PATHNAME. */
extern char **shell_glob_filename ();

/* The variable in NAME has just had its state changed.  Check to see if it
   is one of the special ones where something special happens. */
extern void stupidly_hack_special_variables ();

#endif /* !_SUBST_H_ */
