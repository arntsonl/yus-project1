del mastermind.o mastermind.sms mastermind.sym
wla-z80 -o mastermind.asm
wlalink -drv linkfile mastermind.sms
wlalink -drs linkfile mastermind.sym