/*  Festalon - NSF Player
 *  Copyright (C) 2004 Xodnizel
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <math.h>
#include <stdio.h>
#include "types.h"

#include "sound.h"
#include "x6502.h"
#include "filter.h"
#include "nsf.h"

#include "fcoeffs.h"

static unsigned int SoundVolume;

void FESTAI_SetVolume(uint32 volume)
{
 SoundVolume=volume;
}

static uint32 mrindex;
static uint32 mrratio;

void SexyFilter(int32 *in, int32 *out, int32 count)
{
 static int64 acc1=0,acc2=0;
 int32 mul1,mul2,vmul;

 mul1=(94<<16)/FSettings.SndRate;
 mul2=(24<<16)/FSettings.SndRate;
 vmul=(SoundVolume<<16)*3/4/4/100;

 while(count)
 {    
  int64 ino=(int64)*in*vmul;
  acc1+=((ino-acc1)*mul1)>>16;
  acc2+=((ino-acc1-acc2)*mul2)>>16;
  //printf("%d ",*in);
  *in=0;  
  {   
   int32 t=(acc1-ino+acc2)>>16;
   //if(t>32767 || t<-32768) printf("Flow: %d\n",t);
   if(t>32767) t=32767;
   if(t<-32768) t=-32768;   
   *out=t;
  }  
  in++;  
  out++;  
  count--; 
 }
}

/* Returns number of samples written to out. */
/* leftover is set to the number of samples that need to be copied 
   from the end of in to the beginning of in.
*/

//static uint32 mva=1000;

/* This filtering code assumes that almost all input values stay below 32767.
   Do not adjust the volume in the wlookup tables and the expansion sound
   code to be higher, or you *might* overflow the FIR code.
*/

int32 NeoFilterSound(int32 *in, int32 *out, uint32 inlen, int32 *leftover)
{
	uint32 x;
	uint32 max;
	int32 *outsave=out;
	int32 count=0;

//	for(x=0;x<inlen;x++)
//	{
//	 if(in[x]>mva){ mva=in[x]; printf("%ld\n",in[x]);}
//	}
        max=(inlen-1)<<16;

	if(FSettings.soundq==2)
         for(x=mrindex;x<max;x+=mrratio)
         {
          int32 acc=0,acc2=0;
          unsigned int c;
          int32 *S,*D;
         
          for(c=SQ2NCOEFFS,S=&in[(x>>16)-SQ2NCOEFFS],D=sq2coeffs;c;c--,D++)
          {
           acc+=(S[c]**D)>>6;
           acc2+=(S[1+c]**D)>>6;
          }

          acc=((int64)acc*(65536-(x&65535))+(int64)acc2*(x&65535))>>(16+11);
          *out=acc;
          out++;   
          count++; 
         }
	else
         for(x=mrindex;x<max;x+=mrratio)
         {
          int32 acc=0,acc2=0;
          unsigned int c;
          int32 *S,*D;
 
          for(c=NCOEFFS,S=&in[(x>>16)-NCOEFFS],D=coeffs;c;c--,D++)
 	  {
 	   acc+=(S[c]**D)>>6;
 	   acc2+=(S[1+c]**D)>>6;
 	  }
 
          acc=((int64)acc*(65536-(x&65535))+(int64)acc2*(x&65535))>>(16+11);  
          *out=acc;
          out++;
          count++;
         }
        mrindex=x-max;

	if(FSettings.soundq==2)
	{
         mrindex+=SQ2NCOEFFS*65536;
         *leftover=SQ2NCOEFFS+1;
	}
	else
	{
         mrindex+=NCOEFFS*65536;
         *leftover=NCOEFFS+1;
	}

	if(GameExpSound.NeoFill)
	 GameExpSound.NeoFill(outsave,count);

	SexyFilter(outsave,outsave,count);
	return(count);
}

void MakeFilters(int32 rate)
{
 int32 *tabs[6]={C44100NTSC,C44100PAL,C48000NTSC,C48000PAL,C96000NTSC,
        C96000PAL};
 int32 *sq2tabs[6]={SQ2C44100NTSC,SQ2C44100PAL,SQ2C48000NTSC,SQ2C48000PAL,
	SQ2C96000NTSC,SQ2C96000PAL};

 int32 *tmp;
 int32 x;
 uint32 nco;

 if(FSettings.soundq==2)
  nco=SQ2NCOEFFS;
 else
  nco=NCOEFFS;

 mrindex=(nco+1)<<16;
 mrratio=(PAL?(int64)(PAL_CPU*65536):(int64)(NTSC_CPU*65536))/rate;

 if(FSettings.soundq==2)
  tmp=sq2tabs[(PAL?1:0)|(rate==48000?2:0)|(rate==96000?4:0)];
 else
  tmp=tabs[(PAL?1:0)|(rate==48000?2:0)|(rate==96000?4:0)];

 if(FSettings.soundq==2)
  for(x=0;x<SQ2NCOEFFS>>1;x++)
   sq2coeffs[x]=sq2coeffs[SQ2NCOEFFS-1-x]=tmp[x];
 else
  for(x=0;x<NCOEFFS>>1;x++)
   coeffs[x]=coeffs[NCOEFFS-1-x]=tmp[x];

 #ifdef MOO
 /* Some tests involving precision and error. */
 {
  static int64 acc=0;
  int x;
  for(x=0;x<SQ2NCOEFFS;x++)
   acc+=(int64)32767*sq2coeffs[x];
  printf("Foo: %lld\n",acc);
 }
 #endif
}