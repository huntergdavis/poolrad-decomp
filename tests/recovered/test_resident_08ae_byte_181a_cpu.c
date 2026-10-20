#include "poolrad/recovered/resident_08ae_byte_181a_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(uint8_t *m,size_t a,uint16_t v)
{m[a]=(uint8_t)v;m[(a+1u)&0xFFFFFu]=(uint8_t)(v>>8);}
int main(void)
{
    uint8_t *m=malloc(PR_R08AE_181A_RAM_SIZE); unsigned n, checks=0;
    assert(m);
    for(n=0;n<256u;++n){
        pr_r08ae_181a_cpu c={0x1234,2,3,4,5,6,0xBEEF,0x0211,0x8800,0x181A,0x3000,0x7777,0x5000,0x0ED7};
        memset(m,0xA5,PR_R08AE_181A_RAM_SIZE);
        word(m,0x89820,0x5000);word(m,0x50211,0x1357);word(m,0x50213,0x9000);word(m,0x50215,(uint16_t)(0xAB00u|n));
        assert(pr_r08ae_byte_181a_cpu(m,PR_R08AE_181A_RAM_SIZE,&c)==PR_R08AE_181A_OK);
        assert(c.ax==(uint16_t)(0x1200u|n)); /* write aliases saved AX low byte */
        assert(c.bp==0xBEEF && c.ds==0x3000 && c.flags==0x0ED7);
        assert(c.cs==0x9000 && c.ip==0x1357 && c.sp==0x0217);
        assert(m[0x5020D]==n && m[0x5020E]==0x12 && m[0x50215]==n);
        checks+=5;
    }
    {pr_r08ae_181a_cpu c={0},old; c.ip=0x181A;c.sp=5;old=c;
     assert(pr_r08ae_byte_181a_cpu(m,PR_R08AE_181A_RAM_SIZE,&c)==PR_R08AE_181A_ARGUMENT);
     assert(memcmp(&old,&c,sizeof c)==0);checks+=2;}
    free(m);printf("181A: %u semantic checks passed\n",checks);return 0;
}
