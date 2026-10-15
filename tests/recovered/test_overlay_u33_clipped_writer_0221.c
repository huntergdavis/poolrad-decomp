#include "poolrad/recovered/overlay_u33_clipped_writer_0221.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void put(uint8_t *ram,size_t at,uint16_t value)
{
    ram[at]=(uint8_t)value;ram[at+1u]=(uint8_t)(value >> 8u);
}
static int signed_value(unsigned value,unsigned bits)
{
    return value<(1u<<(bits-1u)) ? (int)value : (int)value-(int)(1u<<bits);
}
static uint16_t expected_flags(unsigned left,unsigned right,int subtract,unsigned bits)
{
    unsigned i,ones=0,limit=1u<<bits;
    unsigned result=(subtract ? left-right : left+right)&(limit-1u);
    int mathematical=signed_value(left,bits)+(subtract ? -signed_value(right,bits) : signed_value(right,bits));
    uint16_t flags=0x7602u;
    for (i=0;i<8u;++i) ones+=(result >> i)&1u;
    if (subtract ? left<right : left+right>=limit) flags|=1u;
    if (ones%2u==0u) flags|=4u;
    if (subtract ? left%16u<right%16u : left%16u+right%16u>=16u) flags|=0x10u;
    if (result==0u) flags|=0x40u;
    if (result>=limit/2u) flags|=0x80u;
    if (mathematical<-(int)(limit/2u) || mathematical>=(int)(limit/2u)) flags|=0x800u;
    return flags;
}
int main(void)
{
    static const unsigned offsets[8][2]={{0,0},{1,1},{49,24},{127,127},
        {128,128},{255,255},{0,255},{255,0}};
    uint8_t *ram=malloc(PR_U33_0221_RAM_SIZE);
    unsigned a,b,k,cases=0,paths[5]={0},glyphs[256]={0},columns[256]={0},rows[256]={0};
    CHECK(ram!=NULL);memset(ram,0xA5,PR_U33_0221_RAM_SIZE);
    put(ram,0x16716,0x100);put(ram,0x16718,0x4000);
    for (a=0;a<256u;++a) for (b=0;b<256u;++b) for (k=0;k<8u;++k) {
        int sa=signed_value(a,8),sb=signed_value(b,8);
        unsigned dx=offsets[k][0],dy=offsets[k][1],glyph=(a+b+k)&255u;
        unsigned column=(unsigned)(5*sa+6*sb+21+(int)dx)&255u;
        unsigned row=(unsigned)(5*sa+10+(int)dy)&255u;
        unsigned path=column>=128u?0u:column>49u?1u:row>=128u?2u:row>24u?3u:4u;
        uint16_t cell=(uint16_t)(0x100+50*signed_value(row,8)+signed_value(column,8)+7);
        pr_u33_0221_cpu cpu={0x1234,0x3456,0x5678,0x789A,0x9ABC,
            0xBCDE,0x7777,0x8000,0x3000,0x0221,0x1000,0x1111,0x2000,0x7603};
        ram[0x29000]=(uint8_t)a;ram[0x29001]=(uint8_t)b;
        ram[0x40000u+cell]=0xA5;
        put(ram,0x28000,0x7000);put(ram,0x28002,0x3000);
        put(ram,0x28004,0x9002);put(ram,0x28006,(uint16_t)(0xCC00u|glyph));
        put(ram,0x28008,(uint16_t)(0xA500u|dy));put(ram,0x2800A,(uint16_t)(0x5A00u|dx));
        CHECK(pr_u33_clipped_writer_0221(ram,PR_U33_0221_RAM_SIZE,&cpu)==PR_U33_0221_OK);
        CHECK(ram[0x27FFD]==column && ram[0x27FFC]==row);
        CHECK(ram[0x27FFE]==0x77 && ram[0x27FFF]==0x77);
        CHECK(ram[0x29000]==a && ram[0x29001]==b);
        CHECK(cpu.si==(uint16_t)sa && cpu.bp==0x7777 && cpu.sp==0x800C);
        CHECK(cpu.ds==0x1000 && cpu.ss==0x2000 && cpu.cs==0x3000 && cpu.ip==0x7000);
        if (path==4u) {
            unsigned left=(uint16_t)(0x100u+50u*row);
            CHECK(ram[0x40000u+cell]==(uint8_t)(glyph+1u));
            CHECK(cpu.ax==50u*row && cpu.dx==0u && cpu.cx==column);
            CHECK(cpu.bx==(uint16_t)(0x3400u|((glyph+1u)&255u)));
            CHECK(cpu.di==(uint16_t)(left+column) && cpu.es==0x4000);
            CHECK(cpu.flags==expected_flags(left,column,0,16));
            glyphs[glyph]=1u;
        } else {
            unsigned operand=path<2u?column:row;
            unsigned right=path==1u?49u:path==3u?24u:0u;
            CHECK(ram[0x40000u+cell]==0xA5);
            CHECK(cpu.ax==(uint16_t)(5*sa+10+(int)dy) && cpu.dx==dy && cpu.cx==dx);
            CHECK(cpu.bx==0x3456 && cpu.di==0x9002 && cpu.es==0x1111);
            CHECK(cpu.flags==expected_flags(operand,right,1,8));
        }
        ++paths[path];columns[column]=1u;rows[row]=1u;++cases;
    }
    for (a=0;a<5u;++a) CHECK(paths[a]>0u);
    for (a=0;a<256u;++a) CHECK(glyphs[a] && columns[a] && rows[a]);
    {
        pr_u33_0221_cpu cpu={0},before=cpu;
        uint8_t snapshot=ram[0];
        CHECK(pr_u33_clipped_writer_0221(NULL,PR_U33_0221_RAM_SIZE,&cpu)==PR_U33_0221_ARGUMENT);
        CHECK(pr_u33_clipped_writer_0221(ram,0u,&cpu)==PR_U33_0221_ARGUMENT);
        CHECK(pr_u33_clipped_writer_0221(ram,PR_U33_0221_RAM_SIZE,NULL)==PR_U33_0221_ARGUMENT);
        CHECK(memcmp(&before,&cpu,sizeof cpu)==0 && ram[0]==snapshot);
    }
    free(ram);
    printf("u33 +0221: %u independent cases; all clipping exits, coordinate bytes and stored glyphs passed\n",cases);
    return 0;
}
