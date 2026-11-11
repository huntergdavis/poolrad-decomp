/* Private run002: bounded entry1 -> coordinate writer -> refused resident.
 * Compares selected modeled data only; not a CPU/full-RAM parity test. */
#include "poolrad/recovered/overlay_u68_entry1.h"
#include "poolrad/recovered/overlay_u68_coordinate_writer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE 655360u
#define DS 0x14a00u
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
typedef struct { uint8_t *ram; unsigned calls; uint16_t word; pr_u68_step_status status; pr_u68_step_report report; } ctx_t;
static unsigned w(const uint8_t *p) { return p[0] | ((unsigned)p[1] << 8); }
static int resident(void *p, uint16_t word) { ctx_t *c=p; c->calls++; c->word=word; return 1; }
static int step(void *p, pr_u68_e1_state *s) {
    ctx_t *c=p; pr_u68_step_state t={0}; pr_u68_step_hooks h={resident,NULL,c};
    t.party_x=s->party_x; t.party_y=s->party_y; t.facing=s->facing;
    t.word_2628=(uint16_t)w(c->ram+DS+0x2628); t.hooks=&h;
    c->status=pr_u68_step_coordinate_writer(&t,&c->report);
    s->party_x=t.party_x; s->party_y=t.party_y; s->facing=t.facing;
    return c->status != PR_U68_STEP_OK;
}
static int load(const char *p, uint8_t *b) { FILE *f=fopen(p,"rb"); size_t n; int extra; if(!f) return 0; n=fread(b,1,SIZE,f); extra=fgetc(f); fclose(f); return n==SIZE && extra==EOF; }
int main(int argc,char **argv) {
    uint8_t *e=malloc(SIZE), *r=malloc(SIZE); pr_u68_e1_state s={0}; pr_u68_e1_report report; ctx_t c={0};
    unsigned win,buf;
    CHECK(argc==3 && e && r && load(argv[1],e) && load(argv[2],r));
    win=(w(e+DS+0x49d8)<<4)+w(e+DS+0x49d6); buf=(w(e+DS+0x6a5e)<<4)+w(e+DS+0x6a5c);
    CHECK(win+0x596<=SIZE && buf+0x400<=SIZE);
    s.selector=e[DS+0x49fa]; s.window1=e+win; s.window1_size=0x596;
    s.party_x=e[DS+0x6aad]; s.party_y=e[DS+0x6aae]; s.facing=e[DS+0x6aaf];
    memcpy(s.ds_6dde,e+DS+0x6dde,3); s.ds_6b00=e[DS+0x6b00]; s.ds_6b05=e[DS+0x6b05];
    s.field.map_id=e[DS+0x84dc]; s.field.buffer=e+buf; c.ram=e; s.ctx=&c;
    CHECK(pr_u68_entry1_fallible_step(&s,&report,step)==PR_U68_E1_STEP_REFUSED);
    CHECK(report.entry3_result==1 && report.move_flag==1 && report.committed_move==1);
    CHECK(report.calls_n==2 && report.calls[0]==PR_U68_E1_CALL_ENTRY3 && report.calls[1]==PR_U68_E1_CALL_073B);
    CHECK(c.status==PR_U68_STEP_HOOK && c.report.stop_offset==0x0742 && c.calls==1);
    CHECK(c.word==w(r+0x1d160u+0x3dbcu) && c.word==10);
    CHECK(s.party_x==r[DS+0x6aad] && s.party_y==r[DS+0x6aae] && s.facing==r[DS+0x6aaf]);
    CHECK(!memcmp(s.ds_6dde,r+DS+0x6dde,3) && s.ds_6b00==r[DS+0x6b00] && s.ds_6b05==r[DS+0x6b05]);
    printf("entry3=1; coordinate stop=0742; resident word=%u; party=(%u,%u,%u); selected state matches same-run 0742\n",c.word,s.party_x,s.party_y,s.facing);
    free(e); free(r); return 0;
}
