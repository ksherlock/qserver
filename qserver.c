#pragma nda NDAOpen NDAClose NDAAction NDAInit 30 0xffff "--Quote Server\\H**"
#pragma lint -1
#pragma optimize -1

#include <Control.h>
#include <Desk.h>
#include <Event.h>
#include <Font.h>
#include <GSOS.h>
#include <Loader.h>
#include <Locator.h>
#include <Memory.h>
#include <QDAux.h>
#include <Quickdraw.h>
#include <Resources.h>
#include <TCPIP.h>
#include <TextEdit.h>
#include <Types.h>
#include <Window.h>
#include <intmath.h>
#include <misctool.h>
#include <stdfile.h>

#include <stdio.h>

#include "qserver.h"

unsigned NDAStartUpTools(Word memID, StartStopRecord *ssRef);
void NDAShutDownTools(StartStopRecord *ssRef);

typedef struct NDAResourceCookie {
  Word oldPrefs;
  Word oldRApp;
  Word resFileID;
} NDAResourceCookie;

void NDAResourceRestore(NDAResourceCookie *cookie);
void NDAResourceShutDown(NDAResourceCookie *cookie);
Word NDAResourceStartUp(Word memID, Word access, NDAResourceCookie *cookie);

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
Boolean ToolsLoaded;

Word MyID;
Word Ipid;

word rFile;
Handle rPath;
word rCount;

struct qentry {
  Word ipid;
  Word state;
  Longword tick;
};

#define QSIZE 16
struct qentry queue[QSIZE];

word total;
word current;

void fixstats(void) {
  static char stats[16];
  Word i;

  i = sprintf(stats + 1, "%u : %u", current, total);
  stats[0] = i; // pascal string

  SetInfoRefCon((LongWord)stats, MyWindow);
  DrawInfoBar(MyWindow);
}

void InsertString(word length, char *cp) {
  Handle handle;
  TERecord **temp;
  longword oldStart, oldEnd;

  handle = (Handle)GetCtlHandleFromID(MyWindow, CtrlTE);
  temp = (TERecord **)handle;

  (**temp).textFlags &= (~fReadOnly);

  TEGetSelection((pointer)&oldStart, (pointer)&oldEnd, handle);

  TESetSelection((Pointer)-1, (Pointer)-1, handle);
  TEInsert(teDataIsTextBlock, (Ref)cp, length, NULL, NULL, /* no style info */
           handle);

  (**temp).textFlags |= fReadOnly;

  TESetSelection((Pointer)oldStart, (Pointer)oldEnd, handle);
}

enum {
  STATE_NULL = 0,
  STATE_ESTABLISH, // waiting to establish
  STATE_QUOTE,     // send the quote...
  STATE_SEND,      // waiting for data to send
  STATE_CLOSE
};

void QServer(void) {
  static srBuff srBuffer;
  word delta;
  int i;

  delta = false;

  TCPIPPoll();
  for (i = 0; i < QSIZE; i++) {
    word ipid;

    ipid = queue[i].ipid;
    if (!ipid)
      continue;

    TCPIPStatusTCP(ipid, &srBuffer);
    if (_toolErr) {
      queue[i].ipid = 0;
      queue[i].state = 0;
      current--;
      delta = true;
      continue;
    }

    if (srBuffer.srState == TCPSCLOSED) {
      TCPIPLogout(ipid);
      queue[i].ipid = 0;
      queue[i].state = 0;
      current--;
      delta = true;
      continue;
    }

    switch (queue[i].state) {
    case STATE_ESTABLISH:
      if (srBuffer.srState != TCPSESTABLISHED)
        break;

      queue[i].state = STATE_QUOTE;
      // drop through and send the quote.

    case STATE_QUOTE: {
      Handle h;
      h = LoadQuote(MyID, rFile);
      if (h) {
        HLock(h);
        TCPIPWriteTCP(ipid, *h, GetHandleSize(h), false, false);
        DisposeHandle(h);
      } else {
        TCPIPWriteTCP(ipid, "Your quote here!\r\n", 18, false, false);
      }
      queue[i].state = STATE_SEND;
      break;
    }

    case STATE_SEND:

      if (srBuffer.srSndQueued == 0) {
        TCPIPCloseTCP(ipid);
        queue[i].state = STATE_CLOSE;
      }
      break;
    }
  }

  // check for a new connection.
  if (current < QSIZE) {
    word child;
    int i;
    child = TCPIPAcceptTCP(Ipid, 0);

    if (!_toolErr)
      for (i = 0; i < QSIZE; i++) {
        if (!queue[i].ipid) {
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

          TCPIPConvertIPToASCII(srBuffer.srDestIP, buffer, 0);

          j = sprintf(line, "%b:%u\r", buffer, srBuffer.srDestPort);

          InsertString(j, line);

          break;
        }
      }
  }
  if (delta)
    fixstats(); // statistics changed.
}

int StartServer(void) {
  int i;
  word oFile;
  word oDepth;
  static char err[256];
  GSString255 *path;

  total = current = 0;

  if (!rPath) {
    InsertString(32, "Fatal: No quote file specified.\r");
    return false;
  }
  HLock(rPath);
  path = *(GSString255 **)rPath;

  rFile = OpenResourceFile(readEnable, NULL, (pointer)path);
  if (_toolErr) {
    /* todo */
    InsertString(
        sprintf(err, "Fatal: Unable to open %.*s\r", path->length, path->text),
        err);
    return false;
  }

  oFile = GetCurResourceFile();
  SetCurResourceFile(rFile);
  oDepth = SetResourceFileDepth(1);
  rCount = CountResources(rTextForLETextBox2);
  SetCurResourceFile(oFile);
  SetResourceFileDepth(oDepth);

  if (!rCount) {
    /* todo */
    InsertString(sprintf(err, "Fatal: Invalid quote file %.*s\r", path->length,
                         path->text),
                 err);

    CloseResourceFile(rFile);
    return false;
  }

  SetRandSeed(GetTick());

  for (i = 0; i < QSIZE; i++) {
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

int StopServer(void) {
  int i;

  // close any q entries
  for (i = 0; i < QSIZE; i++) {
    int ipid;
    ipid = queue[i].ipid;
    if (ipid) {
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

  SetInfoRefCon((LongWord) "\pServer stopped", MyWindow);
  DrawInfoBar(MyWindow);

  InsertString(16, "QServer stopped\r");

  return true;
}

// activate/inactivate controls based on Marinetti status
void UpdateStatus(Boolean redraw) {
  if (FlagTCP) // TCP started
  {
    // deactivate
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStartM);
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStopQS);

    // activate
    HiliteCtlByID(noHilite, MyWindow, CtrlStopM);
    HiliteCtlByID(noHilite, MyWindow, CtrlStartQS);

    SetInfoRefCon((LongWord) "\pNetwork Connected", MyWindow);
  } else {
    // activate
    HiliteCtlByID(noHilite, MyWindow, CtrlStartM);

    // deactivate
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStopM);
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStartQS);
    HiliteCtlByID(inactiveHilite, MyWindow, CtrlStopQS);

    SetInfoRefCon((LongWord) "\pNetwork Disconnected", MyWindow);
  }
  if (redraw)
    DrawInfoBar(MyWindow);
}

#pragma databank 1

/*
 *  watch for
 */
pascal word HandleRequest(word request, longword dataIn, longword dataOut) {
  Word oldRApp;

  oldRApp = GetCurResourceApp();
  SetCurResourceApp(MyID);

  if (request == TCPIPSaysNetworkUp) {
    FlagTCP = true;
    UpdateStatus(true);
  }

  if (request == TCPIPSaysNetworkDown) {
    if (FlagQS)
      StopServer();

    FlagTCP = false;
    Ipid = 0;
    UpdateStatus(true);
  }
  SetCurResourceApp(oldRApp);
}

pascal void MarinettiCallback(char *str) {
  if (MyWindow) {
    SetInfoRefCon((LongWord)str, MyWindow);
    DrawInfoBar(MyWindow);
  }
}

pascal void DrawInfo(void *rect, const char *str, GrafPortPtr w) {
  if (str) {
    SetForeColor(0x00);
    SetBackColor(0x0f);
    MoveTo(8, 22);
    DrawString(str);
  }
}

void DrawWindow(void) { DrawControls(GetPort()); }

#pragma databank 0

static StartStopRecord ss = {0,
                             0,
                             0,
                             0,
                             4,
                             {
                                 0x12, 0x0000, /* QD Aux */
                                 0x17, 0x0000, /* Std File */
                                 0x1b, 0x0000, /* Font Manager */
                                 0x22, 0x0000, /* Text Edit */
                                 0x36, 0x0300, /* TCP */

                             }

};

static NDAResourceCookie resInfo;

GrafPortPtr NDAOpen(void) {

  MyWindow = NULL;

  if (!ToolsLoaded) {
    if (NDAStartUpTools(MyID, &ss)) {
      NDAShutDownTools(&ss);
      return NULL;
    }
    ToolsLoaded = true;
  }

  // Check if Marinetti Active.
  FlagTCP = TCPIPGetConnectStatus();

  if (NDAResourceStartUp(MyID, readEnable, &resInfo)) {

    LoadConfig(MyID);

    MyWindow = NewWindow2(NULL, 0, DrawWindow, NULL, refIsResource, rQSWindow,
                          rWindParam1);

    SetInfoDraw(DrawInfo, MyWindow);

    UpdateStatus(false);

    AcceptRequests(ReqName, MyID, &HandleRequest);

    SetSysWindow(MyWindow);
    ShowWindow(MyWindow);
    SelectWindow(MyWindow);
  }

  NDAResourceRestore(&resInfo);
  return MyWindow;
}

void NDAClose(void) {
  // if running, shut down.

  if (FlagQS)
    StopServer();

  AcceptRequests(ReqName, MyID, NULL);
  CloseWindow(MyWindow);
  MyWindow = NULL;

  UnloadConfig();
  NDAResourceShutDown(&resInfo);
}

void NDAInit(Word code) {
  if (code) {
    MyWindow = NULL;
    FlagTCP = false;
    FlagQS = false;
    ToolsLoaded = false;

    MyID = MMStartUp();
    Ipid = 0;
  } else {
    if (ToolsLoaded)
      NDAShutDownTools(&ss);
    ToolsLoaded = false;
  }
}

word NDAAction(void *param, int code) {
  word eventCode;
  static EventRecord event = {0};
  static word counter = 0;

  if (code == runAction) {
    if (FlagQS)
      QServer();
    return 1;
  }

  else if (code == eventAction) {
    BlockMove((Pointer)param, (Pointer)&event, 16);
    event.wmTaskMask = 0x001FFFFF;
    eventCode = TaskMasterDA(0, &event);
    switch (eventCode) {
    case updateEvt:
      BeginUpdate(MyWindow);
      DrawWindow();
      EndUpdate(MyWindow);
      break;

    case wInControl:
      switch (event.wmTaskData4) {
      /* start marinetti */
      case CtrlStartM:
        //
        if (TCPIPGetConnectStatus()) {
          FlagTCP = true;
          UpdateStatus(true);
        } else {
          TCPIPConnect(MarinettiCallback);
        }
        break;

      /* stop marinetti */
      case CtrlStopM:
        if (!TCPIPGetConnectStatus()) {
          FlagTCP = false;
          UpdateStatus(true);
        } else {
          if (FlagQS)
            StopServer();
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
  } else if (code == copyAction) {
    TECopy(NULL);
    return 1; // yes we handled it.
  }

  return 0;
}
