;# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/lib/importenv.pl,v 1.1.1.1 1992/07/27 23:24:55 polk Exp $

;# This file, when interpreted, pulls the environment into normal variables.
;# Usage:
;#	require 'importenv.pl';
;# or
;#	#include <importenv.pl>

local($tmp,$key) = '';

foreach $key (keys(ENV)) {
    $tmp .= "\$$key = \$ENV{'$key'};" if $key =~ /^[A-Za-z]\w*$/;
}
eval $tmp;

1;
