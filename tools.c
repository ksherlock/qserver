
/* IIgs TN #53:

The greatest conflict between applications and desk accessories, especially
NDAs, is the use of system tool sets. The Apple IIgs Toolbox Reference, Volume
1, defines the minimum collection of tools sets available to an NDA. The Desk
Manager requires that an application start the following tool sets before
calling DeskStartUp:

Tool Locator (#1)
Memory Manager (#2)
Miscellaneous Tools (#3)
QuickDraw II (#4)
Event Manager (#6)
Window Manager (#14)
Menu Manager (#15)
Control Manager (#16)
LineEdit (#20)
Dialog Manager (#21)
Scrap Manager (#22)

NDAs may assume that these tools are all present and running, so they do not
need to check for their presence. NDAs can also use the following tool sets
without special consideration for starting them up:

Desk Manager
Scheduler
Apple Desktop Bus
Integer Math.

 */

#pragma lint -1
#pragma noroot
#pragma optimize -1

#include <gsos.h>
#include <intmath.h>
#include <loader.h>
#include <locator.h>
#include <memory.h>
#include <resources.h>

extern pascal Word GetMasterSCB(void) inline(0x1704, dispatcher);

extern pascal void ListStartUp(void) inline(0x021C, dispatcher);
extern pascal void ListShutDown(void) inline(0x031C, dispatcher);
extern pascal Word ListVersion(void) inline(0x041C, dispatcher);
extern pascal Boolean ListStatus(void) inline(0x061C, dispatcher);

extern pascal void TCPIPStartUp(void) inline(0x0236, dispatcher);
extern pascal void TCPIPShutDown(void) inline(0x0336, dispatcher);
extern pascal Word TCPIPVersion(void) inline(0x0436, dispatcher);
extern pascal Boolean TCPIPStatus(void) inline(0x0636, dispatcher);
extern pascal Long TCPIPLongVersion(void) inline(0x0836, dispatcher);
extern pascal Boolean TCPIPGetConnectStatus(void) inline(0x0936, dispatcher);

extern pascal void SANEStartUp(Word) inline(0x020A, dispatcher);
extern pascal void SANEShutDown(void) inline(0x030A, dispatcher);
extern pascal Word SANEVersion(void) inline(0x040A, dispatcher);
extern pascal Word SANEStatus(void) inline(0x060A, dispatcher);

extern pascal void TEStartUp(Word, Word) inline(0x0222, dispatcher);
extern pascal void TEShutDown(void) inline(0x0322, dispatcher);
extern pascal Word TEVersion(void) inline(0x0422, dispatcher);
extern pascal Word TEStatus(void) inline(0x0622, dispatcher);

extern pascal void PMStartUp(Word, Word) inline(0x0213, dispatcher);
extern pascal void PMShutDown(void) inline(0x0313, dispatcher);
extern pascal Word PMVersion(void) inline(0x0413, dispatcher);
extern pascal Boolean PMStatus(void) inline(0x0613, dispatcher);

extern pascal void FMStartUp(Word, Word) inline(0x021B, dispatcher);
extern pascal void FMShutDown(void) inline(0x031B, dispatcher);
extern pascal Word FMVersion(void) inline(0x041B, dispatcher);
extern pascal Boolean FMStatus(void) inline(0x061B, dispatcher);

extern pascal void QDAuxStartUp(void) inline(0x0212, dispatcher);
extern pascal void QDAuxShutDown(void) inline(0x0312, dispatcher);
extern pascal Word QDAuxVersion(void) inline(0x0412, dispatcher);
extern pascal Boolean QDAuxStatus(void) inline(0x0612, dispatcher);

extern pascal void SFStartUp(Word, Word) inline(0x0217, dispatcher);
extern pascal void SFShutDown(void) inline(0x0317, dispatcher);
extern pascal Word SFVersion(void) inline(0x0417, dispatcher);
extern pascal Boolean SFStatus(void) inline(0x0617, dispatcher);

extern pascal void MCStartUp(Word) inline(0x0226, dispatcher);
extern pascal void MCShutDown(void) inline(0x0326, dispatcher);
extern pascal Word MCVersion(void) inline(0x0426, dispatcher);
extern pascal Boolean MCStatus(void) inline(0x0626, dispatcher);

static char NullString[] = "\p";
static char ok[] = "\pContinue";

static unsigned centerX(void) {
  if (GetMasterSCB() & 0x80)
    return 180; /* (640-280) */
  return 20;    /* 320 - 280 */
}

static void ErrorLoading(unsigned tool) {

  static char msg1[] = "\pThis desk accessory needs Tool000";
  static char msg2[] = "\pin the System:Tools folder.";

  Int2Dec(tool, msg1 + 31, 3, 0);
  msg1[31] |= 0x10; /* ' ' -> '0' */
  TLMountVolume(centerX(), 67, msg1, msg2, ok, ok);
}

void ErrorStarting(unsigned tool) {

  static char msg1[] = "\pError starting Tool000";

  Int2Dec(tool, msg1 + 20, 3, 0);
  msg1[20] |= 0x10; /* ' ' -> '0' */
  TLMountVolume(centerX(), 67, msg1, NullString, ok, ok);
}

unsigned NDAStartUpTools(Word memID, StartStopRecord *ssRef) {

  unsigned i;
  unsigned dp = 0;
  unsigned char *dptr = 0;
  unsigned errors = 0;
  unsigned level = 0;

  ssRef->resFileID = 0;
  ssRef->dPageHandle = 0;

  /*
          todo:
          25 - note synth
          26 - note seq
          29 - ACE
          30 - resource manager
          32 - MIDI
          33 - VOC
          35 - MidiSynth
          37 - Animation

   */
  /* check which ones are already loaded */
  for (i = 0; i < ssRef->numTools; ++i) {
    unsigned tn = ssRef->theTools[i].toolNumber;
    tn &= 0x00ff;
    switch (tn) {
    case 0x0a:
      if (SANEStatus() && !_toolErr)
        tn |= 0x8000;
      else
        dp += 0x0100;
      break;

    case 0x12:
      if (QDAuxStatus() && !_toolErr)
        tn |= 0x8000;
      break;

    case 0x13:
      if (PMStatus() && !_toolErr)
        tn |= 0x8000;
      else
        dp += 0x0200;
      break;

    case 0x17:
      if (SFStatus() && !_toolErr)
        tn |= 0x8000;
      else
        dp += 0x0100;
      break;

    case 0x1b:
      if (FMStatus() && !_toolErr)
        tn |= 0x8000;
      else
        dp += 0x0100;
      break;

    case 0x1c:
      if (ListStatus() && !_toolErr)
        tn |= 0x8000;
      break;

    case 0x22:
      if (TEStatus() && !_toolErr)
        tn |= 0x8000;
      else
        dp += 0x0100;
      break;

#if 0
			case 0x23:
				if (MIDIStatus() && !_toolErr) tn |= 0x8000;
				else dp += 0x0300;
				break;
#endif

    case 0x26:
      if (MCStatus() && !_toolErr)
        tn |= 0x8000;
      break;

    case 0x36:
      if (TCPIPStatus() && !_toolErr)
        tn |= 0x8000;
      break;

    default:
      tn = 0;
    }
    ssRef->theTools[i].toolNumber = tn;
  }

  if (dp) {
    Handle h = NewHandle(dp, memID, 0xc005, 0);
    if (_toolErr)
      return 0;
    dptr = *(unsigned char **)h;
    ssRef->dPageHandle = h;
  }

  for (i = 0; i < ssRef->numTools; ++i) {
    unsigned tn = ssRef->theTools[i].toolNumber;
    unsigned version = ssRef->theTools[i].minVersion;
    if (tn == 0)
      continue;
    if (tn & 0x8000)
      continue;

    LoadOneTool(tn, version);
    if (_toolErr) {
      ErrorLoading(tn);

      tn |= 0x4000;
      ssRef->theTools[i].toolNumber = tn;
      ++errors;
      continue;
    }

    _toolErr = 0;
    switch (tn) {
    case 0x0a:
      SANEStartUp((Word)dptr);
      dptr += 0x0100;
      break;

    case 0x12:
      QDAuxStartUp();
      break;

    case 0x13:
      PMStartUp(memID, (Word)dptr);
      dptr += 0x0200;
      break;

    case 0x17:
      SFStartUp(memID, (Word)dptr);
      dptr += 0x0100;
      break;

    case 0x1b:
      FMStartUp(memID, (Word)dptr);
      dptr += 0x0100;
      break;

    case 0x1c:
      ListStartUp();
      break;

    case 0x22:
      TEStartUp(memID, (Word)dptr);
      dptr += 0x0100;
      break;

#if 0
			case 0x23:
				MIDIStartUp(memID, (Word)dptr);
				dptr += 0x0300;
				break;
#endif

    case 0x26:
      MCStartUp(memID);
      break;

    case 0x36:
      TCPIPStartUp();
      break;
    }
    if (_toolErr) {
      ErrorStarting(tn);
      ++errors;
      tn |= 0x2000;
    } else {
      tn |= 0x1000;
    }
    ssRef->theTools[i].toolNumber = tn;
  }
exit:
  return errors;
}

void NDAShutDownTools(StartStopRecord *ssRef) {
  unsigned i;

  for (i = 0; i < ssRef->numTools; ++i) {

    unsigned tn = ssRef->theTools[i].toolNumber;
    if (!(tn & 0x1000))
      continue;
    switch (tn & 0xff) {
    case 0x0a:
      SANEShutDown();
      break;
    case 0x12:
      QDAuxShutDown();
      break;
    case 0x13:
      PMShutDown();
      break;
    case 0x17:
      SFShutDown();
      break;
    case 0x1b:
      FMShutDown();
      break;
    case 0x1c:
      ListShutDown();
      break;
    case 0x22:
      TEShutDown();
      break;
    case 0x26:
      MCShutDown();
      break;
    case 0x36:
      if (TCPIPGetConnectStatus() == 0)
        TCPIPShutDown();
      break;
    }
  }

  if (ssRef->dPageHandle)
    DisposeHandle(ssRef->dPageHandle);
}

typedef struct NDAResourceCookie {
  Word oldPrefs;
  Word oldRApp;
  Word resFileID;
} NDAResourceCookie;

/*
 * Open the resource fork.
 *
 */
Word NDAResourceStartUp(Word memID, Word access, NDAResourceCookie *cookie) {

  Pointer myPath;
  LevelRecGS levelDCB;
  SysPrefsRecGS prefsDCB;
  Word resFileID;
  Word oldLevel;

  cookie->oldPrefs = 0;
  cookie->resFileID = 0;

  cookie->oldRApp = GetCurResourceApp();

  levelDCB.pCount = 2;
  GetLevelGS(&levelDCB);
  oldLevel = levelDCB.level;
  levelDCB.level = 0;
  SetLevelGS(&levelDCB);
  levelDCB.level = oldLevel;

  prefsDCB.pCount = 1;
  GetSysPrefsGS(&prefsDCB);
  cookie->oldPrefs = prefsDCB.preferences;
  prefsDCB.preferences = (prefsDCB.preferences & 0x1fff) | 0x8000;
  SetSysPrefsGS(&prefsDCB);

  ResourceStartUp(memID);
  myPath = LGetPathname2(memID, 1);
  resFileID = OpenResourceFile(access, NULL, myPath);
  if (_toolErr) {
    ResourceShutDown();
    resFileID = 0;
  }
  cookie->resFileID = resFileID;

  SetLevelGS(&levelDCB);
  return resFileID;
}

/*
 * restore previous resource app and system preferences.
 */
void NDAResourceRestore(NDAResourceCookie *cookie) {

  SysPrefsRecGS prefsDCB;

  prefsDCB.pCount = 1;
  prefsDCB.preferences = cookie->oldPrefs;
  SetSysPrefsGS(&prefsDCB);

  SetCurResourceApp(cookie->oldRApp);
}

/*
 * close the resource fork.
 */
void NDAResourceShutDown(NDAResourceCookie *cookie) {
  CloseResourceFile(cookie->resFileID);
  ResourceShutDown();
}
