/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: AsyncIO.h,v 1.1 1994/01/14 23:21:41 polk Exp $ */
#if defined(__cplusplus)
extern "C" {
#endif
void	_RegisterIO(int, void (*)(void *), void *, void (*)(void *));
void	_AssociateIO(int, int);
void	_DeAssociateIO(int, int);
void	_LockIO(int);
int	_UnlockIO(int);
int	_LevelIO(int);
int	_DetachIO(int);
int	_EndIO(int, int);
void	_BlockIO(void);
void	_UnblockIO(void);
#if defined(__cplusplus)
}
#endif

#define	_Un_RegisterIO(x) _RegisterIO((x), (void (*))0, (void *)0, (void (*))0)
