#pragma lint -1
#pragma noroot
#pragma optimize -1

#include <Locator.h>
#include <tcpip.h>

#define FLAG_LOADED 1
#define FLAG_STARTED 2
#define FLAG_CONNECTED 4
#define FLAG_ERROR 0xffff

void stopMarinetti(int flag)
{
  if (flag == FLAG_ERROR) return;
  if (flag == 0) return;
  if (flag & FLAG_CONNECTED)
  {
    TCPIPDisconnect(0, 0);
    if (_toolErr) return;
  }
  if (flag & FLAG_STARTED) TCPIPShutDown();
  if (flag & FLAG_LOADED) UnloadOneTool(0x36);
}



/*
 * do the various steps to start up marinetti.
 */
int startMarinetti(void)
{
int flag = 0;

  TCPIPStatus();
  if (_toolErr)
  {
    LoadOneTool(0x36, 0x0200); //load Marinetti
    if (_toolErr) return FLAG_ERROR;
    flag |= FLAG_LOADED;
  }
  if (!TCPIPStatus())
  {
    TCPIPStartUp();
    flag |= FLAG_STARTED;
  }
  if (!TCPIPGetConnectStatus())
  {
    TCPIPConnect(0);
    if (_toolErr)
    {
      stopMarinetti(flag);
      return FLAG_ERROR;
    }
    flag |= FLAG_CONNECTED;
  }

  return flag;
}













                          
