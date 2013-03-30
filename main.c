#include <stdio.h>
#include "pcap_collect.h"

//----------------------------------------------------------------------------

int main(void)
{
  start_bpf_process(1638, 1 /* stdout */, "wlan0",
                    "host jarowit.net and not port 22");
  return 0;
}

//----------------------------------------------------------------------------
