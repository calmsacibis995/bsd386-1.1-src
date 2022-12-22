char **environ;

start(int argc, char **argv, char **env)
{
  environ = env;
  exit(main(argc, argv, environ));
}
