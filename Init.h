#ifndef _INIT_SYSTEM_HEADER_
#define _INIT_SYSTEM_HEADER_

int BootOS(void);
void __attribute__((__weak__)) Phase0Init(void);
void __attribute__((__weak__)) Phase1Init(void);

#endif
