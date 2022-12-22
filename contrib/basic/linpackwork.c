/*     dgeco factors a double precision matrix by gaussian elimination
       and estimates the condition of the matrix.
     
      if  rcond  is not needed, dgefa is slightly faster.
      to solve  A*x = B , follow dgeco by dgesl.
      to compute  inverse(A)*c , follow dgeco by dgesl.
      to compute  determinant(A) , follow dgeco by dgedi.
      to compute  inverse(A) , follow dgeco by dgedi.
      
      on entry:
         a       double precision[lda, n]: the matrix to be factored.
         lda     integer: the leading dimension of the array  a .
         n       integer: the order of the matrix  a .
      
      on return
         a       an upper triangular matrix and the multipliers
                 which were used to obtain it.
                 the factorization can be written  a = l*u  where
                 l  is a product of permutation and unit lower
                 triangular matrices and  u  is upper triangular.
         ipvt    integer(n): an integer vector of pivot indices.
      
         rcond   double precision
                 an estimate of the reciprocal condition of  a .
                 for the system  a*x = b , relative perturbations
                 in  a  and  b  of size  epsilon  may cause
                 relative perturbations in  x  of size  epsilon/rcond .
                 if  rcond  is so small that the logical expression
                            1.0 + rcond .eq. 1.0
                 is true, then  a  may be singular to working
                 precision.  in particular,  rcond  is zero  if
                 exact singularity is detected or the estimate
                 underflows.
      
         z       double precision(n)
                 a work vector whose contents are usually unimportant.
                 if  a  is close to a singular matrix, then  z  is
                 an approximate null vector in the sense that
                 norm(a*z) = rcond*norm(a)*norm(z) .
      
      linpack. this version dated 03/11/78 .
      cleve moler, university of new mexico, argonne national labs.
      
      subroutines and functions
      
      linpack dgefa
      blas daxpy,ddot,dscal,dasum
      fortran dabs,dmax1,dsign
*/

dabs(a) double a; { return a < 0 ? -a : a; }

#define a(i,j)  a_addr[(i)*lda+(j)]

dgeco(a,lda,n,ipvt,rcond,z)
	double *a_addr;	/* [lda][] */
	int lda;
	int n;
	int lda;
	int ipvt[];
	double rcond;
	double z[];
{

      
      /* internal variables: */
      
      double ddot(), dsign(), ek, t, wk, wkm;
      double anorm, s, dasum(), sm, ynorm;
      int info, j, k, kb, kp1, l;

	/* compute 1-norm of a: */

      anorm = 0.0;
	for (j = 0; j < n; j++)
         anorm = dmax1(anorm,dasum(n,a(0,j),1));


      dgefa(a_addr, lda, n, ipvt, info);	/* factor */

/*
      rcond = 1/(norm(a)*(estimate of norm(inverse(a)))) .
      estimate = norm(z)/norm(y) where  a*z = y  and  trans(a)*y = e .
      trans(a)  is the transpose of a .  the components of  e  are
      chosen to cause maximum local growth in the elements of w  where
      trans(u)*w = e .  the vectors are frequently rescaled to avoid
      overflow.
*/

      /* solve trans(u)*w = e: */

	ek = 1.0;
	for (j = 0; j < n; j++)
		z[j] = 0.0;
	for (k = 0; k < n; k++) {
		if (z[k] != 0.0)
			ek = dsign(ek, -z[k]);
         if (dabs(ek-z[k]) > dabs(a[k,k]))  {
            s = dabs(a(k,k))/dabs(ek-z[k]);
            call dscal(n,s,z,1);
            ek = s*ek;
	 }
         wk = ek - z[k];
         wkm = -ek - z[k];
         s = dabs(wk);
         sm = dabs(wkm);
         if (a(k,k) == 0.0) go to 40
	{
            wk = 1.0;
            wkm = 1.0;
	}
	else {
            wk = wk/a(k,k);
            wkm = wkm/a(k,k);
	}
         kp1 = k + 1;
         if (kp1 <= n) 
	{
	    for (j = k+1; j < n; j++) {
               sm += dabs(z[j]+wkm*a(k,j));
               z[j] += wk*a(k,j);
               s += dabs(z[j]);
	    }
            if (s < sm)  {
               t = wkm - wk;
               wk = wkm;
		for (j = k + 1; j < n; j++)
                  z[j] += t*a(k,j);
	    }
	}

         z[k] = wk;
	}
      s = 1.0/dasum(n, z, 1);
      call dscal(n, s, z, 1);

      /* solve trans(l)*y = w */

	for (kb =0; kb < n; kb++) {
         k = n + 1 - kb
         if (k < n) { z[k] += ddot(n-k,a(k+1,k),1,z[k+1],1); }
         if (dabs(z[k]) > 1.0)  {
            s = 1.0/dabs(z[k]);
            call dscal(n, s, z, 1);
	  }
	/* swap: */
         l = ipvt[k]; t = z[l]; z[l] = z[k]; z[k] = t;
    }
      s = 1.0d0/dasum(n,z,1)
      call dscal(n,s,z,1)

      ynorm = 1.0;

	/* solve l*v = y */

	for (k = 0; k < n; k++) {
		/* swap */
         l = ipvt[k] t = z[l]; z[l] = z[k]; z[k] = t;
         if (k < n)
		daxpy(n-k, t, a(k+1,k), 1, z[k+1], 1);
XXX above statement has n-k...n-k+1 ???
         if (dabs(z[k]) > 1.0) {
            s = 1.0/dabs(z[k])
            dscal(n, s, z, 1);
            ynorm = s*ynorm;
	}
	}
      s = 1.0/dasum(n,z,1);
      dscal(n, s, z, 1);
      ynorm *= s;

      /* solve  u*z = v */

	for (kb = 0; kb < n; kb++) {
         k = n + 1 - kb;
XXX n + 1 ???
         if (dabs(z[k]) > dabs(a(k,k))) {
            s = dabs(a(k,k))/dabs(z(k));
            dscal(n, s, z, 1);
            ynorm *= s;
	}
         if (a(k,k) != 0.0d0) z[k] = z[k]/a(k,k);
         else z[k] = 1.0;
         t = -z[k];
         daxpy(k-1, t, a(0, k), 1, z(0), 1);
	}

      /* make znorm = 1.0 */
      s = 1.0/dasum(n, z, 1);
      call dscal(n, s, z, 1)
      ynorm *= s;

      if (anorm != 0.0) rcond = ynorm/anorm
	else rcond = 0.0;
}

dgedi(a_addr, lda, n, ipvt, det, work, job)
double *a_addr;
int lda;
int n;
int ipvt[];
double det[2];
double work[];
int job;
{
  /*
      dgedi computes the determinant and inverse of a matrix
      using the factors computed by dgeco or dgefa.
      
      on entry
      
         a       double precision(lda, n)
                 the output from dgeco or dgefa.

         lda     integer
                 the leading dimension of the array  a .
      
         n       integer
                 the order of the matrix  a .
      
         ipvt    integer(n)
                 the pivot vector from dgeco or dgefa.
      
         work    double precision(n)
                 work vector.  contents destroyed.
      
         job     integer
                 = 11   both determinant and inverse.
                 = 01   inverse only.
                 = 10   determinant only.
      
      on return
      
         a       inverse of original matrix if requested.
                 otherwise unchanged.
      
         det     double precision(2)
                 determinant of original matrix if requested.
                 otherwise not referenced.
                 determinant = det(1) * 10.0**det(2)
                 with  1.0 .le. dabs(det(1)) .lt. 10.0
                 or  det(1) .eq. 0.0 .
      
      error condition
      
         a division by zero will occur if the input factor contains
         a zero on the diagonal and the inverse is requested.
         it will not occur if the subroutines are called correctly
         and if dgeco has set rcond .gt. 0.0 or dgefa has set
         info .eq. 0 .
      
      linpack. this version dated 03/11/78 .
      cleve moler, university of new mexico, argonne national labs.
      
      subroutines and functions
      
      blas daxpy,dscal,dswap
      fortran dabs,mod
      
      internal variables
*/
      
      double t, ten;
      int i, j, k, kb, kp1, l, nm1;

      if (job/10 !=  0) {
      		/* compute determinant */

         det[0] = 1.0;
         det[1] = 0.0;
         ten = 10.0;
	for (i = 0; i < n; i++) {
            if (ipvt[i] != i) det[0] = -det[0];
            det[0] = a(i,i)*det[0];

c        ...exit

            if (det[0] .eq. 0.0) break;

            while (dabs(det[0]) < 1.0){
               det[0] *= ten;
               det[1] -= 1.0;
	    }
            while (dabs(det[0]) >=  ten)  {
               det[0] /= ten;
               det[1] += 1.0;
	    }
      }
    }

	/* compute inverse(u) */

      if (job%10 != 0) 
	{
	for (k = 0; k < n; k++ ) {
            a(k,k) = 1.0/a(k,k);
            t = -a(k,k);
            dscal(k-1, t, a(1,k) ,1);	/* XXX: k-1?  1,k?  */
            kp1 = k + 1
            if (n .lt. kp1) go to 90
            do 80 j = kp1, n
               t = a(k,j)
               a(k,j) = 0.0d0
               call daxpy(k,t,a(1,k),1,a(1,j),1)
   80       continue
   90       continue
	}
c     
c        form inverse(u)*inverse(l)
c     
         nm1 = n - 1
         if (nm1 .lt. 1) go to 140
         do 130 kb = 1, nm1
            k = n - kb
            kp1 = k + 1
            do 110 i = kp1, n
               work(i) = a(i,k)
               a(i,k) = 0.0d0
  110       continue
            do 120 j = kp1, n
               t = work(j)
               call daxpy(n,t,a(1,j),1,a(1,k),1)
  120       continue
            l = ipvt(k)
            if (l .ne. k) call dswap(n,a(1,k),1,a(1,l),1)
  130    continue
  140    continue
	}
      return
      end
      subroutine dgefa(a,lda,n,ipvt,info)
      integer lda,n,ipvt(1),info
      double precision a(lda,1)
c     
c     dgefa factors a double precision matrix by gaussian elimination.
c     
c     dgefa is usually called by dgeco, but it can be called
c     directly with a saving in time if  rcond  is not needed.
c     (time for dgeco) = (1 + 9/n)*(time for dgefa) .
c     
c     on entry
c     
c        a       double precision(lda, n)
c                the matrix to be factored.
c     
c        lda     integer
c                the leading dimension of the array  a .
c     
c        n       integer
c                the order of the matrix  a .
c     
c     on return
c     
c        a       an upper triangular matrix and the multipliers
c                which were used to obtain it.
c                the factorization can be written  a = l*u  where
c                l  is a product of permutation and unit lower
c                triangular matrices and  u  is upper triangular.
c     
c        ipvt    integer(n)
c                an integer vector of pivot indices.
c     
c        info    integer
c                = 0  normal value.
c                = k  if  u(k,k) .eq. 0.0 .  this is not an error
c                     condition for this subroutine, but it does
c                     indicate that dgesl or dgedi will divide by zero
c                     if called.  use  rcond  in dgeco for a reliable
c                     indication of singularity.
c     
c     linpack. this version dated 03/11/78 .
c     cleve moler, university of new mexico, argonne national labs.
c     
c     subroutines and functions
c     
c     blas daxpy,dscal,idamax
c     
c     internal variables
c     
      double precision t
      integer idamax,j,k,kp1,l,nm1
c     
c     
c     gaussian elimination with partial pivoting
c     
      info = 0
      nm1 = n - 1
      if (nm1 .lt. 1) go to 70
      do 60 k = 1, nm1
         kp1 = k + 1
c     
c        find l = pivot index
c     
         l = idamax(n-k+1,a(k,k),1) + k - 1
         ipvt(k) = l
c     
c        zero pivot implies this column already triangularized
c     
         if (a(l,k) .eq. 0.0d0) go to 40
c     
c           interchange if necessary
c     
            if (l .eq. k) go to 10
               t = a(l,k)
               a(l,k) = a(k,k)
               a(k,k) = t
   10       continue
c     
c           compute multipliers
c     
            t = -1.0d0/a(k,k)
            call dscal(n-k,t,a(k+1,k),1)
c     
c           row elimination with column indexing
c     
            do 30 j = kp1, n
               t = a(l,j)
               if (l .eq. k) go to 20
                  a(l,j) = a(k,j)
                  a(k,j) = t
   20          continue
               call daxpy(n-k,t,a(k+1,k),1,a(k+1,j),1)
   30       continue
         go to 50
   40    continue
            info = k
   50    continue
   60 continue
   70 continue
      ipvt(n) = n
      if (a(n,n) .eq. 0.0d0) info = n
      return
      end


double
dasum(n,dx,incx)
int n;
double dx[];
int incx;
{

	/* takes the sum of the absolute values.
           jack dongarra, linpack, 6/17/77. */
      
      double dtemp;
      int i,incx,m,mp1;
c      
      dasum = 0.0d0
      dtemp = 0.0d0
      if(n.le.0)return
      if(incx.eq.1)goto 20
c      
c        code for increment not equal to 1
c      
      nincx = n*incx
      do 10 i = 1,nincx,incx
        dtemp = dtemp + dabs(dx(i))
   10 continue
      dasum = dtemp
      return
c      
c        code for increment equal to 1
c      
c      
c        clean-up loop
c      
   20 m = mod(n,6)
      if( m .eq. 0 ) go to 40
      do 30 i = 1,m
        dtemp = dtemp + dabs(dx(i))
   30 continue
      if( n .lt. 6 ) go to 60
   40 mp1 = m + 1
      do 50 i = mp1,n,6
        dtemp = dtemp + dabs(dx(i)) + dabs(dx(i + 1)) + dabs(dx(i + 2))
     *  + dabs(dx(i + 3)) + dabs(dx(i + 4)) + dabs(dx(i + 5))
   50 continue
   60 dasum = dtemp
      return
      end
      subroutine  dscal(n,da,dx,incx)
c      
c     scales a vector by a constant.
c     uses unrolled loops for increment equal to one.
c     jack dongarra, linpack, 6/17/77.
c      
      double precision da,dx(1)
      integer i,incx,m,mp1,n,nincx
c      
      if(n.le.0)return
      if(incx.eq.1)goto 20
c      
c        code for increment not equal to 1
c      
      nincx = n*incx
      do 10 i = 1,nincx,incx
        dx(i) = da*dx(i)
   10 continue
      return
c      
c        code for increment equal to 1
c      
c      
c        clean-up loop
c      
   20 m = mod(n,5)
      if( m .eq. 0 ) go to 40
      do 30 i = 1,m
        dx(i) = da*dx(i)
   30 continue
      if( n .lt. 5 ) return
   40 mp1 = m + 1
      do 50 i = mp1,n,5
        dx(i) = da*dx(i)
        dx(i + 1) = da*dx(i + 1)
        dx(i + 2) = da*dx(i + 2)
        dx(i + 3) = da*dx(i + 3)
        dx(i + 4) = da*dx(i + 4)
   50 continue
      return
      end


      subroutine  dswap (n,dx,incx,dy,incy)
c      
c     interchanges two vectors.
c     uses unrolled loops for increments equal one.
c     jack dongarra, linpack, 6/17/77.
c      
      double precision dx(1),dy(1),dtemp
      integer i,incx,incy,ix,iy,m,mp1,n
c      
      if(n.le.0)return
      if(incx.eq.1.and.incy.eq.1)goto 20
c      
c       code for unequal increments or equal increments not equal
c         to 1
c      
      ix = 1
      iy = 1
      if(incx.lt.0)ix = (-n+1)*incx + 1
      if(incy.lt.0)iy = (-n+1)*incy + 1
      do 10 i = 1,n
        dtemp = dx(ix)
        dx(ix) = dy(iy)
        dy(iy) = dtemp
        ix = ix + incx
        iy = iy + incy
   10 continue
      return
c      
c       code for both increments equal to 1
c      
c      
c       clean-up loop
c      
   20 m = mod(n,3)
      if( m .eq. 0 ) go to 40
      do 30 i = 1,m
        dtemp = dx(i)
        dx(i) = dy(i)
        dy(i) = dtemp
   30 continue
      if( n .lt. 3 ) return
   40 mp1 = m + 1
      do 50 i = mp1,n,3
        dtemp = dx(i)
        dx(i) = dy(i)
        dy(i) = dtemp
        dtemp = dx(i + 1)
        dx(i + 1) = dy(i + 1)
        dy(i + 1) = dtemp
        dtemp = dx(i + 2)
        dx(i + 2) = dy(i + 2)
        dy(i + 2) = dtemp
   50 continue
      return
      end
      integer function idamax(n,dx,incx)
      double precision dx(1),dmax
      integer i,incx,ix,n
c      
      idamax = 0
      if(n.le.0)return
      idamax = 1
      if(n.eq.1)return
      if(incx.eq.1)goto 20
c      
c        code for increment not equal to 1
c      
      ix = 1
      dmax = dabs(dx(1))
      ix = ix + incx
      do 10 i = 2, n
         if (dabs(dx(ix)).le.dmax) go to 5
         idamax = i
         dmax = dabs(dx(ix))
    5    ix = ix + incx
   10 continue
      return
c      
c        code for increment equal to 1
c      
c      
   20 dmax = dabs(dx(1))
      do 30 i = 2, n
         if (dabs(dx(i)).le.dmax) go to 30
         idamax = i
         dmax = dabs(dx(i))
   30 continue
      return
      end
