#ifndef __MACROMAN_H__
#define __MACROMAN_H__

struct charmap
{
	int length;
	char *cp;
};

extern struct charmap macroman[128];

#endif
