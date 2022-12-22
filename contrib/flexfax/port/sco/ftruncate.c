int
ftruncate(int fd, int sz)
{
    return (chsize(fd, sz));
}
