#pragma nda NDAOpen NDAClose NDAAction NDAInit 30 0xffff  "--Quote Server\\H**"
#pragma lint -1
#pragma optimize -1

#include <Types.h>
#include <GSOS.h>
#include <Memory.h>
#include <Locator.h>
#include <Loader.h>
#include <Desk.h>
#include <Event.h>
#include <Window.h>
#include <Control.h>
#include <Resources.h>
#include <Quickdraw.h>
#include <QDAux.h>
#include <Font.h>
#include <stdfile.h>
#include <TextEdit.h>
#include <intmath.h>
#include <TCPIP.h>
#include <misctool.h>

#include <kstring.h>

#include "qserver.h"

Handle LoadQuote(word mID, Word rfile);

Word LoadConfig(Word MemID);
void UnloadConfig(void);
void DoConfig(Word MemID);


const char *ReqName = "\pTCP/IP~kelvin~qserver~";

/*
    variables
 */

WindowPtr MyWindow;
Boolean FlagTCP;
Boolean FlagQS;

Boolean FlagQDAux;
Boolean FlagFM;
Boolean FlagTE;
Boolean FlagSF;

Boolean FlagLoadTCP;

Handle HandleFM;
Handle HandleTE;
Handle HandleSF;

Word MyID;
Word MyRID;
Word Ipid;


word rFile;
Handle rPath;
word rCount;

struct qentry
{
	Word ipid;
	Word state;
	Longword tick;
};

#define QSIZE 16
struct qentry queue[QSIZE];

word total;
word current;


void fixstats(void)
{
static char stats[16];
Word i;

  i = ksprintf(stats + 1, "%D : %D", current, total);
  stats[0] = i; // pascal string

  SetInfoRefCon((LongWord)stats, MyWindow);
  DrawInfoBar(MyWindow);
}


void InsertString(word length, char *cp)
{
Handle handle;
TERecord **temp;
longword oldStart, oldEnd;

	handle = (Handle)GetCtlHandleFromID(MyWindow, CtrlTE);
	temp = (TERecord **)handle;

	(**temp).textFlags &= (~fReadOnly);

	TEGetSelection((pointer)&oldStart, (pointer)&oldEnd, handle);

	TESetSelection((Pointer)-1, (Pointer)-1, handle);
	TEInsert(teDataIsTextBlock, (Ref)cp, length,
		NULL, NULL, /* no style info */
		handle);

	(**temp).textFlags |= fReadOnly;

	TESetSelection((Pointer)oldStart, (Pointer)oldEnd, handle);

}


enum
{
	STATE_NULL = 0,
	STATE_ESTABLISH,	// waiting to establish
	STATE_QUOTE,		// send the quote...
	STATE_SEND,		// waiting for data to send
	STATE_CLOSE
};

void QServer(void)
{
static srBuff srBuffer;
word delta;
int i;

	delta = false;

	TCPIPPoll();
	for (i = 0; i < QSIZE; i++)
	{
	word ipid;

		ipid = queue[i].ipid;
		if (!ipid) continue;

		TCPIPStatusTCP(ipid, &srBuffer);
		if (_toolErr)
		{
			queue[i].ipid = 0;
			queue[i].state = 0;
			current--;
			delta = true;
			continue;
		}

		if (srBuffer.srState == TCPSCLOSED)
		{
			TCPIPLogout(ipid);
			queue[i].ipid = 0;
			queue[i].state = 0;
			current--;
			delta = true;
			continue;
		}                                  


		switch (queue[i].state)
		{
		case STATE_ESTABLISH:
			if (srBuffer.srState != TCPSESTABLISHED)
				break;

			queue[i].state = STATE_QUOTE;
			// drop through and send the quote.

		case STATE_QUOTE:
			{
			Handle h;
			  h = LoadQuote(MyID, rFile);
			  if (h)
			  {
			    HLock(h);
			    TCPIPWriteTCP(ipid, *h,
				GetHandleSize(h), false, false);
			    DisposeHandle(h);
			  }
			  else
			  {
			    TCPIPWriteTCP(ipid, "Your quote here!\r\n",
				18, false, false);
			  }
			}
			queue[i].state = STATE_SEND;
			break;
			
		case STATE_SEND: 			

			if (srBuffer.srSndQueued == 0)
			{
				TCPIPCloseTCP(ipid);
				queue[i].state = STATE_CLOSE;
			}
			break;
		}
	}


	// check for a new connection.
	if (current < QSIZE)
	{
	word child;
	int i;
		child = TCPIPAcceptTCP(Ipid, 0);
	
		if (!_toolErr) for (i = 0; i < QSIZE; i++)
		{
			if (!queue[i].ipid)
			{
			static char buffer[16];
			static char line[32];
			int j;

				TCPIPStatusTCP(child, &srBuffer);

				queue[i].ipid = child;

				if (srBuffer.srState == TCPSESTABLISHED)
					queue[i].state = STATE_SEND;
				else
					queue[i].state = STATE_ESTABLISH;

				queue[i].tick = GetTick();

				current++;
				total++;
				delta = true;
	
				TCPIPConvertIPToASCII(srBuffer.srDestIP,
					buffer, 0);

				j = ksprintf(line, "%p:%D\r",
				  buffer, srBuffer.srDestPort);
				
				InsertString(j, line);

				break;
			}
		}
	}
	if (delta) fixstats(); // statistics changed.
}


int StartServer(void)
{
int i;
word oFile;
word oDepth;
static char err[256];

	total = current = 0;

	if (!rPath)
        {
          InsertString(32, "Fatal: No quote file specified.\r");
          return false;
        }
	HLock(rPath);

        rFile = OpenResourceFile(readEnable, NULL, (pointer)*rPath);
	if (_toolErr)
	{
          InsertString(ksprintf(err, "Fatal: Unable to open %g\r", *rPath),
            err);
	  return false;
	}

	oFile = GetCurResourceFile();
	SetCurResourceFile(rFile);
	oDepth = SetResourceFileDepth(1);
	rCount = CountResources(rTextForLETextBox2);
	SetCurResourceFile(oFile);
	SetResourceFileDepth(oDepth);

	if (!rCount)
	{
          InsertString(ksprintf(err, "Fatal: Invalid quote file %g\r", *rPath),
            err);

	  CloseResourceFile(rFile);
	  return false;
	}

	SetRandSeed(GetTick());


	for (i = 0; i < QSIZE; i++)
	{
		queue[i].ipid = 0;
		queue[i].state = 0;
	}

	Ipid = TCPIPLogin(MyID, 0, 0, 0, 64);

	TCPIPSetSourcePort(Ipid, PORT_QOTD);

	TCPIPListenTCP(Ipid);

	FlagQS = true;

	HiliteCtlByID(inactiveHilite, MyWindow, CtrlStartQS);
	HiliteCtlByID(noHilite, MyWindow, CtrlStopQS);

	fixstats();
	HUnlock(rPath);

	InsertString(16, "QServer started\r");
	return true;

}

int StopServer(void)
{
int i;


	// close any q entries
	for (i = 0; i < QSIZE; i++)
	{
	int ipid;
		ipid = queue[i].ipid;
		if (ipid)
		{
			TCPIPAbortTCP(ipid);
			TCPIPLogout(ipid);
			queue[i].ipid = 0;
		}
	}

	TCPIPCloseTCP(Ipid);
	TCPIPLogout(Ipid);

	FlagQS = false;
	Ipid = 0;

	HiliteCtlByID(inactiveHilite, MyWindow, CtrlStopQS);
	HiliteCtlByID(noHilite, MyWindow, CtrlStartQS);

	CloseResourceFile(rFile);


    SetInfoRefCon((LongWord)"\pServer stopped", MyWindow);
    DrawInfoBar(MyWindow);

    InsertString(16, "QServer stopped\r");

	return true;
}


// activate/inactivate controls based on Marinetti status
void UpdateStatus(Boolean redraw)
{
  if (FlagTCP) // TCP started
  {
    // deactivate                                     
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStartM);
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStopQS);

    // activate
    HiliteCtlByID(noHilite, MyWindow, CtrlStopM);
    HiliteCtlByID(noHilite, MyWindow, CtrlStartQS);

    SetInfoRefCon((LongWord)"\pNetwork Connected", MyWindow);
  }
  else
  {
    // activate                                     
    HiliteCtlByID(noHilite, MyWindow, CtrlStartM);

    // deactivate
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStopM);
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStartQS);
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStopQS);

    SetInfoRefCon((LongWord)"\pNetwork Disconnected", MyWindow);
  }
  if (redraw) 
    DrawInfoBar(MyWindow);

}





#pragma databank  1

/*
 *  watch for
 */
pascal word HandleRequest(word request, longword dataIn, longword dataOut)
{
Word oldRApp;

  oldRApp = GetCurResourceApp();
  SetCurResourceApp(MyID);

  if (request == TCPIPSaysNetworkUp)
  {
    FlagTCP = true;
    UpdateStatus(true);
  }

  if (request == TCPIPSaysNetworkDown)
  {
    if (FlagQS) StopServer();

    FlagTCP = false;
    Ipid = 0;
    UpdateStatus(true);
  }
  SetCurResourceApp(oldRApp);

}

pascal void MarinettiCallback(char *str)
{
  if (MyWindow)
  {
    SetInfoRefCon((LongWord)str, MyWindow);
    DrawInfoBar(MyWindow);
  }
}
#pragma databank 0

pascal void DrawInfo(void *rect, const char *str, GrafPortPtr w)
{
  if (str)
  {
    SetForeColor(0x00);
    SetBackColor(0x0f);
    MoveTo(8,22);
    DrawString(str);
  }
}

void DrawWindow(void)
{
  DrawControls(GetPort());
}

// returns 1 on success, 0 on error.
Word LoadNDATools(Word MyID)
{
  if (!QDAuxStatus() || _toolErr)
  {
    LoadOneTool(0x12,0);
    if (!_toolErr) QDAuxStartUp();
    if (_toolErr)
    {
      AlertWindow(awCString, NULL,
        (Ref)"24~Unable to start QuickDraw Aux.~^Too Bad");
      return 0;
    }
    FlagQDAux = true;
  }

  if (!FMStatus() || _toolErr)
  {
    Handle h;
    LoadOneTool(0x1b, 0);
    if (!_toolErr) HandleFM = NewHandle(0x0100, MyID, 0xc005, 0);
    if (!_toolErr) FMStartUp(MyID, (Word)*HandleFM);
    if (_toolErr)
    {
      if (HandleFM) DisposeHandle(HandleFM);

      AlertWindow(awCString, NULL,
        (Ref)"24~Unable to start Font Manager.~^Too Bad");
      return 0;
    }
    FlagFM = true;
  }

  if (!TEStatus() || _toolErr)
  {
    LoadOneTool(0x22,0x0);
    if (!_toolErr) HandleTE = NewHandle(0x0100, MyID, 0xc005, 0);
    if (!_toolErr) TEStartUp(MyID, (Word)*HandleTE);
    if (_toolErr)
    {
      if (HandleTE) DisposeHandle(HandleTE);

      AlertWindow(awCString, NULL,
        (Ref)"24~Unable to start Text Edit.~^Too Bad");
      return 0;
    }
    FlagTE = true; 
  }

  if (!SFStatus() || _toolErr)
  {
    LoadOneTool(0x17,0);
    if (!_toolErr) HandleSF = NewHandle(0x0100, MyID, 0xc005, 0);
    if (!_toolErr) SFStartUp(MyID, (Word)*HandleSF);
    if (_toolErr)
    {
      if (HandleSF) DisposeHandle(HandleSF);

      AlertWindow(awCString, NULL,        
        (Ref)"24~Unable to start Standard Filer.~^Too Bad");
      return 0;
    }
    FlagSF = true; 
  }

  if (!TCPIPStatus() || _toolErr)
  {
    LoadOneTool(0x36,0x0200);
    if (!_toolErr) TCPIPStartUp();
    if (_toolErr)
    {
      AlertWindow(awCString, NULL,
        (Ref)"24~Unable to start Marinetti.~^Too Bad");
      return 0;
    }
    FlagLoadTCP = true;
  }
  return 1;
}

void UnloadNDATools(void)
{
  if (FlagLoadTCP && !TCPIPGetConnectStatus())
  {
    TCPIPShutDown();
    UnloadOneTool(0x36);
  }
  if (FlagSF)
  {
    SFShutDown();
    UnloadOneTool(0x17);
    DisposeHandle(HandleSF);
  }
  if (FlagTE)
  {
    TEShutDown();
    UnloadOneTool(0x22);
    DisposeHandle(HandleTE);
  }
  if (FlagFM)
  {
    FMShutDown();
    UnloadOneTool(0x1b);
    DisposeHandle(HandleFM);
  }
  if (FlagQDAux)
  {
    QDAuxShutDown();
    UnloadOneTool(0x12);
  }
}





GrafPortPtr NDAOpen(void)
{
Boolean ok  = true;
const char *err = NULL;

  if (!LoadNDATools(MyID)) return NULL;

  LoadConfig(MyID);

  // Check if Marinetti Active.
  FlagTCP = TCPIPGetConnectStatus();


  if (ok)
  {
    Pointer myPath;
    Word oldLevel;
    Word oldPrefs;
    Word oldRApp; 
    LevelRecGS levelDCB;
    SysPrefsRecGS prefsDCB;
    Handle H;

    // load our resource. -- see TN.iigs #71
    oldRApp = GetCurResourceApp();
    ResourceStartUp(MyID);
    myPath = LGetPathname2(MyID, 1);

    levelDCB.pCount = 2;
    GetLevelGS(&levelDCB);
    oldLevel = levelDCB.level;
    levelDCB.level = 0;
    SetLevelGS(&levelDCB);

    prefsDCB.pCount = 1;
    GetSysPrefsGS(&prefsDCB);
    oldPrefs = prefsDCB.preferences;
    prefsDCB.preferences = (prefsDCB.preferences & 0x1fff) | 0x8000;
    SetSysPrefsGS(&prefsDCB);

    MyRID = OpenResourceFile(readEnable, NULL, myPath);

    //
    MyWindow = NewWindow2(NULL, 0, DrawWindow, NULL,
      refIsResource, rQSWindow, rWindParam1);

    SetInfoDraw(DrawInfo, MyWindow);

    UpdateStatus(false);

    AcceptRequests(ReqName, MyID, &HandleRequest);

    SetSysWindow(MyWindow);
    ShowWindow(MyWindow);
    SelectWindow(MyWindow);

    //
    prefsDCB.preferences = oldPrefs;
    SetSysPrefsGS(&prefsDCB);

    levelDCB.level = oldLevel;
    SetLevelGS(&levelDCB);

    SetCurResourceApp(oldRApp);

    return MyWindow;
  }
  return NULL;

}

void NDAClose(void)
{
    // if running, shut down.

    if (FlagQS) StopServer();

    CloseWindow(MyWindow);
    MyWindow = NULL;
    AcceptRequests(ReqName, MyID, NULL);
    UnloadConfig();
    CloseResourceFile(MyRID);
    ResourceShutDown();
}

void NDAInit(Word code)
{
  if (code)
  {
    MyWindow = NULL;
    FlagTCP = false;
    FlagQS = false;
    FlagQDAux = false;
    FlagFM = false;
    FlagTE = false;
    FlagSF = false;
    HandleFM = NULL;
    HandleTE = NULL;
    HandleSF = NULL;

    FlagLoadTCP = false;

    MyID = MMStartUp();
    Ipid = 0;
    MyRID = 0;
  }
  else
  {
    UnloadNDATools();
  }
}

word NDAAction(void *param, int code)
{
word eventCode;
static EventRecord event = { 0 };
static word counter = 0;

  if (code == runAction)
  {
    if (FlagQS) QServer();
    return 1;
  }

  else if (code == eventAction)
  {
    BlockMove((Pointer)param, (Pointer)&event, 16);
    event.wmTaskMask = 0x001FFFFF;
    eventCode = TaskMasterDA(0, &event);
    switch(eventCode)
    {
    case updateEvt:
      BeginUpdate(MyWindow);
      DrawWindow();
      EndUpdate(MyWindow);
      break;

    case wInControl:
      switch (event.wmTaskData4)
      {
      /* start marinetti */
      case CtrlStartM:
        //
        if (TCPIPGetConnectStatus())
        {
          FlagTCP = true;
          UpdateStatus(true);
        }
        else
        {
          TCPIPConnect(MarinettiCallback);
        }
        break;

      /* stop marinetti */
      case CtrlStopM:
        if (!TCPIPGetConnectStatus())
        {
          FlagTCP = false;
          UpdateStatus(true);
        }
        else
        {
	  if (FlagQS) StopServer();
          // if option key down, force a shutdown.
          TCPIPDisconnect(event.modifiers & optionKey, MarinettiCallback);
        }
        break;

	/* start the server */
	case CtrlStartQS:
	  StartServer();
	  break;

	/* stop the server */
	case CtrlStopQS:
	  StopServer();
	  break;

        case CtrlConfig:
          DoConfig(MyID);
          break;
      }
      // todo - Command-A selects all.
    }
  }
  else if (code == copyAction)
  {
    TECopy(NULL);
    return 1; // yes we handled it.
  }

 return 0;

}
