/* Original U9E +115C: ordered byte-pair replacement and resident recalculation. */
#include "poolrad/recovered/overlay_u9e_character_pair_115c_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry17_110d.h"
#define RAM_MASK 0xFFFFFu
#define COPY_CPU(d,s) do { \
 (d).ax=(s).ax; (d).bx=(s).bx; (d).cx=(s).cx; (d).dx=(s).dx; \
 (d).si=(s).si; (d).di=(s).di; (d).bp=(s).bp; (d).sp=(s).sp; \
 (d).cs=(s).cs; (d).ip=(s).ip; (d).ds=(s).ds; (d).es=(s).es; \
 (d).ss=(s).ss; (d).flags=(s).flags; } while (0)
static size_t phys(uint16_t seg, uint16_t off)
{ return (((size_t)seg << 4u) + off) & RAM_MASK; }
static uint16_t word(const uint8_t *ram, size_t p)
{ return (uint16_t)(ram[p] | ((uint16_t)ram[(p+1u)&RAM_MASK] << 8u)); }
static uint8_t r8(const uint8_t *ram,uint16_t seg,uint16_t off)
{ return ram[phys(seg,off)]; }
static uint16_t r16(const uint8_t *ram,uint16_t seg,uint16_t off)
{ return word(ram,phys(seg,off)); }
static void w8(uint8_t *ram,uint16_t seg,uint16_t off,uint8_t value)
{ ram[phys(seg,off)]=value; }
static void w16(uint8_t *ram,uint16_t seg,uint16_t off,uint16_t value)
{ size_t p=phys(seg,off); ram[p]=(uint8_t)value; ram[(p+1u)&RAM_MASK]=(uint8_t)(value>>8u); }
static void push(uint8_t *ram,pr_u9e_10e4_cpu *c,uint16_t value)
{ c->sp=(uint16_t)(c->sp-2u); w16(ram,c->ss,c->sp,value); }
static uint16_t pop(const uint8_t *ram,pr_u9e_10e4_cpu *c)
{ uint16_t value=r16(ram,c->ss,c->sp); c->sp=(uint16_t)(c->sp+2u); return value; }
static uint16_t parity(uint8_t v)
{ v^=(uint8_t)(v>>4u);v^=(uint8_t)(v>>2u);v^=(uint8_t)(v>>1u);return (v&1u)?0u:4u; }
static uint16_t arithmetic(pr_u9e_10e4_cpu *c,uint16_t a,uint16_t b,unsigned bits,int subtract)
{
 uint32_t mask=bits==8u?0xFFu:0xFFFFu,sign=(mask+1u)>>1u;
 uint32_t wide=subtract?(uint32_t)a-b:(uint32_t)a+b;
 uint16_t r=(uint16_t)(wide&mask),f=parity((uint8_t)r);
 if (subtract?a<b:wide>mask) f|=1u;
 if ((a^b^r)&0x10u) f|=0x10u;
 if (!r) f|=0x40u;
 if (r&sign) f|=0x80u;
 if ((subtract?((a^b)&(a^r)):(~(a^b)&(a^r)))&sign) f|=0x800u;
 c->flags=(uint16_t)((c->flags&~0x08D5u)|f);return r;
}
static uint16_t logic(pr_u9e_10e4_cpu *c,uint16_t value,unsigned bits)
{
 uint16_t r=(uint16_t)(value&(bits==8u?0xFFu:0xFFFFu)),f=parity((uint8_t)r);
 if (!r)f|=0x40u;
 if (r&(bits==8u?0x80u:0x8000u))f|=0x80u;
 c->flags=(uint16_t)((c->flags&~0x08D5u)|f);return r;
}
pr_res00ba_status pr_u9e_character_pair_115c(uint8_t *ram,size_t ram_size,
                                           pr_u9e_10e4_cpu *c,
                                           uint16_t resident_cs,size_t *budget)
{
 if (!ram || !c || !budget || ram_size!=PR_U9E_10E4_RAM_SIZE || c->ip!=0x115Cu)
     return PR_RES00BA_ARGUMENT;
 if (r8(ram,c->cs,0x1263u)!=0x9Au || r16(ram,c->cs,0x1264u)!=0x0BB8u ||
     r16(ram,c->cs,0x1266u)!=resident_cs) return PR_RES00BA_ARGUMENT;
 for (;;) {
  if (!*budget) return PR_RES00BA_LIMIT;
  --*budget;
  switch (c->ip) {
  case 0x115Cu: /* +115C PUSH */
   push(ram,c,c->bp); c->ip=0x115Du; break;
  case 0x115Du: /* +115D MOV */
   c->bp=(uint16_t)(c->sp); c->ip=0x115Fu; break;
  case 0x115Fu: /* +115F SUB */
   c->sp=(uint16_t)(arithmetic(c,c->sp,0x000Cu,16u,1)); c->ip=0x1162u; break;
  case 0x1162u: /* +1162 LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + 14)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x1165u; break;
  case 0x1165u: /* +1165 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->es,(uint16_t)(c->di + 16)))); c->ip=0x1169u; break;
  case 0x1169u: /* +1169 MOV */
   w8(ram,c->ss,(uint16_t)(c->bp + -10),(uint8_t)(((uint8_t)c->ax))); c->ip=0x116Cu; break;
  case 0x116Cu: /* +116C LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + 14)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x116Fu; break;
  case 0x116Fu: /* +116F MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->es,(uint16_t)(c->di + 22)))); c->ip=0x1173u; break;
  case 0x1173u: /* +1173 MOV */
   w8(ram,c->ss,(uint16_t)(c->bp + -11),(uint8_t)(((uint8_t)c->ax))); c->ip=0x1176u; break;
  case 0x1176u: /* +1176 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + 12)))); c->ip=0x1179u; break;
  case 0x1179u: /* +1179 CMP */
   (void)arithmetic(c,((uint8_t)c->ax),r8(ram,c->ss,(uint16_t)(c->bp + -10)),8u,1); c->ip=0x117Cu; break;
  case 0x117Cu: /* +117C JB */
   c->ip=((c->flags&1u)!=0u)?0x118Cu:0x117Eu; break;
  case 0x117Eu: /* +117E CMP */
   (void)arithmetic(c,r8(ram,c->ss,(uint16_t)(c->bp + 12)),0x0012u,8u,1); c->ip=0x1182u; break;
  case 0x1182u: /* +1182 JNE */
   c->ip=((c->flags&0x40u)==0u)?0x11AAu:0x1184u; break;
  case 0x1184u: /* +1184 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + 10)))); c->ip=0x1187u; break;
  case 0x1187u: /* +1187 CMP */
   (void)arithmetic(c,((uint8_t)c->ax),r8(ram,c->ss,(uint16_t)(c->bp + -11)),8u,1); c->ip=0x118Au; break;
  case 0x118Au: /* +118A JAE */
   c->ip=((c->flags&1u)==0u)?0x11AAu:0x118Cu; break;
  case 0x118Cu: /* +118C MOV */
   w8(ram,c->ss,(uint16_t)(c->bp + -1),(uint8_t)(0x0000u)); c->ip=0x1190u; break;
  case 0x1190u: /* +1190 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + 12)))); c->ip=0x1193u; break;
  case 0x1193u: /* +1193 PUSH */
   push(ram,c,c->ax); c->ip=0x1194u; break;
  case 0x1194u: /* +1194 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + 10)))); c->ip=0x1197u; break;
  case 0x1197u: /* +1197 PUSH */
   push(ram,c,c->ax); c->ip=0x1198u; break;
  case 0x1198u: /* +1198 PUSH */
   push(ram,c,c->cs); c->ip=0x1199u; break;
  case 0x1199u: /* +1199 CALL */
   { uint16_t caller_cs=c->cs; push(ram,c,0x119Cu); c->ip=0x10E4u; (void)pr_u9e_entry16_10e4(ram,ram_size,c); if(c->cs!=caller_cs || c->ip!=0x119Cu)return PR_RES00BA_TRANSFER; } c->ip=0x119Cu; break;
  case 0x119Cu: /* +119C XOR */
   c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)(((uint8_t)(c->ax>>8u)) ^ ((uint8_t)(c->ax>>8u))),8u))<<8u)); c->ip=0x119Eu; break;
  case 0x119Eu: /* +119E ADD */
   c->ax=(uint16_t)(arithmetic(c,c->ax,0x0080u,16u,0)); c->ip=0x11A1u; break;
  case 0x11A1u: /* +11A1 LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + 6)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x11A4u; break;
  case 0x11A4u: /* +11A4 MOV */
   w8(ram,c->es,(uint16_t)(c->di),(uint8_t)(((uint8_t)c->ax))); c->ip=0x11A7u; break;
  case 0x11A7u: /* +11A7 JMP */
   c->ip=0x127Eu; break;
  case 0x11AAu: /* +11AA XOR */
   c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax ^ c->ax),16u)); c->ip=0x11ACu; break;
  case 0x11ACu: /* +11AC MOV */
   w16(ram,c->ss,(uint16_t)(c->bp + -9),(uint16_t)(c->ax)); c->ip=0x11AFu; break;
  case 0x11AFu: /* +11AF MOV */
   w16(ram,c->ss,(uint16_t)(c->bp + -7),(uint16_t)(c->ax)); c->ip=0x11B2u; break;
  case 0x11B2u: /* +11B2 LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + 14)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x11B5u; break;
  case 0x11B5u: /* +11B5 MOV */
   c->ax=(uint16_t)(r16(ram,c->es,(uint16_t)(c->di + 127))); c->ip=0x11B9u; break;
  case 0x11B9u: /* +11B9 MOV */
   c->dx=(uint16_t)(r16(ram,c->es,(uint16_t)(c->di + 129))); c->ip=0x11BEu; break;
  case 0x11BEu: /* +11BE MOV */
   w16(ram,c->ss,(uint16_t)(c->bp + -5),(uint16_t)(c->ax)); c->ip=0x11C1u; break;
  case 0x11C1u: /* +11C1 MOV */
   w16(ram,c->ss,(uint16_t)(c->bp + -3),(uint16_t)(c->dx)); c->ip=0x11C4u; break;
  case 0x11C4u: /* +11C4 MOV */
   c->ax=(uint16_t)(r16(ram,c->ss,(uint16_t)(c->bp + -5))); c->ip=0x11C7u; break;
  case 0x11C7u: /* +11C7 OR */
   c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax | r16(ram,c->ss,(uint16_t)(c->bp + -3))),16u)); c->ip=0x11CAu; break;
  case 0x11CAu: /* +11CA JE */
   c->ip=((c->flags&0x40u)!=0u)?0x120Fu:0x11CCu; break;
  case 0x11CCu: /* +11CC MOV */
   c->ax=(uint16_t)(r16(ram,c->ss,(uint16_t)(c->bp + -9))); c->ip=0x11CFu; break;
  case 0x11CFu: /* +11CF OR */
   c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax | r16(ram,c->ss,(uint16_t)(c->bp + -7))),16u)); c->ip=0x11D2u; break;
  case 0x11D2u: /* +11D2 JNE */
   c->ip=((c->flags&0x40u)==0u)?0x120Fu:0x11D4u; break;
  case 0x11D4u: /* +11D4 LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + -5)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x11D7u; break;
  case 0x11D7u: /* +11D7 CMP */
   (void)arithmetic(c,r8(ram,c->es,(uint16_t)(c->di)),0x000Cu,8u,1); c->ip=0x11DBu; break;
  case 0x11DBu: /* +11DB JE */
   c->ip=((c->flags&0x40u)!=0u)?0x11E6u:0x11DDu; break;
  case 0x11DDu: /* +11DD LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + -5)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x11E0u; break;
  case 0x11E0u: /* +11E0 CMP */
   (void)arithmetic(c,r8(ram,c->es,(uint16_t)(c->di)),0x0026u,8u,1); c->ip=0x11E4u; break;
  case 0x11E4u: /* +11E4 JNE */
   c->ip=((c->flags&0x40u)==0u)?0x11FCu:0x11E6u; break;
  case 0x11E6u: /* +11E6 LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + -5)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x11E9u; break;
  case 0x11E9u: /* +11E9 CMP */
   (void)arithmetic(c,r8(ram,c->es,(uint16_t)(c->di + 3)),0x0080u,8u,1); c->ip=0x11EEu; break;
  case 0x11EEu: /* +11EE JAE */
   c->ip=((c->flags&1u)==0u)?0x11FCu:0x11F0u; break;
  case 0x11F0u: /* +11F0 MOV */
   c->ax=(uint16_t)(r16(ram,c->ss,(uint16_t)(c->bp + -5))); c->ip=0x11F3u; break;
  case 0x11F3u: /* +11F3 MOV */
   c->dx=(uint16_t)(r16(ram,c->ss,(uint16_t)(c->bp + -3))); c->ip=0x11F6u; break;
  case 0x11F6u: /* +11F6 MOV */
   w16(ram,c->ss,(uint16_t)(c->bp + -9),(uint16_t)(c->ax)); c->ip=0x11F9u; break;
  case 0x11F9u: /* +11F9 MOV */
   w16(ram,c->ss,(uint16_t)(c->bp + -7),(uint16_t)(c->dx)); c->ip=0x11FCu; break;
  case 0x11FCu: /* +11FC LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + -5)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x11FFu; break;
  case 0x11FFu: /* +11FF MOV */
   c->ax=(uint16_t)(r16(ram,c->es,(uint16_t)(c->di + 5))); c->ip=0x1203u; break;
  case 0x1203u: /* +1203 MOV */
   c->dx=(uint16_t)(r16(ram,c->es,(uint16_t)(c->di + 7))); c->ip=0x1207u; break;
  case 0x1207u: /* +1207 MOV */
   w16(ram,c->ss,(uint16_t)(c->bp + -5),(uint16_t)(c->ax)); c->ip=0x120Au; break;
  case 0x120Au: /* +120A MOV */
   w16(ram,c->ss,(uint16_t)(c->bp + -3),(uint16_t)(c->dx)); c->ip=0x120Du; break;
  case 0x120Du: /* +120D JMP */
   c->ip=0x11C4u; break;
  case 0x120Fu: /* +120F MOV */
   c->ax=(uint16_t)(r16(ram,c->ss,(uint16_t)(c->bp + -9))); c->ip=0x1212u; break;
  case 0x1212u: /* +1212 OR */
   c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax | r16(ram,c->ss,(uint16_t)(c->bp + -7))),16u)); c->ip=0x1215u; break;
  case 0x1215u: /* +1215 JE */
   c->ip=((c->flags&0x40u)!=0u)?0x1249u:0x1217u; break;
  case 0x1217u: /* +1217 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + -10)))); c->ip=0x121Au; break;
  case 0x121Au: /* +121A PUSH */
   push(ram,c,c->ax); c->ip=0x121Bu; break;
  case 0x121Bu: /* +121B MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + -11)))); c->ip=0x121Eu; break;
  case 0x121Eu: /* +121E PUSH */
   push(ram,c,c->ax); c->ip=0x121Fu; break;
  case 0x121Fu: /* +121F PUSH */
   push(ram,c,c->cs); c->ip=0x1220u; break;
  case 0x1220u: /* +1220 CALL */
   { uint16_t caller_cs=c->cs; push(ram,c,0x1223u); c->ip=0x10E4u; (void)pr_u9e_entry16_10e4(ram,ram_size,c); if(c->cs!=caller_cs || c->ip!=0x1223u)return PR_RES00BA_TRANSFER; } c->ip=0x1223u; break;
  case 0x1223u: /* +1223 MOV */
   w8(ram,c->ss,(uint16_t)(c->bp + -12),(uint8_t)(((uint8_t)c->ax))); c->ip=0x1226u; break;
  case 0x1226u: /* +1226 PUSH */
   push(ram,c,r16(ram,c->ss,(uint16_t)(c->bp + -7))); c->ip=0x1229u; break;
  case 0x1229u: /* +1229 PUSH */
   push(ram,c,r16(ram,c->ss,(uint16_t)(c->bp + -9))); c->ip=0x122Cu; break;
  case 0x122Cu: /* +122C LEA */
   c->di=(uint16_t)((uint16_t)(c->bp + -10)); c->ip=0x122Fu; break;
  case 0x122Fu: /* +122F PUSH */
   push(ram,c,c->ss); c->ip=0x1230u; break;
  case 0x1230u: /* +1230 PUSH */
   push(ram,c,c->di); c->ip=0x1231u; break;
  case 0x1231u: /* +1231 LEA */
   c->di=(uint16_t)((uint16_t)(c->bp + -11)); c->ip=0x1234u; break;
  case 0x1234u: /* +1234 PUSH */
   push(ram,c,c->ss); c->ip=0x1235u; break;
  case 0x1235u: /* +1235 PUSH */
   push(ram,c,c->di); c->ip=0x1236u; break;
  case 0x1236u: /* +1236 PUSH */
   push(ram,c,c->cs); c->ip=0x1237u; break;
  case 0x1237u: /* +1237 CALL */
   { uint16_t caller_cs=c->cs; push(ram,c,0x123Au); c->ip=0x110Du; pr_u9e_110d_cpu leaf; COPY_CPU(leaf,*c); (void)pr_u9e_entry17_110d(ram,ram_size,&leaf); COPY_CPU(*c,leaf); if(c->cs!=caller_cs || c->ip!=0x123Au)return PR_RES00BA_TRANSFER; } c->ip=0x123Au; break;
  case 0x123Au: /* +123A MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + -12)))); c->ip=0x123Du; break;
  case 0x123Du: /* +123D XOR */
   c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)(((uint8_t)(c->ax>>8u)) ^ ((uint8_t)(c->ax>>8u))),8u))<<8u)); c->ip=0x123Fu; break;
  case 0x123Fu: /* +123F ADD */
   c->ax=(uint16_t)(arithmetic(c,c->ax,0x0080u,16u,0)); c->ip=0x1242u; break;
  case 0x1242u: /* +1242 LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + -9)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x1245u; break;
  case 0x1245u: /* +1245 MOV */
   w8(ram,c->es,(uint16_t)(c->di + 3),(uint8_t)(((uint8_t)c->ax))); c->ip=0x1249u; break;
  case 0x1249u: /* +1249 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + 12)))); c->ip=0x124Cu; break;
  case 0x124Cu: /* +124C LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + 14)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x124Fu; break;
  case 0x124Fu: /* +124F MOV */
   w8(ram,c->es,(uint16_t)(c->di + 16),(uint8_t)(((uint8_t)c->ax))); c->ip=0x1253u; break;
  case 0x1253u: /* +1253 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + 10)))); c->ip=0x1256u; break;
  case 0x1256u: /* +1256 LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + 14)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x1259u; break;
  case 0x1259u: /* +1259 MOV */
   w8(ram,c->es,(uint16_t)(c->di + 22),(uint8_t)(((uint8_t)c->ax))); c->ip=0x125Du; break;
  case 0x125Du: /* +125D PUSH */
   push(ram,c,r16(ram,c->ss,(uint16_t)(c->bp + 16))); c->ip=0x1260u; break;
  case 0x1260u: /* +1260 PUSH */
   push(ram,c,r16(ram,c->ss,(uint16_t)(c->bp + 14))); c->ip=0x1263u; break;
  case 0x1263u: /* +1263 LCALL */
   { uint16_t caller_cs=c->cs; pr_res00ba_status status; push(ram,c,caller_cs); push(ram,c,0x1268u); c->cs=resident_cs; c->ip=0x0BB8u; status=pr_res00ba_0bb8_cpu(ram,ram_size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=caller_cs || c->ip!=0x1268u)return PR_RES00BA_TRANSFER; } c->ip=0x1268u; break;
  case 0x1268u: /* +1268 MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + -10)))); c->ip=0x126Bu; break;
  case 0x126Bu: /* +126B PUSH */
   push(ram,c,c->ax); c->ip=0x126Cu; break;
  case 0x126Cu: /* +126C MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + -11)))); c->ip=0x126Fu; break;
  case 0x126Fu: /* +126F PUSH */
   push(ram,c,c->ax); c->ip=0x1270u; break;
  case 0x1270u: /* +1270 PUSH */
   push(ram,c,c->cs); c->ip=0x1271u; break;
  case 0x1271u: /* +1271 CALL */
   { uint16_t caller_cs=c->cs; push(ram,c,0x1274u); c->ip=0x10E4u; (void)pr_u9e_entry16_10e4(ram,ram_size,c); if(c->cs!=caller_cs || c->ip!=0x1274u)return PR_RES00BA_TRANSFER; } c->ip=0x1274u; break;
  case 0x1274u: /* +1274 LES */
   { size_t p=phys(c->ss,(uint16_t)(c->bp + 6)); uint16_t di=word(ram,p),es=word(ram,(p+2u)&RAM_MASK); c->di=di; c->es=es; } c->ip=0x1277u; break;
  case 0x1277u: /* +1277 MOV */
   w8(ram,c->es,(uint16_t)(c->di),(uint8_t)(((uint8_t)c->ax))); c->ip=0x127Au; break;
  case 0x127Au: /* +127A MOV */
   w8(ram,c->ss,(uint16_t)(c->bp + -1),(uint8_t)(0x0001u)); c->ip=0x127Eu; break;
  case 0x127Eu: /* +127E MOV */
   c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(r8(ram,c->ss,(uint16_t)(c->bp + -1)))); c->ip=0x1281u; break;
  case 0x1281u: /* +1281 MOV */
   c->sp=(uint16_t)(c->bp); c->ip=0x1283u; break;
  case 0x1283u: /* +1283 POP */
   c->bp=(uint16_t)(pop(ram,c)); c->ip=0x1284u; break;
  case 0x1284u: /* +1284 RETF */
   c->ip=pop(ram,c); c->cs=pop(ram,c); c->sp=(uint16_t)(c->sp+0x000Cu); return PR_RES00BA_OK;
  default: return PR_RES00BA_TRANSFER;
  }
 }
}
