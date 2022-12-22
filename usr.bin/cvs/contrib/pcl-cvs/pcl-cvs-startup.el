;;; $Id: pcl-cvs-startup.el,v 1.1.1.1 1992/07/28 01:47:11 polk Exp $
(autoload 'cvs-update "pcl-cvs"
	  "Run a 'cvs update' in the current working directory. Feed the
output to a *cvs* buffer and run cvs-mode on it.
If optional prefix argument LOCAL is non-nil, 'cvs update -l' is run."
	  t)
