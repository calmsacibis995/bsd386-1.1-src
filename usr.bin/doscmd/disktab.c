/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: disktab.c,v 1.2 1994/01/30 09:12:57 donn Exp $ */
static struct {
	int	cylinders;
	int	heads;
	int	sectors;
} disk_table[] = {
	{  306,  4, 17 },	/* type 01   10M	*/
	{  615,  4, 17 },	/* type 02   20M	*/
	{  615,  6, 17 },	/* type 03   30M	*/
	{  940,  8, 17 },	/* type 04   62M	*/
	{  940,  6, 17 },	/* type 05   46M	*/
	{  615,  4, 17 },	/* type 06   20M	*/
	{  462,  8, 17 },	/* type 07   30M	*/
	{  733,  5, 17 },	/* type 08   30M	*/
	{  900, 15, 17 },	/* type 09   112M	*/
	{  820,  3, 17 },	/* type 10   20M	*/
	{  855,  5, 17 },	/* type 11   35M	*/
	{  855,  7, 17 },	/* type 12   49M	*/
	{  306,  8, 17 },	/* type 13   20M	*/
	{  733,  7, 17 },	/* type 14   42M	*/
	{  976, 15, 17 },	/* type 15   121M	*/
	{  612,  4, 17 },	/* type 16   20M	*/
	{  977,  5, 17 },	/* type 17   40M	*/
	{  977,  7, 17 },	/* type 18   56M	*/
	{ 1024,  7, 17 },	/* type 19   59M	*/
	{  733,  5, 17 },	/* type 20   30M	*/
	{  733,  7, 17 },	/* type 21   42M	*/
	{  733,  5, 17 },	/* type 22   30M	*/
	{  306,  4, 17 },	/* type 23   10M	*/
	{  925,  7, 17 },	/* type 24   53M	*/
	{  925,  9, 17 },	/* type 25   69M	*/
	{  754,  7, 17 },	/* type 26   43M	*/
	{  754, 11, 17 },	/* type 27   68M	*/
	{  699,  7, 17 },	/* type 28   40M	*/
	{  823, 10, 17 },	/* type 29   68M	*/
	{  918,  7, 17 },	/* type 30   53M	*/
	{ 1024, 11, 17 },	/* type 31   93M	*/
	{ 1024, 15, 17 },	/* type 32   127M	*/
	{ 1024,  5, 17 },	/* type 33   42M	*/
	{  612,  2, 17 },	/* type 34   10M	*/
	{ 1024,  9, 17 },	/* type 35   76M	*/
	{ 1024,  8, 17 },	/* type 36   68M	*/
	{  615,  8, 17 },	/* type 37   40M	*/
	{  987,  3, 17 },	/* type 38   24M	*/
	{  987,  7, 17 },	/* type 39   57M	*/
	{  820,  6, 17 },	/* type 40   40M	*/
	{  977,  5, 17 },	/* type 41   40M	*/
	{  981,  5, 17 },	/* type 42   40M	*/
	{  830,  7, 17 },	/* type 43   48M	*/
	{  830, 10, 17 },	/* type 44   68M	*/
	{  917, 15, 17 },	/* type 45   114M	*/
	{ 1224, 15, 17 },	/* type 46   152M	*/
};

static int ntypes = sizeof(disk_table)/sizeof(disk_table[0]);

int
map_type(int type, int *cyl, int *head, int *sec)
{
    if (--type < 0 || type >= ntypes)
	return(0);
    *cyl = disk_table[type].cylinders;
    *head = disk_table[type].heads;
    *sec = disk_table[type].sectors;
    return(1);
}
