/* common.h -- extern declarations for functions defined in common.c. */

#if  !defined (__COMMON_H)
#  define __COMMON_H

extern int get_numeric_arg ();

extern void remember_args ();

extern void no_args ();

extern int read_octal ();

extern char *find_hashed_filename ();
extern void remove_hashed_filename ();

extern void push_context (), pop_context ();
extern void push_dollar_vars (), pop_dollar_vars ();
extern int dispose_saved_dollar_vars ();
extern int dollar_vars_changed ();
extern void set_dollar_vars_unchanged (), set_dollar_vars_changed ();

extern int bad_option ();

/* Keeps track of the current working directory. */
extern char *the_current_working_directory;
extern char *get_working_directory ();
extern void set_working_directory ();

#if defined (JOB_CONTROL)
extern int get_job_spec ();
#endif

extern int parse_and_execute ();
extern int parse_and_execute_cleanup ();

/* It's OK to declare a function as returning a Function * without
   providing a definition of what a `Function' is. */
extern Function *find_shell_builtin ();
extern Function *builtin_address ();

extern char *single_quote ();
extern char *double_quote ();

#endif /* !__COMMON_H */
