#pragma noroot
#pragma lint -1
#pragma optimize -1

#include <control.h>
#include <gsos.h>
#include <memory.h>
#include <quickdraw.h>
#include <resources.h>
#include <stdfile.h>
#include <window.h>

#include <string.h>

#include "qserver.h"


extern Handle rPath;

static Word fd;

/*
 * load default values.
 * return 0 on failure, anything else on success.
 */
//
Word LoadConfig(Word MyID)
{
Word t;
Word oFile;
Word oDepth;

static GSString32 filePath = {28, "*:System:Preferences:QServer"};
static GSString32 folderPath = {21, "*:System:Preferences:"};

static FileInfoRecGS InfoDCB = {12, (GSString255Ptr)&filePath};
static CreateRecGS CreateDCB = {4, (GSString255Ptr)&folderPath, 0xe3, 0x0f, 0};
   

// 1 - check if file exists
// 2 - if no, create the folder and file
// 3 - load up the data
// 4 - if data doesn't exist, store a default value.

  fd = 0;

  GetFileInfoGS(&InfoDCB);
  t = _toolErr;
  if (_toolErr == pathNotFound)
  {
    CreateGS(&CreateDCB);
    if (!_toolErr) t = fileNotFound;
  }
  if (t == fileNotFound) // file doesn't exist, create
  {
    CreateResourceFile(0,0x5A,0xe3,(Pointer)&filePath);
    if (_toolErr) return 0;
  }
  else if (t) return 0;

  fd = OpenResourceFile(readWriteEnable, NULL, (Pointer)&filePath);
  if (_toolErr) return 0;

  // make sure we're the only resource file...
  oFile = GetCurResourceFile();
  SetCurResourceFile(fd);
  oDepth = SetResourceFileDepth(1);

  // load the quote path.

  rPath = LoadResource(rC1InputString, 1);
  if (_toolErr)
  {
    static GSString255 defPath = { 34, "*/system/CDEVs/Twilight/QuotesFile"};

    rPath = NewHandle(36, MyID, 0, NULL);
    if (rPath)
    {
      PtrToHand((Pointer)defPath, rPath, 36);
      AddResource(rPath, 0, rC1InputString, 1);
    }
  }

  // restore old resource file...

  SetCurResourceFile(oFile);
  SetResourceFileDepth(oDepth);

  return 1;
}

void UnloadConfig(void)
{
  if (fd) CloseResourceFile(fd);
  fd = 0;
}

/*
 * callback from SFGetFile2.  only allows if resource fork found.
 */
#pragma toolparms 1
Word SFFilter(DirEntryRecGS *file)
{
  if (file->flags & 0x8000) return displaySelect;
  return noSelect;
}
#pragma toolparms 0


static void SetText(WindowPtr win, Word id, void *data)
{
GSString255 * gstr = (GSString255 *)data;
CtlRecHndl CtrlHand;
Rect r;

  CtrlHand = GetCtlHandleFromID(win, id);

  SetCtlMoreFlags(0x1000, CtrlHand);
  r = (**CtrlHand).ctlRect;
  EraseRect(&r);

  SetCtlValue(gstr->length, CtrlHand);
  SetCtlTitle(2 + (Pointer)gstr, (Handle)CtrlHand);
}

void DoConfig(Word MyID)
{
static EventRecord event;

WindowPtr win;
Word control;
Word ok;
GrafPortPtr oldPort;
Handle newPath = NULL;

  oldPort = GetPort();

  memset(&event, 0, sizeof(event));
  event.wmTaskMask = 0x001f0004;

  win = NewWindow2(NULL, NULL, NULL, NULL,
    refIsResource, (long)rConfigWindow, rWindParam1);

  SetPort(win);
  // set the current path text...
  HLock(rPath);
  SetText(win, CtrlPath, *rPath);

  for (ok = true; ok;)
  {
    control = (Word)DoModalWindow(&event, NULL, NULL, NULL, 0x0008);

    switch(control)
    {
    case CtrlCancel:
    case CtrlOk:
      ok = false;
      break;

    case CtrlBrowse:
      {
      SFReplyRec2 myReply;

        myReply.nameRefDesc = refIsNewHandle;
        myReply.pathRefDesc = refIsNewHandle;

        SFGetFile2(
          10, 35,
          refIsPointer,
          (Ref)"\pSelect quotation file.",
          (WordProcPtr)SFFilter,           /* filter proc */
          NULL,           /* type list   */
          &myReply);
        if (myReply.good)
        {
          Handle h;
          Word size;

          h = (Handle)myReply.pathRef;
          HLock(h);
          size = GetHandleSize(h) - 2;
          BlockMove(*h + 2, *h, size);
          ReAllocHandle(size, MyID, 0x8000, NULL, h);

          SetText(win, CtrlPath, *h);

          if (newPath) DisposeHandle(newPath);
          newPath = h;

          DisposeHandle((Handle)myReply.nameRef);
        }
      }
      break;
    }

  }
  if (control == CtrlOk) // update the config file.
  {
    if (newPath)
    {
      Word size = (Word)GetHandleSize(newPath);

      ReAllocHandle(size, MyID, 0, NULL, rPath);
      HandToHand(newPath, rPath, size);
      MarkResourceChange(true, rC1InputString, 1);
    }
    UpdateResourceFile(fd);
  }

  CloseWindow(win);
  SetPort(oldPort);

  HUnlock(rPath);

}
