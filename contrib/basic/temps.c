# include "structs.h"

int *IntTemp(n) {
	if (IntSpace != NULL) {
		RunError("Basic Bug! Needed int temps twice!");
		abort();
	}
	IntSpace = (int *) malloc((unsigned)(n*sizeof(int)));
	return IntSpace;
}

double *DoubleTemp(n) {
	if (DoubleSpace != NULL) {
		RunError("Basic Bug!  Needed temps twice!");
		abort();
	}
	DoubleSpace = (double *) malloc((unsigned)(n*sizeof(double)));
	return DoubleSpace;
}

char *StringTemp() {
	/* Skip over previous string */
	StringTempPtr += strlen(StringTempPtr)+1;
	if (StringTempPtr + MAX_STRLEN >= StringSpace + sizeof StringSpace) {
		RunError("HELP!  Too much string temporary space used.\n");
	}
	return StringTempPtr;
}
