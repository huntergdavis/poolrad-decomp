#include "poolrad/recovered/overlay_u9e_character_pair_115c_cpu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static size_t at(uint16_t s,uint16_t o) {return (((size_t)s<<4u)+o)&0xFFFFFu;}
static void put16(uint8_t *r,uint16_t s,uint16_t o,uint16_t v)
{size_t p=at(s,o);r[p]=(uint8_t)v;r[(p+1u)&0xFFFFFu]=(uint8_t)(v>>8u);}
static pr_u9e_10e4_cpu setup(uint8_t *r,uint8_t primary,uint8_t secondary,int node)
{
 pr_u9e_10e4_cpu c={0};memset(r,0,PR_U9E_10E4_RAM_SIZE);
 c.cs=0x6000;c.ip=0x115C;c.ds=0x1000;c.ss=0x2000;c.sp=0x8000;c.bp=0x7777;c.flags=0x202;
 put16(r,c.ss,c.sp,0x9000);put16(r,c.ss,(uint16_t)(c.sp+2),0x8000);
 put16(r,c.ss,(uint16_t)(c.sp+4),0x100);put16(r,c.ss,(uint16_t)(c.sp+6),0x3000);
 put16(r,c.ss,(uint16_t)(c.sp+8),(uint16_t)(0xAB00u|secondary));
 put16(r,c.ss,(uint16_t)(c.sp+10),(uint16_t)(0xCD00u|primary));
 put16(r,c.ss,(uint16_t)(c.sp+12),0x100);put16(r,c.ss,(uint16_t)(c.sp+14),0x4000);
 r[at(0x4000,0x110)]=18;r[at(0x4000,0x116)]=50;
 if(node){put16(r,0x4000,0x17F,0x300);put16(r,0x4000,0x181,0x3000);r[at(0x3000,0x300)]=12;r[at(0x3000,0x303)]=6;}
 r[at(c.cs,0x1263)]=0x9A;put16(r,c.cs,0x1264,0x0BB8);put16(r,c.cs,0x1266,0x7000);
 return c;
}
static void returned(const pr_u9e_10e4_cpu *c,int changed)
{assert(c->cs==0x8000 && c->ip==0x9000 && c->sp==0x8010 && c->bp==0x7777);assert((uint8_t)c->ax==(uint8_t)changed);}
int main(void)
{
 uint8_t *r=malloc(PR_U9E_10E4_RAM_SIZE),*before=malloc(PR_U9E_10E4_RAM_SIZE);
 pr_u9e_10e4_cpu c,saved;size_t budget;assert(r && before);
 c=setup(r,17,0,0);budget=10000;
 assert(pr_u9e_character_pair_115c(r,PR_U9E_10E4_RAM_SIZE,&c,0x7000,&budget)==PR_RES00BA_OK);
 returned(&c,0);assert(r[at(0x3000,0x100)]==245);assert(r[at(0x4000,0x110)]==18 && r[at(0x4000,0x116)]==50);
 c=setup(r,19,0,0);budget=10000;
 assert(pr_u9e_character_pair_115c(r,PR_U9E_10E4_RAM_SIZE,&c,0x7000,&budget)==PR_RES00BA_OK);
 returned(&c,1);assert(r[at(0x3000,0x100)]==51);assert(r[at(0x4000,0x110)]==19 && r[at(0x4000,0x116)]==0);
 c=setup(r,19,0,1);budget=10000;
 assert(pr_u9e_character_pair_115c(r,PR_U9E_10E4_RAM_SIZE,&c,0x7000,&budget)==PR_RES00BA_OK);
 returned(&c,1);assert(r[at(0x3000,0x100)]==6);assert(r[at(0x3000,0x303)]==179);
 c=setup(r,19,0,0);saved=c;memcpy(before,r,PR_U9E_10E4_RAM_SIZE);budget=0;
 assert(pr_u9e_character_pair_115c(r,PR_U9E_10E4_RAM_SIZE,&c,0x7000,&budget)==PR_RES00BA_LIMIT);
 assert(!memcmp(&c,&saved,sizeof(c)) && !memcmp(r,before,PR_U9E_10E4_RAM_SIZE));
 budget=10000;
 assert(pr_u9e_character_pair_115c(r,PR_U9E_10E4_RAM_SIZE,&c,0x7001,&budget)==PR_RES00BA_ARGUMENT);
 assert(budget==10000 && !memcmp(&c,&saved,sizeof(c)) && !memcmp(r,before,PR_U9E_10E4_RAM_SIZE));
 assert(pr_u9e_character_pair_115c(NULL,PR_U9E_10E4_RAM_SIZE,&c,0x7000,&budget)==PR_RES00BA_ARGUMENT);
 assert(pr_u9e_character_pair_115c(r,0,&c,0x7000,&budget)==PR_RES00BA_ARGUMENT);
 assert(pr_u9e_character_pair_115c(r,PR_U9E_10E4_RAM_SIZE,NULL,0x7000,&budget)==PR_RES00BA_ARGUMENT);
 assert(pr_u9e_character_pair_115c(r,PR_U9E_10E4_RAM_SIZE,&c,0x7000,NULL)==PR_RES00BA_ARGUMENT);
 free(before);free(r);puts("U9E115C synthetic composition/guard cases passed");return 0;
}
