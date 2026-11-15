#include "poolrad/replay/slot_a_first_save_compose.h"
#include "poolrad/replay/slot_a_save_epilogue.h"
#include "poolrad/replay/slot_a_save_dispatch_return.h"
#include "poolrad/replay/slot_a_save_runner_jump.h"
#include "poolrad/replay/slot_a_save_runner_loop.h"
#include "poolrad/replay/slot_a_runner_opcode_prefix.h"
#include "poolrad/replay/slot_a_runner_opcode_fetch.h"
#include "poolrad/replay/slot_a_compare_dispatch.h"
#include "poolrad/replay/slot_a_compare_handler_prefix.h"
#include "poolrad/replay/slot_a_compare_loader.h"
#include "poolrad/replay/slot_a_compare_tag_tests.h"
#include "poolrad/replay/slot_a_compare_first_slot.h"
#include "poolrad/replay/slot_a_compare_6dd5_slot.h"
#include "poolrad/replay/slot_a_if_equal_dispatch_prefix.h"
#include "poolrad/replay/slot_a_if_equal_later_dispatch.h"
#include "poolrad/replay/slot_a_if_equal_handler_prefix.h"
#include "poolrad/replay/slot_a_goto_dispatch.h"
#include "poolrad/replay/slot_a_goto_body.h"
#include "poolrad/replay/slot_a_and_dispatch.h"
#include "poolrad/replay/slot_a_and_handler_prefix.h"
#include "poolrad/replay/slot_a_and_loader_call.h"
#include "poolrad/replay/slot_a_goto_dispatch_return.h"
#include "poolrad/replay/slot_a_compare_second_slot.h"
#include "poolrad/replay/slot_a_compare_word_effects.h"
#include "poolrad/replay/slot_a_compare_return.h"
#include "poolrad/replay/slot_a_compare_dispatch_return.h"
#include "poolrad/replay/slot_a_compare_runner_next.h"
#include "poolrad/replay/slot_a_if_dispatch_prefix.h"
#include "poolrad/replay/slot_a_if_later_dispatch.h"
#include "poolrad/replay/slot_a_if_call.h"
#include "poolrad/replay/slot_a_if_handler_prefix.h"
#include "poolrad/replay/slot_a_if_skip_entry.h"
#include "poolrad/replay/slot_a_skip_fetch.h"
#include "poolrad/replay/slot_a_skip_selection.h"
#include "poolrad/replay/slot_a_skip_loader_entry.h"
#include "poolrad/replay/slot_a_skip_loader_body.h"
#include "poolrad/replay/slot_a_skip_return.h"
#include "poolrad/replay/slot_a_if_return.h"
#include "poolrad/replay/slot_a_if_dispatch_return.h"
#include "poolrad/replay/slot_a_dispatch_save_select.h"
#include "poolrad/replay/slot_a_save_call_frame.h"
#include "poolrad/replay/slot_a_save_handler_prefix.h"
#include "poolrad/replay/slot_a_save_loader_call.h"
#include "poolrad/replay/slot_a_save_loader_compose.h"
#include "poolrad/replay/slot_a_save_word_args.h"
#include "poolrad/replay/slot_a_save_word_helper.h"
#include "poolrad/replay/slot_a_save_tag_branch.h"
#include "poolrad/replay/slot_a_save_slot_value.h"
#include "poolrad/replay/slot_a_and_first_value.h"
#include "poolrad/replay/slot_a_and_second_call.h"
#include "poolrad/replay/slot_a_and_second_stub.h"
#include "poolrad/replay/slot_a_and_reader_prefix.h"
#include "poolrad/replay/slot_a_and_reader_bytes.h"
#include "poolrad/replay/slot_a_and_reader_tag.h"
#include "poolrad/replay/slot_a_and_reader_address.h"
#include "poolrad/replay/slot_a_and_loadword_call.h"
#include "poolrad/replay/slot_a_loadword_classifier_call.h"
#include "poolrad/replay/slot_a_loadword_classifier.h"
#include "poolrad/replay/slot_a_loadword_class4.h"
#include "poolrad/replay/slot_a_loadword_index.h"
#include "poolrad/replay/slot_a_synthetic_cell_read.h"
#include "poolrad/replay/slot_a_synthetic_loadword_return.h"
#include "poolrad/replay/slot_a_synthetic_reader_return.h"
#include "poolrad/replay/slot_a_and_return_args.h"
#include "poolrad/replay/slot_a_and_word_helper.h"
#include "poolrad/replay/slot_a_synthetic_and_arithmetic.h"
#include "poolrad/replay/slot_a_and_compare_args.h"
#include "poolrad/replay/slot_a_and_compare_helper.h"
#include "poolrad/replay/slot_a_and_store_args.h"
#include "poolrad/replay/slot_a_and_store_call.h"
#include "poolrad/replay/slot_a_and_store_stub.h"
#include "poolrad/replay/slot_a_store_classifier_call.h"
#include "poolrad/replay/slot_a_save_store_args.h"
#include "poolrad/replay/slot_a_save_49fd_store.h"
#include "poolrad/replay/slot_a_save_49fe_store.h"
#include "poolrad/replay/slot_a_save_6e7d_slot.h"
#include "poolrad/replay/slot_a_synthetic_and_return.h"
#include "poolrad/replay/slot_a_synthetic_dispatch_return.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
static void word(uint8_t *r,size_t p,unsigned v) { r[p]=(uint8_t)v; r[p+1]=(uint8_t)(v>>8); }
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),sc[8]={0,0,0,0x9a,0x25,0,0x34,0x12},ss[5]={0xea,0x36,1,0,0x30},sb[128]={0},wb[36]={0},sa[9]={0},tc[5]={0x9a,0x52,0,0x34,0x12},ts[5]={0xea,0x77,0x0c,0,0x30},tb[405]={0},th[515]={0},cl[91]={0};
    uint8_t jump[2]={0},tail[4]={0},dj[3]={0},dt[2]={0},loop[14]={0},fetch[10]={0},pro[6]={0},branch[4]={0},select[26]={0},compare_call[4]={0},compare_body[10]={0},ldcall[5]={0x9a,0x2a,0,0x34,0x12},ldstub[5]={0xea,0,0,0,0x30},ldbody[310]={0},tagcode[14]={0}; pr_slot_a_compare_load_report ldreport; pr_slot_a_first_save_refs refs={sc,ss,sb,wb,sa,tc,ts,tb,th,cl}; pr_u1c_3171_cpu c={0},before; pr_slot_a_first_save_report report;
    uint8_t cmpcall[8]={0,0,0,0x9a,0x25,0,0x34,0x12}, load[327]={0}, hook[696]={0}, party[102]={0}; pr_compare_slot_refs slotrefs={cmpcall,ss,sb,wb,load,cl,hook,party}; pr_u25_slot_value_status slotstatus;
    uint8_t secondcall[11]={0,0,0,0,0,0,0x9a,0x25,0,0x34,0x12}; pr_compare_slot_refs secondrefs={secondcall,ss,sb,wb,load,cl,hook,party};
    uint8_t wordcall[14]={0,0,0,0,0,0,0,0,0,0x9a,0x70,0,0x34,0x12}, wordstub[5]={0xea,0xeb,0x18,0,0x30}, wordbody[102]={0}, fill[20]={0}; pr_compare_word_refs wordrefs={wordcall,wordstub,wordbody,fill}; pr_u25_18eb_status wordstatus;
    wordbody[15]=0x9a;wordbody[16]=0xa8;wordbody[17]=0x16;wordbody[18]=0;wordbody[19]=0x50;
    uint8_t returncode[4]={0}, compare_jump[3]={0}, compare_tail[2]={0}, ifselect[78]={0}, later[149]={0}, ifcall[4]={0x0e,0xe8,0x2d,0xdb}, ifbody[111]={0}, skipcall[5]={0x9a,0x8e,0,0x34,0x12}, skipstub[5]={0xea,0xa6,0x1e,0,0x30}, skipcode[23]={0}, selectcode[102]={0}, loadercall[7]={0xb0,2,0x50,0x0e,0xe8,0xd6,0xe0}, skiploader[310]={0}, skipjump[3]={0}, skiptail[2]={0}, iftail[2]={0}, ifjump[3]={0}, dispatchtail[2]={0}, saveselect[71]={0}, savecall[4]={0}, saveprefix[9]={0}, sloadcall[5]={0x9a,0x2a,0,0x34,0x12}, wordargs2[8]={0}, wordcall2[5]={0x9a,0x4d,0,0x34,0x12}, wordstub2[5]={0xea,0xaf,6,0,0x30}, savebranch2[10]={0}; pr_u25_0000_status loadstatus; uint8_t gotoselect[4]={0},gotocall[4]={0x0e,0xe8,0xae,0xd1}; pr_u25_store_word_status storestatus; pr_compare_slot_refs source9934={cmpcall,ss,sb,wb,load,cl,hook,party}; pr_compare_slot_refs source_refs={sc,ss,sb,wb,load,cl,hook,party};
    memcpy(select+4,gotocall,4); memcpy(ifselect+4,gotocall,4); memcpy(saveselect+4,gotocall,4);
    pr_u25_06fb_status classstatus;
    uint8_t andtail[4]={0x89,0xec,0x5d,0xcb}, andjump[2]={0xeb,0x76}, anddispatchtail[2]={0x5d,0xcb};
    uint8_t andstoreargs[9]={0};
    uint8_t cmpfar[5]={0x9a,0x70,0,0x34,0x12}; pr_and_compare_refs andcmp={cmpfar,wordstub,wordbody,fill};
    uint8_t compareargs[9]={0};
    uint8_t andmath[30]={0};
    uint8_t andargs[11]={0};
    uint8_t cellcode[10]={0xa0,0xb1,0x6a,0x30,0xe4,0x89,0x46,0xfe,0xeb,5};
    uint8_t loadwordcall[5]={0x50,0x0e,0xe8,0x7b,0x0c};
    uint8_t andprefix[9]={0};
    uint8_t andselect[437],andcall[4]={0x0e,0xe8,0xf8,0xdc};
    uint8_t gotojump[3]={0};
    uint8_t gotobody[29]={0}; pr_goto_refs grefs={gotobody,ldstub,wordstub2,ldbody,wb}; pr_u1c_goto_status gs; pr_u1c_goto_callee gc; int gcs;
    memcpy(gotobody+6,ldcall,5); memcpy(gotobody+19,wordcall2,5);
    memcpy(load+303,cellcode,10);
    tb[97]=0xe9; tb[98]=0x2b; tb[99]=0x01; tb[399]=0x89; tb[400]=0xec; tb[401]=0x5d; tb[402]=0xca; tb[403]=0x04; tb[404]=0;
    tb[93]=0x0e; tb[94]=0xe8; tb[95]=0x9c; tb[96]=0xfd;
    tb[9]=0x0e;tb[10]=0xe8;tb[11]=0x77;tb[12]=0xfa;
    memcpy(sb+86,loadwordcall,5); load[9]=0x0e; load[10]=0xe8; load[11]=0xe2; load[12]=0xf8;
    CHECK(r&&saved); memcpy(r+0x38917,andjump,2); memcpy(r+0x3667a,andtail,4); memcpy(r+0x30cd8,tb+97,3); memcpy(r+0x30e06,tb+399,6); memcpy(r+0x30c84,tb+13,84); memcpy(r+0x30c80,tb+9,4); memcpy(r+0x36675,tc,5); memcpy(r+0x3666c,andstoreargs,9); memcpy(r+0x36667,cmpfar,5); memcpy(r+0x3665e,compareargs,9); memcpy(r+0x36640,andmath,30); memcpy(r+0x3663b,wordcall2,5); r[0x26ab1]=0x95; memcpy(r+0x30f3b,cellcode,10); memcpy(r+0x30e15,load+9,4); memcpy(r+0x3018c,loadwordcall,5); memcpy(r+0x36625,secondcall,11); memcpy(r+0x3661d,sc,8); memcpy(r+0x36618,ldcall,5); memcpy(r+0x3660f,andprefix,9); memcpy(r+0x38913,andcall,4); memcpy(r+0x38766,gotojump,3); r[0x6406b]=0x2f; r[0x6406c]=0; r[0x6406d]=0x7f; r[0x6406e]=1; r[0x6406f]=0x4f; r[0x64070]=0xc0; r[0x64071]=1; r[0x64072]=0x82; r[0x64073]=0x6e; r[0x64074]=0x1b; memcpy(r+0x35914,gotobody,29); memcpy(r+0x38762,gotocall,4); memcpy(r+0x35ac8,wordcall2,5); memcpy(r+0x1238d,wordstub2,5); memcpy(r+0x35abb,sloadcall,5); memcpy(r+0x31f23,loadercall,7); memcpy(r+0x363e1,skipcall,5); memcpy(r+0x123ce,skipstub,5); memcpy(r+0x38841,ifcall,4); memcpy(r+0x359bf,returncode,4); memcpy(r+0x359b1,wordcall,14); memcpy(r+0x123b0,wordstub,5); memcpy(r+0x318eb,wordbody,102); memcpy(r+0x359a6,secondcall,11); memcpy(r+0x3599e,cmpcall,8); word(r,0x249d2,0x100); word(r,0x249d4,0x6000); word(r,0x60292,10); memcpy(r+0x35ad7,sc,8); memcpy(r+0x12365,ss,5); memcpy(r+0x35ae8,tc,5); memcpy(r+0x12392,ts,5);
    c.cs=0x3582; c.ip=0x02b7; c.ds=0x2000; c.ss=0x4000; c.sp=0x8000; c.bp=0x8010; c.ax=0xab00;
    r[0x27006]=0; r[0x27086]=11; word(r,0x4800c,0x6e7d); word(r,0x48010,0x4567); word(r,0x48012,0x2f89); word(r,0x48014,0x3582); word(r,0x48016,0x2222); word(r,0x48018,0x31a8); word(r,0x4801a,0x3582); word(r,0x249d6,0x100); word(r,0x249d8,0x6000); r[0x389c4]=0x0e; r[0x389c5]=0xe8; r[0x389c6]=0x85; r[0x389c7]=0xfd; r[0x389c8]=0xeb; r[0x389c9]=0xd5; { const uint8_t op[]={0xa0,0x7f,0x6f,0xa2,0x7e,0x6f,0xa1,0xed,0x49,0xc4,0x3e,0xde,0x49}; memcpy(r+0x389ad,op,sizeof op); } r[0x26f7f]=9; word(r,0x249ed,0x991a); word(r,0x249de,0x1000); word(r,0x249e0,0x6300); r[0x6401a]=3; r[0x6401b]=1; r[0x6401c]=0xc9; r[0x6401d]=0x49; r[0x6401e]=0; r[0x6401f]=14; r[0x64020]=0x1b; r[0x64021]=9; r[0x64022]=0; r[0x64023]=0; r[0x64024]=2; r[0x64025]=0x7d; r[0x64026]=0x6e; r[0x64027]=9; r[0x64028]=1; r[0x64029]=0x7d; r[0x6402a]=0x6e; r[0x6402b]=1; r[0x6402c]=0xfd; r[0x6402d]=0x49; r[0x6402e]=9; r[0x6402f]=0; r[0x64030]=9; r[0x64031]=1; r[0x64032]=0xfe; r[0x64033]=0x49; r[0x64034]=3; r[0x64035]=1; r[0x64036]=0xd5; r[0x64037]=0x6d; r[0x64038]=0; r[0x64039]=0; r[0x6403a]=0x16; r[0x6403b]=1; r[0x6403c]=1; r[0x6403d]=0x6b; r[0x6403e]=0x99; memcpy(r+0x35950,ldcall,5); memcpy(r+0x1236a,ldstub,5); before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_AT_02CD);
    CHECK(report.slot==PR_U25_SLOT_VALUE_OK && report.store==PR_U25_STORE_WORD_OK && c.cs==0x3582 && c.ip==0x02cd && c.sp==0x8000 && r[0x607fa]==11 && r[0x607fb]==0);
    CHECK(!pr_slot_a_save_epilogue(r,0x100000u,jump,2,tail,4,&c));
      CHECK(c.cs==0x3582 && c.ip==0x2f89 && c.sp==0x8016 && c.bp==0x4567 && r[0x607fa]==11);
    CHECK(!pr_slot_a_save_dispatch_return(r,0x100000u,dj,3,dt,2,&c));
      CHECK(c.cs==0x3582 && c.ip==0x31a8 && c.sp==0x801c && c.bp==0x2222 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x389c8]^=1;
      CHECK(!pr_slot_a_save_runner_jump(&c,r,0x100000u));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x389c8]^=1;
      CHECK(pr_slot_a_save_runner_jump(&c,r,0x100000u));
      CHECK(c.cs==0x3582 && c.ip==0x317f && c.sp==0x801c && r[0x607fa]==11); }
    CHECK(!pr_slot_a_save_runner_loop(r,0x100000u,loop,14,&c));
      CHECK(c.cs==0x3582 && c.ip==0x318d && r[0x607fa]==11);
    CHECK(pr_slot_a_runner_opcode_prefix(r,0x100000u,&c)==0x319a);
      CHECK(c.ip==0x319a && c.ax==0x991a && c.di==0x1000 && c.es==0x6300 && r[0x26f7e]==9 && r[0x607fa]==11);
    CHECK(!pr_slot_a_runner_opcode_fetch(r,0x100000u,fetch,10,&c));
      CHECK(c.ip==0x31a4 && c.di==0xa91a && c.ax==0x9903 && r[0x26f7f]==3 && r[0x607fa]==11);
    CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f58);
      CHECK(c.ip==0x2f58 && c.sp==0x8016 && (uint8_t)c.ax==3 && r[0x607fa]==11);
    CHECK(!pr_slot_a_compare_handler_prefix(r,0x100000u,compare_call,4,compare_body,10,&c));
      CHECK(c.ip==0x0130 && c.sp==0x7e0a && c.bp==0x8010 && (uint8_t)c.ax==2 && r[0x607fa]==11);
    CHECK(pr_slot_a_compare_loader(r,0x100000u,ldcall,ldstub,ldbody,0x5000,2,1,&c,&ldreport)==PR_COMPARE_LOAD_REACHED);
      CHECK(ldreport.loader==PR_U25_0000_OK && c.cs==0x3582 && c.ip==0x0135 && r[0x27006]==1 && r[0x27007]==0 && r[0x27087]==14 && r[0x607fa]==11);
    CHECK(!pr_slot_a_compare_tag_tests(r,0x100000u,tagcode,14,&c));
      CHECK(c.cs==0x3582 && c.ip==0x017e && r[0x607fa]==11);
    CHECK(!pr_slot_a_compare_first_slot(r,0x100000u,&slotrefs,&c,&slotstatus));
      CHECK(slotstatus==PR_U25_SLOT_VALUE_OK && c.cs==0x3582 && c.ip==0x0186 && c.ax==10 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x27007]=1; uint8_t *refused=malloc(0x100000u);
      CHECK(refused); memcpy(refused,r,0x100000u);
      CHECK(pr_slot_a_compare_second_slot(r,0x100000u,&secondrefs,&c,&slotstatus));
      CHECK(!memcmp(&c,&at,sizeof c)&&!memcmp(r,refused,0x100000u)); free(refused); r[0x27007]=0; }
    CHECK(!pr_slot_a_compare_second_slot(r,0x100000u,&secondrefs,&c,&slotstatus));
      CHECK(slotstatus==PR_U25_SLOT_VALUE_OK && c.cs==0x3582 && c.ip==0x0191 && c.ax==14 && r[0x4800e]==10 && r[0x4800f]==0 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; uint8_t *refused=malloc(0x100000u);
      CHECK(refused); r[0x123b0]^=1; memcpy(refused,r,0x100000u);
      CHECK(pr_slot_a_compare_word_effects(r,0x100000u,&wordrefs,&c,&wordstatus));
      CHECK(!memcmp(&c,&at,sizeof c)&&!memcmp(r,refused,0x100000u)); r[0x123b0]^=1; free(refused); }
    CHECK(!pr_slot_a_compare_word_effects(r,0x100000u,&wordrefs,&c,&wordstatus));
      CHECK(wordstatus==PR_U25_18EB_RETURNED && c.cs==0x3582 && c.ip==0x019f && r[0x26f78]==0 && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x26f7b]==0 && r[0x26f7c]==1 && r[0x26f7d]==0 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x359bf]^=1;
      CHECK(pr_slot_a_compare_return(r,0x100000u,returncode,4,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x359bf]^=1; }
    { pr_u1c_3171_cpu altered=c; uint16_t prior=(uint16_t)(r[0x48012]|((unsigned)r[0x48013]<<8)); word(r,0x48012,0x1234);
      CHECK(!pr_slot_a_compare_return(r,0x100000u,returncode,4,&altered));
      CHECK(altered.ip==0x1234); word(r,0x48012,prior); }
    CHECK(!pr_slot_a_compare_return(r,0x100000u,returncode,4,&c));
      CHECK(c.cs==0x3582 && c.ip==0x2f5c && c.sp==0x8016 && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x3877c]^=1;
      CHECK(pr_slot_a_compare_dispatch_return(r,0x100000u,compare_jump,3,compare_tail,2,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x3877c]^=1; }
    CHECK(!pr_slot_a_compare_dispatch_return(r,0x100000u,compare_jump,3,compare_tail,2,&c));
      CHECK(c.cs==0x3582 && c.ip==0x31a8 && c.sp==0x801c && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x389c8]^=1;
      CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_JUMP_REFUSED);
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x389c8]^=1; }
    { pr_u1c_3171_cpu exitcpu=c; r[0x2442e]=1;
      CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&exitcpu)==PR_COMPARE_RUNNER_EXIT);
      CHECK(exitcpu.ip==0x31aa && r[0x607fa]==11); r[0x2442e]=0; }
    CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_FETCHED);
      CHECK(c.ip==0x31a4 && (uint8_t)c.ax==0x1b && r[0x26f7f]==0x1b && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; uint8_t *trial=malloc(0x100000u);
      CHECK(trial); memcpy(trial,r,0x100000u); trial[0x3875e]^=1;
      CHECK(pr_slot_a_if_dispatch_prefix(trial,0x100000u,&at,pro,branch,ifselect)==0x2f3e);
      CHECK(at.ip==0x2f3e && at.sp==0x8016); free(trial); }
    CHECK(pr_slot_a_if_dispatch_prefix(r,0x100000u,&c,pro,branch,ifselect)==0x2f8c);
      CHECK(c.ip==0x2f8c && c.sp==0x8016 && (uint8_t)c.ax==0x1b && (c.flags&0x08d5)==0x0004 && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x387ac]^=1;
      CHECK(pr_slot_a_if_later_dispatch(r,0x100000u,later,149,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x387ac]^=1; }
    CHECK(!pr_slot_a_if_later_dispatch(r,0x100000u,later,149,&c));
      CHECK(c.ip==0x3021 && c.sp==0x8016 && (c.flags&0x08d5)==0x0044 && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; uint8_t *trial=malloc(0x100000u);
      CHECK(trial); memcpy(trial,r,0x100000u); trial[0x38843]^=1;
      CHECK(pr_slot_a_if_call(trial,0x100000u,ifcall,4,&at)==PR_IF_CALL_NEAR_CODE);
      CHECK(at.ip==0x3022 && at.sp==0x8014 && trial[0x48014]==0x82 && trial[0x48015]==0x35); free(trial); }
    CHECK(pr_slot_a_if_call(r,0x100000u,ifcall,4,&c)==PR_IF_CALL_REACHED);
      CHECK(c.ip==0x0b52 && c.sp==0x8012 && r[0x48012]==0x25 && r[0x48013]==0x30 && r[0x48014]==0x82 && r[0x48015]==0x35 && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; uint8_t *trial=malloc(0x100000u);
      CHECK(trial); memcpy(trial,r,0x100000u); trial[0x26f7d]=1;
      CHECK(pr_slot_a_if_handler_prefix(trial,0x100000u,ifbody,111,&at));
      CHECK(!memcmp(&c,&at,sizeof c) && trial[0x249ed]==r[0x249ed]); free(trial); }
    CHECK(!pr_slot_a_if_handler_prefix(r,0x100000u,ifbody,111,&c));
      CHECK(c.ip==0x0bc1 && c.sp==0x8010 && c.bp==0x8010 && r[0x249ed]==0x21 && r[0x249ee]==0x99 && (c.flags&0x08d5)==0x0044 && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x123ce]^=1;
      CHECK(pr_slot_a_if_skip_entry(r,0x100000u,skipcall,skipstub,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x123ce]^=1; }
    CHECK(!pr_slot_a_if_skip_entry(r,0x100000u,skipcall,skipstub,&c));
      CHECK(c.cs==0x3000 && c.ip==0x1ea6 && c.sp==0x800c && r[0x4800c]==0xc6 && r[0x4800d]==0x0b && r[0x4800e]==0x82 && r[0x4800f]==0x35 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x31ea6]^=1;
      CHECK(pr_slot_a_skip_fetch(r,0x100000u,skipcode,23,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x31ea6]^=1; }
    CHECK(!pr_slot_a_skip_fetch(r,0x100000u,skipcode,23,&c));
      CHECK(c.ip==0x1ebd && c.sp==0x800a && c.bp==0x800a && c.ax==0x9909 && c.di==0xa921 && c.es==0x6300 && (c.flags&0x08d5)==0x0084 && r[0x26f7f]==9 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x31ebd]^=1;
      CHECK(pr_slot_a_skip_selection(r,0x100000u,selectcode,102,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x31ebd]^=1; }
    CHECK(!pr_slot_a_skip_selection(r,0x100000u,selectcode,102,&c));
      CHECK(c.ip==0x1f23 && c.ax==0x9909 && (c.flags&0x08d5)==0x0044 && r[0x26f7f]==9 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; uint8_t *trial=malloc(0x100000u);
      CHECK(trial); memcpy(trial,r,0x100000u); trial[0x31f28]^=1;
      CHECK(pr_slot_a_skip_loader_entry(trial,0x100000u,loadercall,7,&at)==PR_SKIP_LOADER_CALL_CODE);
      CHECK(at.ip==0x1f27 && at.sp==0x8006 && at.ax==0x9902 && trial[0x48006]==0 && trial[0x48007]==0x30); free(trial); }
    CHECK(pr_slot_a_skip_loader_entry(r,0x100000u,loadercall,7,&c)==PR_SKIP_LOADER_REACHED);
      CHECK(c.cs==0x3000 && c.ip==0 && c.sp==0x8004 && c.ax==0x9902 && r[0x48004]==0x2a && r[0x48005]==0x1f && r[0x48006]==0 && r[0x48007]==0x30 && r[0x48008]==2 && r[0x48009]==0x99 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x64024]=0x80;
      CHECK(pr_slot_a_skip_loader_body(r,0x100000u,skiploader,310,0x5000,&c,&loadstatus));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x64024]=2; }
    { uint8_t *trial=malloc(0x100000u),*snapshot=malloc(0x100000u); pr_u1c_3171_cpu at=c,before_alias; pr_u25_0000_status sentinel=PR_U25_0000_TRANSFER;
      CHECK(trial&&snapshot); memcpy(trial,r,0x100000u); at.ss=0x1800; at.sp=0x7ffb; trial[0x1ffff]=2; trial[0x20000]=0; before_alias=at; memcpy(snapshot,trial,0x100000u);
      CHECK(pr_slot_a_skip_loader_body(trial,0x100000u,skiploader,310,0x5000,&at,&sentinel));
      CHECK(!memcmp(&at,&before_alias,sizeof at) && !memcmp(trial,snapshot,0x100000u) && sentinel==PR_U25_0000_TRANSFER); free(snapshot); free(trial); }
    { uint8_t *trial=malloc(0x100000u),*snapshot=malloc(0x100000u); pr_u1c_3171_cpu at=c,before_alias; pr_u25_0000_status sentinel=PR_U25_0000_TRANSFER;
      CHECK(trial&&snapshot); memcpy(trial,r,0x100000u); at.ss=0x5c02; at.sp=0x7ffd; trial[0x64021]=2; before_alias=at; memcpy(snapshot,trial,0x100000u);
      CHECK(pr_slot_a_skip_loader_body(trial,0x100000u,skiploader,310,0x5000,&at,&sentinel));
      CHECK(!memcmp(&at,&before_alias,sizeof at) && !memcmp(trial,snapshot,0x100000u) && sentinel==PR_U25_0000_TRANSFER); free(snapshot); free(trial); }
    CHECK(!pr_slot_a_skip_loader_body(r,0x100000u,skiploader,310,0x5000,&c,&loadstatus));
      CHECK(loadstatus==PR_U25_0000_OK && c.cs==0x3000 && c.ip==0x1f2a && c.sp==0x800a && r[0x249ed]==0x27 && r[0x249ee]==0x99 && r[0x27006]==0 && r[0x27007]==2 && r[0x27087]==0x7d && r[0x27047]==0x6e && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x31f2a]^=1;
      CHECK(pr_slot_a_skip_return(r,0x100000u,skipjump,3,skiptail,2,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x31f2a]^=1; }
    { pr_u1c_3171_cpu at=c; uint16_t old=(uint16_t)(r[0x4800c]|((unsigned)r[0x4800d]<<8)); word(r,0x4800c,0x1234);
      CHECK(!pr_slot_a_skip_return(r,0x100000u,skipjump,3,skiptail,2,&at));
      CHECK(at.ip==0x1234); word(r,0x4800c,old); }
    CHECK(!pr_slot_a_skip_return(r,0x100000u,skipjump,3,skiptail,2,&c));
      CHECK(c.cs==0x3582 && c.ip==0x0bc6 && c.sp==0x8010 && c.bp==0x8010 && r[0x249ed]==0x27 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x363e6]^=1;
      CHECK(pr_slot_a_if_return(r,0x100000u,iftail,2,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x363e6]^=1; }
    { pr_u1c_3171_cpu at=c; uint16_t old=(uint16_t)(r[0x48012]|((unsigned)r[0x48013]<<8)); word(r,0x48012,0x1234);
      CHECK(!pr_slot_a_if_return(r,0x100000u,iftail,2,&at));
      CHECK(at.ip==0x1234); word(r,0x48012,old); }
    CHECK(!pr_slot_a_if_return(r,0x100000u,iftail,2,&c));
      CHECK(c.cs==0x3582 && c.ip==0x3025 && c.sp==0x8016 && c.bp==0x8016 && r[0x249ed]==0x27 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x38845]^=1;
      CHECK(pr_slot_a_if_dispatch_return(r,0x100000u,ifjump,3,dispatchtail,2,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x38845]^=1; }
    { pr_u1c_3171_cpu at=c; uint16_t old=(uint16_t)(r[0x48018]|((unsigned)r[0x48019]<<8)); word(r,0x48018,0x1234);
      CHECK(!pr_slot_a_if_dispatch_return(r,0x100000u,ifjump,3,dispatchtail,2,&at));
      CHECK(at.ip==0x1234); word(r,0x48018,old); }
    CHECK(!pr_slot_a_if_dispatch_return(r,0x100000u,ifjump,3,dispatchtail,2,&c));
      CHECK(c.cs==0x3582 && c.ip==0x31a8 && c.sp==0x801c && c.bp==0x2222 && r[0x249ed]==0x27 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x389c8]^=1;
      CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_JUMP_REFUSED);
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x389c8]^=1; }
    CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_FETCHED);
      CHECK(c.ip==0x31a4 && (uint8_t)c.ax==9 && r[0x26f7f]==9 && r[0x249ed]==0x27 && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x607fa]==11);
    CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f3e);
      CHECK(c.ip==0x2f3e && c.sp==0x8016 && (uint8_t)c.ax==9);
    { pr_u1c_3171_cpu at=c; r[0x3875e]^=1;
      CHECK(pr_slot_a_dispatch_save_select(r,0x100000u,saveselect,71,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x3875e]^=1; }
    CHECK(!pr_slot_a_dispatch_save_select(r,0x100000u,saveselect,71,&c));
      CHECK(c.ip==0x2f85 && (c.flags&0x08d5)==0x0044);
    CHECK(!pr_slot_a_save_call_frame(r,0x100000u,savecall,4,&c));
      CHECK(c.cs==0x3582 && c.ip==0x0292 && c.sp==0x8012 && r[0x48012]==0x89 && r[0x48013]==0x2f && r[0x48014]==0x82 && r[0x48015]==0x35 && r[0x249ed]==0x27 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x35ab2]^=1;
      CHECK(pr_slot_a_save_handler_prefix(r,0x100000u,saveprefix,9,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x35ab2]^=1; }
    CHECK(!pr_slot_a_save_handler_prefix(r,0x100000u,saveprefix,9,&c));
      CHECK(c.ip==0x029b && c.sp==0x800a && c.bp==0x8010);
    CHECK(!pr_slot_a_save_loader_call(r,0x100000u,sloadcall,5,&c));
      CHECK(c.cs==0x1234 && c.ip==0x002a && c.sp==0x8006);
    CHECK(pr_slot_a_save_loader_compose(r,0x100000u,ldstub,5,ldbody,310,0x5000,2,1,&c,&loadstatus)==PR_SLOT_A_LOAD_REACHED);
      CHECK(loadstatus==PR_U25_0000_OK && c.cs==0x3582 && c.ip==0x02a0 && c.sp==0x800c && r[0x249ed]==0x2e && r[0x249ee]==0x99 && r[0x27006]==1 && r[0x27086]==0x7d && r[0x27046]==0x6e && r[0x27007]==1 && r[0x27087]==0xfd && r[0x27047]==0x49 && r[0x607fa]==11);
    CHECK(!pr_slot_a_save_word_args(r,0x100000u,wordargs2,8,&c));
      CHECK(c.ip==0x02a8 && c.sp==0x8008);
    { pr_u1c_3171_cpu at=c; r[0x1238d]^=1;
      CHECK(pr_slot_a_save_word_helper(r,0x100000u,wordcall2,5,wordstub2,5,wb,36,&c,&loadstatus));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x1238d]^=1; }
    CHECK(!pr_slot_a_save_word_helper(r,0x100000u,wordcall2,5,wordstub2,5,wb,36,&c,&loadstatus));
      CHECK(loadstatus==PR_U25_0000_OK && c.cs==0x3582 && c.ip==0x02ad && c.ax==0x49fd && c.sp==0x800c);
    CHECK(!pr_slot_a_save_tag_branch(r,0x100000u,savebranch2,10,&c));
      CHECK(c.ip==0x02b7 && r[0x4800c]==0xfd && r[0x4800d]==0x49);
    { uint8_t *trial=malloc(0x100000u),*snapshot=malloc(0x100000u); pr_u1c_3171_cpu at=c,before_alias=c; pr_u25_slot_value_status sentinel=PR_U25_SLOT_VALUE_TRANSFER;
      CHECK(trial&&snapshot); memcpy(trial,r,0x100000u); word(trial,0x249d6,0x7912); word(trial,0x249d8,0x4000); memcpy(snapshot,trial,0x100000u);
      CHECK(pr_slot_a_save_6e7d_slot(trial,0x100000u,&source_refs,&at,&sentinel));
      CHECK(!memcmp(&at,&before_alias,sizeof at) && !memcmp(trial,snapshot,0x100000u) && sentinel==PR_U25_SLOT_VALUE_TRANSFER); free(snapshot); free(trial); }
    CHECK(!pr_slot_a_save_6e7d_slot(r,0x100000u,&source_refs,&c,&slotstatus));
      CHECK(slotstatus==PR_U25_SLOT_VALUE_OK && c.cs==0x3582 && c.ip==0x02bf && c.ax==11 && r[0x607fa]==11);
    CHECK(!pr_slot_a_save_store_args(r,0x100000u,sa,9,&c));
      CHECK(c.ip==0x02c8 && c.sp==0x8008);
    { uint8_t *trial=malloc(0x100000u),*snapshot=malloc(0x100000u); pr_u1c_3171_cpu at=c,before_alias=c; pr_u25_store_word_status sentinel=PR_U25_STORE_WORD_TRANSFER;
      CHECK(trial&&snapshot); memcpy(trial,r,0x100000u); word(trial,0x249d4,0x2000); memcpy(snapshot,trial,0x100000u);
      CHECK(pr_slot_a_save_49fd_store(trial,0x100000u,tc,5,ts,5,tb,405,th,515,cl,91,&at,&sentinel));
      CHECK(!memcmp(&at,&before_alias,sizeof at) && !memcmp(trial,snapshot,0x100000u) && sentinel==PR_U25_STORE_WORD_TRANSFER); free(snapshot); free(trial); }
    CHECK(!pr_slot_a_save_49fd_store(r,0x100000u,tc,5,ts,5,tb,405,th,515,cl,91,&c,&storestatus));
      CHECK(storestatus==PR_U25_STORE_WORD_OK && c.cs==0x3582 && c.ip==0x02cd && c.sp==0x800c && r[0x602fa]==11 && r[0x602fb]==0 && r[0x284e8]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x35aed]^=1;
      CHECK(pr_slot_a_save_epilogue(r,0x100000u,jump,2,tail,4,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x35aed]^=1; }
    CHECK(!pr_slot_a_save_epilogue(r,0x100000u,jump,2,tail,4,&c));
      CHECK(c.cs==0x3582 && c.ip==0x2f89 && c.sp==0x8016 && r[0x602fa]==11 && r[0x284e8]==1);
    CHECK(!pr_slot_a_save_dispatch_return(r,0x100000u,dj,3,dt,2,&c));
      CHECK(c.cs==0x3582 && c.ip==0x31a8 && c.sp==0x801c && c.bp==0x2222 && r[0x602fa]==11 && r[0x284e8]==1 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x389c8]^=1;
      CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_JUMP_REFUSED);
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x389c8]^=1; }
    CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_FETCHED);
      CHECK(c.ip==0x31a4 && (uint8_t)c.ax==9 && r[0x26f7f]==9 && r[0x249ed]==0x2e && r[0x249ee]==0x99 && r[0x602fa]==11 && r[0x284e8]==1 && r[0x607fa]==11);
    CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f3e);
      CHECK(c.ip==0x2f3e && c.sp==0x8016 && (uint8_t)c.ax==9);
    CHECK(!pr_slot_a_dispatch_save_select(r,0x100000u,saveselect,71,&c));
      CHECK(c.ip==0x2f85 && (c.flags&0x08d5)==0x0044);
    { pr_u1c_3171_cpu at=c; r[0x387a5]^=1;
      CHECK(pr_slot_a_save_call_frame(r,0x100000u,savecall,4,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x387a5]^=1; }
    CHECK(!pr_slot_a_save_call_frame(r,0x100000u,savecall,4,&c));
      CHECK(c.cs==0x3582 && c.ip==0x0292 && c.sp==0x8012 && r[0x48012]==0x89 && r[0x48013]==0x2f && r[0x48014]==0x82 && r[0x48015]==0x35 && r[0x249ed]==0x2e && r[0x602fa]==11 && r[0x284e8]==1);
    CHECK(!pr_slot_a_save_handler_prefix(r,0x100000u,saveprefix,9,&c));
      CHECK(c.ip==0x029b && c.sp==0x800a);
    CHECK(!pr_slot_a_save_loader_call(r,0x100000u,sloadcall,5,&c));
      CHECK(pr_slot_a_save_loader_compose(r,0x100000u,ldstub,5,ldbody,310,0x5000,2,1,&c,&loadstatus)==PR_SLOT_A_LOAD_REACHED);
      CHECK(loadstatus==PR_U25_0000_OK && c.cs==0x3582 && c.ip==0x02a0 && r[0x249ed]==0x34 && r[0x249ee]==0x99 && r[0x27006]==0 && r[0x27086]==9 && r[0x27007]==1 && r[0x27087]==0xfe && r[0x27047]==0x49);
    CHECK(!pr_slot_a_save_word_args(r,0x100000u,wordargs2,8,&c));
      CHECK(!pr_slot_a_save_word_helper(r,0x100000u,wordcall2,5,wordstub2,5,wb,36,&c,&loadstatus));
      CHECK(loadstatus==PR_U25_0000_OK && c.ip==0x02ad && c.ax==0x49fe);
    CHECK(!pr_slot_a_save_tag_branch(r,0x100000u,savebranch2,10,&c));
      CHECK(!pr_slot_a_save_slot_value(r,0x100000u,sc,8,ss,5,sb,128,wb,36,&c,&slotstatus));
      CHECK(slotstatus==PR_U25_SLOT_VALUE_OK && c.ip==0x02bf && c.ax==9);
    CHECK(!pr_slot_a_save_store_args(r,0x100000u,sa,9,&c));
      CHECK(c.ip==0x02c8);
    { uint8_t *trial=malloc(0x100000u),*snapshot=malloc(0x100000u); pr_u1c_3171_cpu at=c,before_alias=c; pr_u25_store_word_status sentinel=PR_U25_STORE_WORD_TRANSFER;
      CHECK(trial&&snapshot); memcpy(trial,r,0x100000u); word(trial,0x249d4,0x2000); memcpy(snapshot,trial,0x100000u);
      CHECK(pr_slot_a_save_49fe_store(trial,0x100000u,tc,5,ts,5,tb,405,th,515,cl,91,&at,&sentinel));
      CHECK(!memcmp(&at,&before_alias,sizeof at)&&!memcmp(trial,snapshot,0x100000u)&&sentinel==PR_U25_STORE_WORD_TRANSFER); free(snapshot); free(trial); }
    { uint8_t *trial=malloc(0x100000u); pr_u1c_3171_cpu at=c; pr_u25_store_word_status status;
      CHECK(trial); memcpy(trial,r,0x100000u); trial[0x284e8]=0;
      CHECK(!pr_slot_a_save_49fe_store(trial,0x100000u,tc,5,ts,5,tb,405,th,515,cl,91,&at,&status));
      CHECK(status==PR_U25_STORE_WORD_OK && trial[0x284e8]==1 && trial[0x602fc]==9); free(trial); }
    CHECK(!pr_slot_a_save_49fe_store(r,0x100000u,tc,5,ts,5,tb,405,th,515,cl,91,&c,&storestatus));
      CHECK(storestatus==PR_U25_STORE_WORD_OK && c.cs==0x3582 && c.ip==0x02cd && c.sp==0x800c && r[0x602fc]==9 && r[0x602fd]==0 && r[0x284e8]==1 && r[0x602fa]==11 && r[0x607fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x35aed]^=1;
      CHECK(pr_slot_a_save_epilogue(r,0x100000u,jump,2,tail,4,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x35aed]^=1; }
    CHECK(!pr_slot_a_save_epilogue(r,0x100000u,jump,2,tail,4,&c));
      CHECK(c.cs==0x3582 && c.ip==0x2f89 && c.sp==0x8016);
    CHECK(!pr_slot_a_save_dispatch_return(r,0x100000u,dj,3,dt,2,&c));
      CHECK(c.cs==0x3582 && c.ip==0x31a8 && c.sp==0x801c && c.bp==0x2222);
    { pr_u1c_3171_cpu at=c; r[0x389c8]^=1;
      CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_JUMP_REFUSED);
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x389c8]^=1; }
    CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_FETCHED);
      CHECK(c.ip==0x31a4 && (uint8_t)c.ax==3 && r[0x26f7f]==3 && r[0x249ed]==0x34 && r[0x249ee]==0x99 && r[0x602fc]==9 && r[0x602fa]==11 && r[0x284e8]==1 && r[0x607fa]==11);
    CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f58);
      CHECK(c.ip==0x2f58 && c.sp==0x8016);
    { pr_u1c_3171_cpu at=c; r[0x38778]^=1;
      CHECK(pr_slot_a_compare_handler_prefix(r,0x100000u,compare_call,4,compare_body,10,&c));
      CHECK(!memcmp(&c,&at,sizeof c)); r[0x38778]^=1; }
    CHECK(!pr_slot_a_compare_handler_prefix(r,0x100000u,compare_call,4,compare_body,10,&c));
      CHECK(c.ip==0x0130 && c.sp==0x7e0a);
    CHECK(pr_slot_a_compare_loader(r,0x100000u,ldcall,ldstub,ldbody,0x5000,2,1,&c,&ldreport)==PR_COMPARE_LOAD_REACHED);
      CHECK(ldreport.loader==PR_U25_0000_OK && c.cs==0x3582 && c.ip==0x0135 && r[0x249ed]==0x3a && r[0x249ee]==0x99 && r[0x27006]==1 && r[0x27086]==0xd5 && r[0x27046]==0x6d && r[0x27007]==0 && r[0x27087]==0);
    CHECK(!pr_slot_a_compare_tag_tests(r,0x100000u,tagcode,14,&c));
      CHECK(c.ip==0x017e && r[0x602fc]==9 && r[0x602fa]==11 && r[0x284e8]==1);
    { uint8_t *trial=malloc(0x100000u); pr_u1c_3171_cpu at=c; pr_u25_slot_value_status status;
      CHECK(trial); memcpy(trial,r,0x100000u); word(trial,0x606aa,0x1234);
      CHECK(!pr_slot_a_compare_6dd5_slot(trial,0x100000u,&source9934,&at,&status));
      CHECK(status==PR_U25_SLOT_VALUE_OK && at.ip==0x0186 && at.ax==0x1234); free(trial); }
    { uint8_t *trial=malloc(0x100000u),*snapshot=malloc(0x100000u); pr_u1c_3171_cpu at=c,before_alias=c; pr_u25_slot_value_status sentinel=PR_U25_SLOT_VALUE_TRANSFER;
      CHECK(trial&&snapshot); memcpy(trial,r,0x100000u); word(trial,0x249d6,0x7862); word(trial,0x249d8,0x4000); memcpy(snapshot,trial,0x100000u);
      CHECK(pr_slot_a_compare_6dd5_slot(trial,0x100000u,&source9934,&at,&sentinel));
      CHECK(!memcmp(&at,&before_alias,sizeof at)&&!memcmp(trial,snapshot,0x100000u)&&sentinel==PR_U25_SLOT_VALUE_TRANSFER); free(snapshot); free(trial); }
    CHECK(!pr_slot_a_compare_6dd5_slot(r,0x100000u,&source9934,&c,&slotstatus));
      CHECK(slotstatus==PR_U25_SLOT_VALUE_OK && c.cs==0x3582 && c.ip==0x0186 && c.ax==0);
    CHECK(!pr_slot_a_compare_second_slot(r,0x100000u,&secondrefs,&c,&slotstatus));
      CHECK(slotstatus==PR_U25_SLOT_VALUE_OK && c.ip==0x0191 && c.ax==0);
    CHECK(!pr_slot_a_compare_word_effects(r,0x100000u,&wordrefs,&c,&wordstatus));
      CHECK(wordstatus==PR_U25_18EB_RETURNED && c.cs==0x3582 && c.ip==0x019f && r[0x26f78]==1 && r[0x26f79]==0 && r[0x26f7a]==0 && r[0x26f7b]==0 && r[0x26f7c]==1 && r[0x26f7d]==1 && r[0x602fa]==11 && r[0x602fc]==9);
    CHECK(!pr_slot_a_compare_return(r,0x100000u,returncode,4,&c));
      CHECK(c.ip==0x2f5c);
    CHECK(!pr_slot_a_compare_dispatch_return(r,0x100000u,compare_jump,3,compare_tail,2,&c));
      CHECK(c.ip==0x31a8);
    CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_FETCHED);
      CHECK(c.ip==0x31a4 && (uint8_t)c.ax==0x16);
    CHECK(pr_slot_a_if_equal_dispatch_prefix(r,0x100000u,&c,pro,branch,ifselect)==0x2f8c);
      CHECK((c.flags&0x08d5)==0x0010);
    CHECK(!pr_slot_a_if_equal_later_dispatch(r,0x100000u,later,149,&c));
      CHECK(c.ip==0x3021 && (c.flags&0x08d5)==0x0044);
    CHECK(pr_slot_a_if_call(r,0x100000u,ifcall,4,&c)==PR_IF_CALL_REACHED);
      CHECK(c.ip==0x0b52);
    { uint8_t *trial=malloc(0x100000u),*snapshot=malloc(0x100000u); pr_u1c_3171_cpu at=c,before_refusal=c;
      CHECK(trial&&snapshot); memcpy(trial,r,0x100000u); trial[0x26f78]=0; memcpy(snapshot,trial,0x100000u);
      CHECK(pr_slot_a_if_equal_handler_prefix(trial,0x100000u,ifbody,111,&at));
      CHECK(!memcmp(&at,&before_refusal,sizeof at)&&!memcmp(trial,snapshot,0x100000u)); free(snapshot); free(trial); }
    CHECK(!pr_slot_a_if_equal_handler_prefix(r,0x100000u,ifbody,111,&c));
      CHECK(c.ip==0x0bc6 && c.sp==0x8010 && r[0x249ed]==0x3b && r[0x249ee]==0x99 && (c.flags&0x08d5)==0 && r[0x26f78]==1 && r[0x602fa]==11 && r[0x602fc]==9);
    CHECK(!pr_slot_a_if_return(r,0x100000u,iftail,2,&c));
      CHECK(c.ip==0x3025);
    CHECK(!pr_slot_a_if_dispatch_return(r,0x100000u,ifjump,3,dispatchtail,2,&c));
      CHECK(c.ip==0x31a8);
    CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_FETCHED);
      CHECK(c.ip==0x31a4 && (uint8_t)c.ax==1 && r[0x249ed]==0x3b);
    CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f3e);
      CHECK(c.ip==0x2f3e && c.sp==0x8016);
    { uint8_t *trial=malloc(0x100000u); pr_u1c_3171_cpu at=c;
      CHECK(trial); memcpy(trial,r,0x100000u); trial[0x38764]^=1;
      CHECK(pr_slot_a_goto_dispatch(trial,0x100000u,gotoselect,gotocall,&at)==0x2f43);
      CHECK(at.ip==0x2f43 && at.sp==0x8014 && trial[0x48014]==0x82 && trial[0x48015]==0x35); free(trial); }
    CHECK(pr_slot_a_goto_dispatch(r,0x100000u,gotoselect,gotocall,&c)==0x00f4);
      CHECK(c.ip==0x00f4 && c.cs==0x3582 && c.sp==0x8012 && r[0x48012]==0x46 && r[0x48013]==0x2f && r[0x48014]==0x82 && r[0x48015]==0x35 && r[0x602fa]==11 && r[0x602fc]==9);
    { uint8_t *trial=malloc(0x100000u), *snapshot=malloc(0x100000u); pr_u1c_3171_cpu at=c, old=c;
      CHECK(trial&&snapshot); memcpy(trial,r,0x100000u); trial[0x6403c]=0; memcpy(snapshot,trial,0x100000u); gs=PR_U1C_GOTO_TRANSFER; gc=PR_U1C_GOTO_WORD; gcs=99;
      CHECK(pr_slot_a_goto_body(trial,0x100000u,&grefs,0x5000,&at,&gs,&gc,&gcs));
      CHECK(!memcmp(trial,snapshot,0x100000u) && !memcmp(&at,&old,sizeof at) && gs==PR_U1C_GOTO_TRANSFER && gc==PR_U1C_GOTO_WORD && gcs==99); memcpy(trial,r,0x100000u); word(trial,0x48012,0x1234); at=c;
      CHECK(!pr_slot_a_goto_body(trial,0x100000u,&grefs,0x5000,&at,&gs,&gc,&gcs));
      CHECK(gs==PR_U1C_GOTO_OK && at.ip==0x1234); free(snapshot); free(trial); }
    CHECK(!pr_slot_a_goto_body(r,0x100000u,&grefs,0x5000,&c,&gs,&gc,&gcs));
      CHECK(gs==PR_U1C_GOTO_OK && gc==PR_U1C_GOTO_WORD && gcs==0 && c.cs==0x3582 && c.ip==0x2f46 && c.sp==0x8016 && r[0x249ed]==0x6b && r[0x249ee]==0x99 && r[0x602fa]==11 && r[0x602fc]==9);
    { pr_u1c_3171_cpu at=c; r[0x38766]^=1;
      CHECK(pr_slot_a_goto_dispatch_return(r,0x100000u,gotojump,3,dispatchtail,2,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x38766]^=1; }
    { pr_u1c_3171_cpu at=c; uint16_t old=(uint16_t)(r[0x48018]|((unsigned)r[0x48019]<<8)); word(r,0x48018,0x1234);
      CHECK(!pr_slot_a_goto_dispatch_return(r,0x100000u,gotojump,3,dispatchtail,2,&at));
      CHECK(at.ip==0x1234); word(r,0x48018,old); }
    CHECK(!pr_slot_a_goto_dispatch_return(r,0x100000u,gotojump,3,dispatchtail,2,&c));
      CHECK(c.cs==0x3582 && c.ip==0x31a8 && c.sp==0x801c && c.bp==0x2222 && r[0x249ed]==0x6b && r[0x249ee]==0x99);
    CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_FETCHED);
      CHECK(c.cs==0x3582 && c.ip==0x31a4 && (uint8_t)c.ax==0x2f && r[0x26f7e]==1 && r[0x26f7f]==0x2f && r[0x249ed]==0x6b && r[0x249ee]==0x99 && r[0x602fa]==11 && r[0x602fc]==9);
    CHECK(pr_slot_a_compare_dispatch(r,0x100000u,&c,pro,branch,select)==0x2f3e);
      CHECK(c.sp==0x8016 && (uint8_t)c.ax==0x2f); memcpy(andselect,r+0x3875e,437);
    { uint8_t *trial=malloc(0x100000u); pr_u1c_3171_cpu at=c;
      CHECK(trial); memcpy(trial,r,0x100000u); trial[0x38915]^=1;
      CHECK(pr_slot_a_and_dispatch(trial,0x100000u,andselect,andcall,&at)==0x30f4);
      CHECK(at.sp==0x8014 && trial[0x48014]==0x82 && trial[0x48015]==0x35); free(trial); }
    CHECK(pr_slot_a_and_dispatch(r,0x100000u,andselect,andcall,&c)==0x0def);
      CHECK(c.cs==0x3582 && c.sp==0x8012 && r[0x48012]==0xf7 && r[0x48013]==0x30 && r[0x48014]==0x82 && r[0x48015]==0x35 && r[0x249ed]==0x6b && r[0x602fa]==11 && r[0x602fc]==9);
    { pr_u1c_3171_cpu at=c; r[0x3660f]^=1;
      CHECK(pr_slot_a_and_handler_prefix(r,0x100000u,andprefix,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x3660f]^=1; }
    CHECK(!pr_slot_a_and_handler_prefix(r,0x100000u,andprefix,&c));
      CHECK(c.ip==0x0df8 && c.bp==0x8010 && c.sp==0x8006 && (uint8_t)c.ax==3 && r[0x48006]==3 && r[0x48010]==0x16 && r[0x48011]==0x80 && r[0x249ed]==0x6b && r[0x602fa]==11 && r[0x602fc]==9);
    { pr_u1c_3171_cpu at=c; r[0x36618]^=1;
      CHECK(pr_slot_a_and_loader_call(r,0x100000u,ldcall,5,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x36618]^=1; }
    CHECK(!pr_slot_a_and_loader_call(r,0x100000u,ldcall,5,&c));
      CHECK(c.cs==0x1234 && c.ip==0x002a && c.sp==0x8002 && r[0x48002]==0xfd && r[0x48003]==0x0d && r[0x48004]==0x82 && r[0x48005]==0x35 && r[0x48006]==3 && r[0x249ed]==0x6b && r[0x602fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x1236a]^=1;
      CHECK(pr_slot_a_save_loader_compose(r,0x100000u,ldstub,5,ldbody,310,0x5000,3,1,&at,&loadstatus)==PR_SLOT_A_LOAD_REFUSED);
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x1236a]^=1; }
    CHECK(pr_slot_a_save_loader_compose(r,0x100000u,ldstub,5,ldbody,310,0x5000,3,1,&c,&loadstatus)==PR_SLOT_A_LOAD_REACHED);
      CHECK(loadstatus==PR_U25_0000_OK && c.cs==0x3582 && c.ip==0x0dfd && c.sp==0x8008 && r[0x249ed]==0x74 && r[0x249ee]==0x99 && r[0x27006]==0 && r[0x27086]==0x7f && r[0x27007]==1 && r[0x27087]==0x4f && r[0x27047]==0xc0 && r[0x27008]==1 && r[0x27088]==0x82 && r[0x27048]==0x6e && r[0x602fa]==11);
    { pr_u1c_3171_cpu at=c; r[0x27086]^=1;
      CHECK(pr_slot_a_and_first_value(r,0x100000u,sc,8,ss,5,sb,128,wb,36,&at,&slotstatus));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x27086]^=1; }
    CHECK(!pr_slot_a_and_first_value(r,0x100000u,sc,8,ss,5,sb,128,wb,36,&c,&slotstatus));
      CHECK(slotstatus==PR_U25_SLOT_VALUE_OK && c.cs==0x3582 && c.ip==0x0e05 && c.sp==0x8008 && c.ax==0x007f && r[0x249ed]==0x74 && r[0x27007]==1 && r[0x27087]==0x4f && r[0x27047]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x3662b]^=1;
      CHECK(pr_slot_a_and_second_call(r,0x100000u,secondcall,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x3662b]^=1; }
    CHECK(!pr_slot_a_and_second_call(r,0x100000u,secondcall,&c));
      CHECK(c.cs==0x1234 && c.ip==0x25 && c.sp==0x8002 && c.ax==2 && r[0x4800a]==0x7f && r[0x4800b]==0 && r[0x48002]==0x10 && r[0x48003]==0x0e && r[0x48006]==2 && r[0x27007]==1 && r[0x27087]==0x4f && r[0x27047]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x12365]^=1;
      CHECK(pr_slot_a_and_second_stub(r,0x100000u,ss,5,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x12365]^=1; }
    CHECK(!pr_slot_a_and_second_stub(r,0x100000u,ss,5,&c));
      CHECK(c.cs==0x3000 && c.ip==0x0136 && c.sp==0x8002 && r[0x48002]==0x10 && r[0x48003]==0x0e && r[0x48006]==2 && r[0x27087]==0x4f && r[0x27047]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x30136]^=1;
      CHECK(pr_slot_a_and_reader_prefix(r,0x100000u,sb,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x30136]^=1; }
    CHECK(!pr_slot_a_and_reader_prefix(r,0x100000u,sb,&c));
      CHECK(c.cs==0x3000 && c.ip==0x013c && c.bp==0x8000 && c.sp==0x7ff6 && r[0x48000]==0x10 && r[0x48001]==0x80 && r[0x48002]==0x10 && r[0x48003]==0x0e && r[0x27087]==0x4f && r[0x27047]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x3013c]^=1;
      CHECK(pr_slot_a_and_reader_bytes(r,0x100000u,sb+6,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x3013c]^=1; }
    CHECK(!pr_slot_a_and_reader_bytes(r,0x100000u,sb+6,&c));
      CHECK(c.cs==0x3000 && c.ip==0x0156 && c.bp==0x8000 && c.sp==0x7ff6 && c.di==2 && c.ax==0x00c0 && r[0x47ff7]==0x4f && r[0x47ff6]==0xc0 && r[0x27007]==1 && r[0x249ed]==0x74);
    { pr_u1c_3171_cpu at=c; r[0x27007]=0;
      CHECK(pr_slot_a_and_reader_tag(r,0x100000u,sb+32,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x27007]=1; }
    CHECK(!pr_slot_a_and_reader_tag(r,0x100000u,sb+32,&c));
      CHECK(c.cs==0x3000 && c.ip==0x017a && c.ax==1 && c.di==2 && (c.flags&0x08d5)==0x0044 && r[0x47ff7]==0x4f && r[0x47ff6]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x3017a]^=1;
      CHECK(pr_slot_a_and_reader_address(r,0x100000u,sb+68,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x3017a]^=1; }
    CHECK(!pr_slot_a_and_reader_address(r,0x100000u,sb+68,&c));
      CHECK(c.cs==0x3000 && c.ip==0x018c && c.ax==0xc04f && c.dx==0x004f && (uint8_t)c.cx==8 && (c.flags&0x08d5)==0x0080 && r[0x47ff7]==0x4f && r[0x47ff6]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x3018e]^=1;
      CHECK(pr_slot_a_and_loadword_call(r,0x100000u,loadwordcall,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x3018e]^=1; }
    CHECK(!pr_slot_a_and_loadword_call(r,0x100000u,loadwordcall,&c));
      CHECK(c.cs==0x3000 && c.ip==0x0e0c && c.sp==0x7ff0 && c.ax==0xc04f && r[0x47ff0]==0x91 && r[0x47ff1]==0x01 && r[0x47ff2]==0 && r[0x47ff3]==0x30 && r[0x47ff4]==0x4f && r[0x47ff5]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x30e16]^=1;
      CHECK(pr_slot_a_loadword_classifier_call(r,0x100000u,load,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x30e16]^=1; }
    CHECK(!pr_slot_a_loadword_classifier_call(r,0x100000u,load,&c));
      CHECK(c.cs==0x3000 && c.ip==0x06fb && c.bp==0x7fee && c.sp==0x7fe2 && r[0x47fe2]==0x19 && r[0x47fe3]==0x0e && r[0x47fe4]==0 && r[0x47fe5]==0x30 && r[0x47fe6]==0x4f && r[0x47fe7]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x306fb]^=1;
      CHECK(pr_slot_a_loadword_classifier(r,0x100000u,cl,&at,&classstatus));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x306fb]^=1; }
    CHECK(!pr_slot_a_loadword_classifier(r,0x100000u,cl,&c,&classstatus));
      CHECK(classstatus==PR_U25_06FB_RETURNED && c.cs==0x3000 && c.ip==0x0e19 && c.sp==0x7fe8 && c.bp==0x7fee && (uint8_t)c.ax==4 && r[0x47ff4]==0x4f && r[0x47ff5]==0xc0);
    { pr_u1c_3171_cpu at=c; at.ax=(uint16_t)((at.ax&0xff00u)|3u);
      CHECK(pr_slot_a_loadword_class4(r,0x100000u,load+13,&at));
      CHECK(at.ip==0x0e19 && (uint8_t)at.ax==3); }
    CHECK(!pr_slot_a_loadword_class4(r,0x100000u,load+13,&c));
      CHECK(c.cs==0x3000 && c.ip==0x0ea7 && c.bp==0x7fee && c.sp==0x7fe8 && (c.flags&0x08d5)==0x0044 && r[0x47feb]==4 && r[0x47fea]==0 && r[0x47ff4]==0x4f && r[0x47ff5]==0xc0);
    { pr_u1c_3171_cpu at=c; r[0x47ff4]^=1;
      CHECK(pr_slot_a_loadword_index(r,0x100000u,load+155,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); r[0x47ff4]^=1; }
    CHECK(!pr_slot_a_loadword_index(r,0x100000u,load+155,&c));
      CHECK(c.cs==0x3000 && c.ip==0x0f3b && c.ax==4 && (c.flags&0x08d5)==0x0044 && r[0x47ff4]==4 && r[0x47ff5]==0 && r[0x47feb]==4);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_synthetic_cell_read(r,0x100000u,cellcode,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_synthetic_cell_read(r,0x100000u,cellcode,0x95,&c));
      CHECK(c.cs==0x3000 && c.ip==0x0f4a && c.ax==0x0095 && (c.flags&0x08d5)==0x0044 && r[0x47fec]==0x95 && r[0x47fed]==0 && r[0x26ab1]==0x95);
    { pr_u1c_3171_cpu at=c;
      CHECK(pr_slot_a_synthetic_loadword_return(r,0x100000u,load+318,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); }
    { pr_u1c_3171_cpu at=c; word(r,0x47ff0,0x1234);
      CHECK(!pr_slot_a_synthetic_loadword_return(r,0x100000u,load+318,0x95,&at));
      CHECK(at.ip==0x1234); word(r,0x47ff0,0x0191); }
    CHECK(!pr_slot_a_synthetic_loadword_return(r,0x100000u,load+318,0x95,&c));
      CHECK(c.cs==0x3000 && c.ip==0x0191 && c.sp==0x7ff6 && c.bp==0x8000 && c.ax==0x0095 && r[0x26ab1]==0x95);
    { pr_u1c_3171_cpu at=c;
      CHECK(pr_slot_a_synthetic_reader_return(r,0x100000u,sb+91,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at)); }
    { uint8_t *trial=malloc(0x100000u); pr_u1c_3171_cpu at=c;
      CHECK(trial); memcpy(trial,r,0x100000u); word(trial,0x48002,0x1234);
      CHECK(!pr_slot_a_synthetic_reader_return(trial,0x100000u,sb+91,0x95,&at));
      CHECK(at.ip==0x1234); free(trial); }
    CHECK(!pr_slot_a_synthetic_reader_return(r,0x100000u,sb+91,0x95,&c));
      CHECK(c.cs==0x3582 && c.ip==0x0e10 && c.sp==0x8008 && c.bp==0x8010 && c.ax==0x0095 && r[0x47ffe]==0x95 && r[0x47fff]==0);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_and_return_args(r,0x100000u,andargs,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_and_return_args(r,0x100000u,andargs,0x95,&c));
      CHECK(c.cs==0x3582 && c.ip==0x0e1b && c.sp==0x8004 && c.ax==0x006e && r[0x4800c]==0x95 && r[0x4800d]==0 && r[0x48006]==0x82 && r[0x48007]==0 && r[0x48004]==0x6e && r[0x48005]==0);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_and_word_helper(r,0x100000u,wordcall2,5,wordstub2,5,wb,36,0x94,&at,&loadstatus));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_and_word_helper(r,0x100000u,wordcall2,5,wordstub2,5,wb,36,0x95,&c,&loadstatus));
    CHECK(loadstatus==PR_U25_0000_OK && c.cs==0x3582 && c.ip==0x0e20 && c.sp==0x8008 && c.ax==0x6e82 && r[0x4800c]==0x95 && r[0x4800d]==0);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_synthetic_and_arithmetic(r,0x100000u,andmath,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_synthetic_and_arithmetic(r,0x100000u,andmath,0x95,&c));
      CHECK(c.cs==0x3582 && c.ip==0x0e3e && c.ax==0x0015 && (c.flags&0x08d5)==0 && r[0x4800e]==0x82 && r[0x4800f]==0x6e && r[0x48009]==0x15 && r[0x4800c]==0x95);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_and_compare_args(r,0x100000u,compareargs,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_and_compare_args(r,0x100000u,compareargs,0x95,&c));
      CHECK(c.cs==0x3582 && c.ip==0x0e47 && c.sp==0x8004 && c.ax==0x0015 && (c.flags&0x08d5)==0x0044 && r[0x48004]==0x15 && r[0x48005]==0 && r[0x48006]==0 && r[0x48007]==0);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_and_compare_helper(r,0x100000u,&andcmp,0x94,&at,&wordstatus));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_and_compare_helper(r,0x100000u,&andcmp,0x95,&c,&wordstatus));
      CHECK(wordstatus==PR_U25_18EB_RETURNED && c.cs==0x3582 && c.ip==0x0e4c && c.sp==0x8008 && r[0x4800c]==0x95 && r[0x4800e]==0x82 && r[0x4800f]==0x6e && r[0x26f78]==0 && r[0x26f79]==1 && r[0x26f7a]==1 && r[0x26f7b]==0 && r[0x26f7c]==1 && r[0x26f7d]==0);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_and_store_args(r,0x100000u,andstoreargs,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_and_store_args(r,0x100000u,andstoreargs,0x95,&c));
      CHECK(c.cs==0x3582 && c.ip==0x0e55 && c.sp==0x8004 && c.ax==0x0015 && (c.flags&0x08d5)==0x0044 && r[0x48004]==0x15 && r[0x48005]==0 && r[0x48006]==0x82 && r[0x48007]==0x6e);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_and_store_call(r,0x100000u,tc,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_and_store_call(r,0x100000u,tc,0x95,&c));
      CHECK(c.cs==0x1234 && c.ip==0x0052 && c.sp==0x8000 && c.ax==0x0015 && r[0x48000]==0x5a && r[0x48001]==0x0e && r[0x48002]==0x82 && r[0x48003]==0x35 && r[0x48004]==0x15 && r[0x48006]==0x82 && r[0x48007]==0x6e);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_and_store_stub(r,0x100000u,ts,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_and_store_stub(r,0x100000u,ts,0x95,&c));
      CHECK(c.cs==0x3000 && c.ip==0x0c77 && c.sp==0x8000 && c.ax==0x0015 && r[0x48000]==0x5a && r[0x48001]==0x0e && r[0x48004]==0x15 && r[0x48006]==0x82 && r[0x48007]==0x6e);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_store_classifier_call(r,0x100000u,tb,0x94,&at));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    CHECK(!pr_slot_a_store_classifier_call(r,0x100000u,tb,0x95,&c));
      CHECK(c.cs==0x3000 && c.ip==0x06fb && c.bp==0x7ffe && c.sp==0x7ff6 && r[0x47ff6]==0x84 && r[0x47ff7]==0x0c && r[0x47ff8]==0 && r[0x47ff9]==0x30 && r[0x47ffa]==0x82 && r[0x47ffb]==0x6e && r[0x48004]==0x15);
    { pr_u1c_3171_cpu at=c; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); r[0x306fb]^=1; memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_loadword_classifier(r,0x100000u,cl,&at,&classstatus));
      CHECK(!memcmp(&at,&c,sizeof at) && !memcmp(snapshot,r,0x100000u)); r[0x306fb]^=1; free(snapshot); }
    { uint8_t before0=r[0x26e82], before1=r[0x26e83];
      CHECK(!pr_slot_a_loadword_classifier(r,0x100000u,cl,&c,&classstatus));
      CHECK(classstatus==PR_U25_06FB_RETURNED && c.cs==0x3000 && c.ip==0x0c84 && c.sp==0x7ffc && c.bp==0x7ffe && (uint8_t)c.ax==1);
      CHECK(r[0x26e82]==before0 && r[0x26e83]==before1 && r[0x48004]==0x15); }
    { pr_u25_store_word_cpu raw; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(&raw,&c,sizeof raw); memcpy(snapshot,r,0x100000u);
      CHECK(pr_u25_store_class1_hook_entry(r,0x100000u,tb+13,0x94,&raw)==PR_U25_STORE_WORD_ARGUMENT);
      CHECK(!memcmp(&raw,&c,sizeof c) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
    { pr_u25_store_word_cpu raw; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(&raw,&c,sizeof raw); r[0x30c84]^=1; memcpy(snapshot,r,0x100000u);
      CHECK(pr_u25_store_class1_hook_entry(r,0x100000u,tb+13,0x95,&raw)==PR_U25_STORE_WORD_ARGUMENT);
      CHECK(!memcmp(&raw,&c,sizeof c) && !memcmp(snapshot,r,0x100000u)); r[0x30c84]^=1; free(snapshot); }
    { pr_u25_store_word_cpu raw; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(&raw,&c,sizeof raw); word(r,0x249d6,0x78f2); word(r,0x249d8,0x4000); memcpy(snapshot,r,0x100000u);
      CHECK(pr_u25_store_class1_hook_entry(r,0x100000u,tb+13,0x95,&raw)==PR_U25_STORE_WORD_ARGUMENT);
      CHECK(!memcmp(&raw,&c,sizeof c) && !memcmp(snapshot,r,0x100000u)); word(r,0x249d6,0x100); word(r,0x249d8,0x6000); free(snapshot); }
    { pr_u25_store_word_cpu raw; memcpy(&raw,&c,sizeof raw);
      CHECK(pr_u25_store_class1_hook_entry(r,0x100000u,tb+13,0x95,&raw)==PR_U25_STORE_WORD_OK);
      CHECK(raw.cs==0x3000 && raw.ip==0x0a74 && raw.sp==0x7ff4 && raw.bp==0x7ffe && raw.ax==0xdd04 && raw.dx==0x0015 && raw.di==0xde04 && raw.es==0x6000 && (raw.flags&0x08d5)==0x0080);
      CHECK(r[0x60804]==0x15 && r[0x60805]==0);
      CHECK(r[0x47ff4]==0xd8 && r[0x47ff5]==0x0c && r[0x47ff6]==0 && r[0x47ff7]==0x30 && r[0x47ff8]==0x15 && r[0x47ff9]==0 && r[0x47ffa]==0x82 && r[0x47ffb]==0x6e);
      { pr_u25_store_word_cpu prior=raw; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_u25_store_synthetic_hook_return(r,0x100000u,th,0x94,&raw)==PR_U25_STORE_WORD_ARGUMENT);
      CHECK(!memcmp(&raw,&prior,sizeof raw) && !memcmp(snapshot,r,0x100000u)); free(snapshot); }
      { pr_u25_store_word_cpu prior=raw; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); r[0x30a74]^=1; memcpy(snapshot,r,0x100000u);
      CHECK(pr_u25_store_synthetic_hook_return(r,0x100000u,th,0x95,&raw)==PR_U25_STORE_WORD_ARGUMENT);
      CHECK(!memcmp(&raw,&prior,sizeof raw) && !memcmp(snapshot,r,0x100000u)); r[0x30a74]^=1; free(snapshot); }
      { uint8_t *expected=malloc(0x100000u);
      CHECK(expected); memcpy(expected,r,0x100000u); expected[0x47ff2]=0xfe; expected[0x47ff3]=0x7f; expected[0x47ffa]=0x82; expected[0x47ffb]=3;
      CHECK(pr_u25_store_synthetic_hook_return(r,0x100000u,th,0x95,&raw)==PR_U25_STORE_WORD_OK);
      CHECK(raw.cs==0x3000 && raw.ip==0x0cd8 && raw.sp==0x7ffc && raw.bp==0x7ffe && raw.ax==0x0382 && (raw.flags&0x08d5)==0x0014);
      CHECK(!memcmp(expected,r,0x100000u)); free(expected); }
      { pr_u25_store_word_cpu prior=raw; uint8_t *snapshot=malloc(0x100000u);
      CHECK(snapshot); memcpy(snapshot,r,0x100000u);
      CHECK(pr_u25_store_synthetic_return(r,0x100000u,tb+97,tb+399,0x94,&raw)==PR_U25_STORE_WORD_ARGUMENT);
      CHECK(!memcmp(&raw,&prior,sizeof raw) && !memcmp(snapshot,r,0x100000u)); r[0x30e06]^=1; memcpy(snapshot,r,0x100000u);
      CHECK(pr_u25_store_synthetic_return(r,0x100000u,tb+97,tb+399,0x95,&raw)==PR_U25_STORE_WORD_ARGUMENT);
      CHECK(!memcmp(&raw,&prior,sizeof raw) && !memcmp(snapshot,r,0x100000u)); r[0x30e06]^=1; memcpy(snapshot,r,0x100000u);
      CHECK(pr_u25_store_synthetic_return(r,0x100000u,tb+97,tb+399,0x95,&raw)==PR_U25_STORE_WORD_OK);
      CHECK(raw.cs==0x3582 && raw.ip==0x0e5a && raw.sp==0x8008 && raw.bp==0x8010 && raw.ax==0x0382 && (raw.flags&0x08d5)==0x0014);
      CHECK(!memcmp(snapshot,r,0x100000u)); memcpy(&c,&raw,sizeof c); { pr_u1c_3171_cpu prior=c;
      CHECK(pr_slot_a_synthetic_and_return(r,0x100000u,andtail,0x94,&c));
      CHECK(!memcmp(&c,&prior,sizeof c) && !memcmp(snapshot,r,0x100000u)); r[0x3667a]^=1; memcpy(snapshot,r,0x100000u);
      CHECK(pr_slot_a_synthetic_and_return(r,0x100000u,andtail,0x95,&c));
      CHECK(!memcmp(&c,&prior,sizeof c) && !memcmp(snapshot,r,0x100000u)); r[0x3667a]^=1; memcpy(snapshot,r,0x100000u);
      CHECK(!pr_slot_a_synthetic_and_return(r,0x100000u,andtail,0x95,&c));
      CHECK(c.cs==0x3582 && c.ip==0x30f7 && c.sp==0x8016 && c.bp==0x8016 && !memcmp(snapshot,r,0x100000u));
      { pr_u1c_3171_cpu prior=c; memcpy(r+0x3898f,anddispatchtail,2); memcpy(snapshot,r,0x100000u); CHECK(pr_slot_a_synthetic_dispatch_return(r,0x100000u,andjump,anddispatchtail,0x94,&c)); CHECK(!memcmp(&c,&prior,sizeof c) && !memcmp(snapshot,r,0x100000u)); r[0x38917]^=1; memcpy(snapshot,r,0x100000u); CHECK(pr_slot_a_synthetic_dispatch_return(r,0x100000u,andjump,anddispatchtail,0x95,&c)); CHECK(!memcmp(&c,&prior,sizeof c) && !memcmp(snapshot,r,0x100000u)); r[0x38917]^=1; memcpy(snapshot,r,0x100000u); CHECK(!pr_slot_a_synthetic_dispatch_return(r,0x100000u,andjump,anddispatchtail,0x95,&c)); CHECK(c.cs==0x3582 && c.ip==0x31a8 && c.sp==0x801c && c.bp==0x2222 && !memcmp(snapshot,r,0x100000u));
      { pr_u1c_3171_cpu prior=c; r[0x389c8]^=1; memcpy(snapshot,r,0x100000u); CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_JUMP_REFUSED); CHECK(!memcmp(&c,&prior,sizeof c) && !memcmp(snapshot,r,0x100000u)); r[0x389c8]^=1; CHECK(pr_slot_a_compare_runner_next(r,0x100000u,loop,fetch,&c)==PR_COMPARE_RUNNER_FETCHED); CHECK(c.cs==0x3582 && c.ip==0x31a4 && c.sp==0x801c && (uint8_t)c.ax==0x1b && r[0x26f7f]==0x1b && r[0x60804]==0x15 && r[0x60805]==0);
      CHECK(pr_slot_a_if_dispatch_prefix(r,0x100000u,&c,pro,branch,ifselect)==0x2f8c); CHECK(c.ip==0x2f8c && c.sp==0x8016 && (uint8_t)c.ax==0x1b && (c.flags&0x08d5)==0x0004);
      { pr_u1c_3171_cpu prior=c; r[0x387ac]^=1; memcpy(snapshot,r,0x100000u); CHECK(pr_slot_a_if_later_dispatch(r,0x100000u,later,149,&c)); CHECK(!memcmp(&c,&prior,sizeof c) && !memcmp(snapshot,r,0x100000u)); r[0x387ac]^=1; }
      CHECK(!pr_slot_a_if_later_dispatch(r,0x100000u,later,149,&c)); CHECK(c.cs==0x3582 && c.ip==0x3021 && c.sp==0x8016 && (c.flags&0x08d5)==0x0044 && r[0x26f7f]==0x1b && r[0x60804]==0x15);
      { pr_u1c_3171_cpu trialcpu=c; uint8_t *trial=malloc(0x100000u); CHECK(trial); memcpy(trial,r,0x100000u); trial[0x38843]^=1; CHECK(pr_slot_a_if_call(trial,0x100000u,ifcall,4,&trialcpu)==PR_IF_CALL_NEAR_CODE); CHECK(trialcpu.ip==0x3022 && trialcpu.sp==0x8014 && trial[0x48014]==0x82 && trial[0x48015]==0x35); free(trial); }
      CHECK(pr_slot_a_if_call(r,0x100000u,ifcall,4,&c)==PR_IF_CALL_REACHED); CHECK(c.cs==0x3582 && c.ip==0x0b52 && c.sp==0x8012 && r[0x48012]==0x25 && r[0x48013]==0x30 && r[0x48014]==0x82 && r[0x48015]==0x35 && r[0x26f7f]==0x1b && r[0x60804]==0x15);
      { pr_u1c_3171_cpu trialcpu=c; uint8_t *trial=malloc(0x100000u),*trialbefore=malloc(0x100000u); CHECK(trial&&trialbefore); memcpy(trial,r,0x100000u); trial[0x26f7d]=1; memcpy(trialbefore,trial,0x100000u); CHECK(pr_slot_a_if_handler_prefix(trial,0x100000u,ifbody,111,&trialcpu)); CHECK(!memcmp(&trialcpu,&c,sizeof c) && !memcmp(trial,trialbefore,0x100000u)); free(trialbefore); free(trial); }
      CHECK(!pr_slot_a_if_handler_prefix(r,0x100000u,ifbody,111,&c)); CHECK(c.cs==0x3582 && c.ip==0x0bc1 && c.sp==0x8010 && c.bp==0x8010 && r[0x249ed]==0x75 && r[0x249ee]==0x99 && (c.flags&0x08d5)==0x0044 && r[0x26f7f]==0x1b && r[0x60804]==0x15);
      { pr_u1c_3171_cpu prior=c; uint8_t *savedram=malloc(0x100000u); CHECK(savedram); r[0x123ce]^=1; memcpy(savedram,r,0x100000u); CHECK(pr_slot_a_if_skip_entry(r,0x100000u,skipcall,skipstub,&c)); CHECK(!memcmp(&c,&prior,sizeof c) && !memcmp(savedram,r,0x100000u)); r[0x123ce]^=1; free(savedram); }
      CHECK(!pr_slot_a_if_skip_entry(r,0x100000u,skipcall,skipstub,&c)); CHECK(c.cs==0x3000 && c.ip==0x1ea6 && c.sp==0x800c && c.bp==0x8010 && r[0x4800c]==0xc6 && r[0x4800d]==0x0b && r[0x4800e]==0x82 && r[0x4800f]==0x35 && r[0x249ed]==0x75 && r[0x249ee]==0x99 && r[0x60804]==0x15); } } } free(snapshot); } }
    memcpy(r,saved,0x100000u); c=before; r[0x27006]=1; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_SLOT_REFUSED);
      CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    memcpy(r,saved,0x100000u); c=before; r[0x27006]=0; r[0x35adf]=1;
    CHECK(pr_slot_a_first_save_compose(r,0x100000u,&refs,&c,&report)==PR_FIRST_SAVE_ARGS_REFUSED);
      CHECK(report.slot==PR_U25_SLOT_VALUE_OK && c.cs==0x3582 && c.ip==0x02bf && c.ax==11 && r[0x607fa]==0);
    free(saved); free(r); puts("conditional GOTO dispatch chain passed"); return 0;
}
