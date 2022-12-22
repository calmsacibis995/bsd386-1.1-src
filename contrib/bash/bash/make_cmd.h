/* make_cmd.h -- Declarations of functions found in make_cmd.c */

#if !defined (_MAKE_CMD_H_)
#define _MAKE_CMD_H_

extern WORD_DESC *make_word (), *make_word_from_token (), *coerce_to_word ();
extern WORD_LIST *make_word_list (), *add_string_to_list ();
extern COMMAND *make_command (), *command_connect ();
extern COMMAND *make_for_command (), *make_group_command ();
extern COMMAND *make_case_command (), *make_if_command ();
extern COMMAND *make_until_or_while (), *make_while_command ();
extern COMMAND *make_until_command (), *make_bare_simple_command ();
extern COMMAND *make_simple_command (), *make_function_def ();

extern PATTERN_LIST *make_pattern_list ();
extern REDIRECT *make_redirection ();

extern void make_here_document ();
extern char **make_word_array ();

#endif /* !_MAKE_CMD_H */
