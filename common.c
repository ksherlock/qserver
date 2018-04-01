#pragma noroot
#pragma lint -1
#pragma optimize -1

#include <Memory.h>
#include <Resources.h>
#include <IntMath.h>
#include <texttool.h>

#include "macroman.h"

#if 0
void printint(char *str, int i)
{
char tmp[6];
	if (str && *str) WriteCString(str);

	Int2Dec(i, tmp, 5, 0);
	tmp[5] = (char)0;
	WriteCString(tmp);

	WriteCString("\r\n");
}
#endif


extern pascal Word Random(void) inline(0x8604,dispatcher);

Handle LoadQuote(word mID, Word rFile)
{
Word oFile;
Word oDepth;

int rID;
Word rCount;

Handle rHandle;
Handle h;


	h = NULL;

	oFile = GetCurResourceFile();
	SetCurResourceFile(rFile);
	oDepth = SetResourceFileDepth(1);
	rCount = CountResources(rTextForLETextBox2);

	//printint("rCount: ", rCount);

	if (rCount)
	{
	WordDivRec wdr;
	longword index;

	  wdr = UDivide(Random(), (word)rCount);

	//printint("remainder: ", wdr.remainder);

	  index = GetIndResource(rTextForLETextBox2, wdr.remainder + 1);

	// todo - repeat above until index out of range
	//printint("index: ", index);


	  rHandle = LoadResource(rTextForLETextBox2, index);
	//printint("loadres: ", _toolErr);
	  if (!_toolErr)
	  {
	    word oldLen;
	    word newLen;
	    char *newText;
	    char *oldText;

	    word len;
	    word i;

	    oldLen = (word)GetHandleSize(rHandle);
	//printint("oldLen: ", oldLen);

	    h = NewHandle(oldLen << 2 + 2, mID, 0, NULL);
	    if (!_toolErr)
	    {
	      HLock(h);
	      HLock(rHandle);

	      oldText = *rHandle;
	      newText = *h;
	      newLen = 0;

	      i = 0;
	      while (i < oldLen)
	      {
	      char c;

		c  = *oldText++;
		i++;

		if (c == '\r')
		{
		  *((Word *)newText) = 0x0A0D;
                  newText += 2;
		  newLen += 2;
		  continue;

			//*newText++ = '\r';
			//*newText++ = '\n';
			//newLen += 2;
			//continue;
		}
	        if (c == 0x01) // formatting codes.
		{
		  c = *oldText;
		  switch (c)
		  {
		  case 'F': // font
			oldText += 5;
			i += 5;
			break;

		  case 'S': // set style
			oldText += 5;
			i += 4;
			break;

		  case 'C': // fore color
		  case 'B': // back color
		  case 'J': // justify
		  case 'L': // left margin
		  case 'R': // right margin
		  case 'X': // extra space
			oldText += 3;
			i += 3;
			break;
		  }
		  continue;
		}
		if (c & 0x80) // macroman encoding.
		{
		int j;
		  c &= 0x7f;
		  j = macroman[c].length;

		  if (j == 0) continue;

		  if (j > 1)
		  {
		  char *cp;
		    cp = macroman[c].cp;

		    newLen += j;
		    do
		    {
		      *newText++ = *cp++;
		    }
		    while (--j);

		    continue;
		  }
		  else c = (char)macroman[c].cp;
		}

		newLen++;
		*newText++ = c;
	      }

	      //*newText++ = '\r';
	      //*newText++ = '\n';
	      *((Word *)newText) = 0x0A0D;
	      newLen += 2;
	      HUnlock(h);
	      SetHandleSize(newLen, h);
	    }

	    HUnlock(rHandle);
	    ReleaseResource(0, rTextForLETextBox2, index);
	  }
	}
	SetCurResourceFile(oFile);
	SetResourceFileDepth(oDepth);

	return h;
}
