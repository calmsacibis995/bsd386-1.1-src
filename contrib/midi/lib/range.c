
/*
 * $Id: range.c,v 1.1.1.1 1992/08/14 00:14:35 trent Exp $
 *
 * Copyright (C) 1992, Berkeley Software Design, Inc. 
 *  based on code written for BSDI by Mike Durian
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>

int
get_range(str, range)
	char	*str;
	int 	*range;
{
	int		pos;
	char	divider;
	int		i;
	int		hi_range;
	int		count = 0;

	if (sscanf(str, "%d%n", range, &pos) != 1)
		return (-1);
	str += pos;
	count++;

	while (sscanf(str, "%c", &divider) == 1) {
		str++;
		switch (divider) {
		case ',':
			range++;
			if (sscanf(str, "%d%n", range, &pos) != 1)
				return (-1);
			str += pos;
			count++;
			break;
		case '-':
			if (sscanf(str, "%d%n", &hi_range, &pos) != 1)
				return (-1);
			str += pos;
			for (i = *range + 1; i <= hi_range; i++) {
				count++;
				*(++range) = i;
			}
			range++;
			break;
		default:
			return (-1);
		}
	}

	return (count);
}
