daxpy_(n,sa,sx,incx,sy,incy) double *sa, *sx, *sy; int *n, *incx, *incy; {
	register double *psx, *psy, vsa;
	register int ix, iy, i;

	if(*n <= 0)
		return;
	if(*sa == 0.0)
		return;
	vsa = *sa;
	ix = *incx;
	iy = *incy;
	if( ix < 0)
		psx = &sx[ (-(*n)+1)*ix];
	else
		psx = sx;
	if( iy < 0)
		psy = &sy[ (-(*n)+1)*iy];
	else
		psy = sy;
	for(i=0; i<*n; i++) {
		*psy += vsa * *psx;
		psy += iy;
		psx += ix;
	}
}
double ddot_(n,sx,incx,sy,incy) double *sx, *sy; int *n, *incx, *incy; {
	register double *psx, *psy, stemp;
	register int ix, iy, i;

	if(*n <= 0)
		return(0.0);
	ix = *incx;
	iy = *incy;
	stemp = 0.0;
	if( ix < 0)
		psx = &sx[ (-(*n)+1)*ix];
	else
		psx = sx;
	if( iy < 0)
		psy = &sy[ (-(*n)+1)*iy];
	else
		psy = sy;

	for(i=0; i < *n; i++) {
		stemp += *psy * *psx;
		psy += iy;
		psx += ix;
	}
	return(stemp);
}
