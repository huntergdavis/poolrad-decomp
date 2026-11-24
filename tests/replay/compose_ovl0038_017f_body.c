/* OVL_0038_017F body composition (evidence only).
   Models the 31 instructions of the body (+0000..+0041) at SS:SP. The three far calls are not executed here: at each call the
   declared callee effect is applied (the bytes the callee wrote, the registers after its return, the SP after its RETF). The
   declared effects are read from a text file produced from the native trace. Output: the final 640 KiB RAM image and a
   14-field register line (AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS) for comparison with the native exit state. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE 655360u
#define MAX_W 65536u
static uint8_t ram[RAM_SIZE];
static uint16_t AX, BX, CX, DX, SI, DI, BP, SP, CS, IP, DS, ES, SS, FL;

struct callee { uint32_t n; uint32_t addr[MAX_W]; uint8_t val[MAX_W]; uint16_t regs[10]; uint16_t sp_after; };
static struct callee cal[3];

static void die(const char *m) { fprintf(stderr, "ovl017f_body: %s\n", m); exit(2); }
static uint32_t lin(uint16_t seg, uint16_t off) { return (((uint32_t)seg << 4) + off) & 0xFFFFFu; }
static void wb(uint32_t a, uint8_t v) { if (a < RAM_SIZE) ram[a] = v; else die("write outside 640 KiB"); }
static void ww(uint32_t a, uint16_t v) { wb(a, (uint8_t)v); wb(a + 1, (uint8_t)(v >> 8)); }
static uint16_t rw(uint32_t a) {
    if (a + 1 >= RAM_SIZE) die("read outside 640 KiB");
    return (uint16_t)(ram[a] | (ram[a + 1] << 8));
}
static void push(uint16_t v) { SP = (uint16_t)(SP - 2); ww(lin(SS, SP), v); }
static uint16_t pop(void) { uint16_t v = rw(lin(SS, SP)); SP = (uint16_t)(SP + 2); return v; }

static unsigned hex_token(FILE *f) {
    unsigned v;
    if (fscanf(f, "%x", &v) != 1) die("malformed hex field");
    return v;
}

static void load_effects(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) die("cannot open effects");
    int k = -1;
    char tag[16];
    while (fscanf(f, "%15s", tag) == 1) {
        if (!strcmp(tag, "CALL")) {
            unsigned kk = hex_token(f);
            if (kk > 2) die("call index out of range");
            k = (int)kk;
        } else if (!strcmp(tag, "W")) {
            if (k < 0 || cal[k].n >= MAX_W) die("write record out of order or too many writes");
            cal[k].addr[cal[k].n] = hex_token(f);
            cal[k].val[cal[k].n] = (uint8_t)hex_token(f);
            cal[k].n++;
        } else if (!strcmp(tag, "R")) {
            if (k < 0) die("register record out of order");
            for (int i = 0; i < 10; i++) cal[k].regs[i] = (uint16_t)hex_token(f);
        } else if (!strcmp(tag, "SP")) {
            if (k < 0) die("SP record out of order");
            cal[k].sp_after = (uint16_t)hex_token(f);
        } else {
            die("unknown effect tag");
        }
    }
    fclose(f);
}

static void apply_callee(int k) {
    for (uint32_t i = 0; i < cal[k].n; i++) wb(cal[k].addr[i], cal[k].val[i]);
    AX = cal[k].regs[0]; BX = cal[k].regs[1]; CX = cal[k].regs[2]; DX = cal[k].regs[3];
    SI = cal[k].regs[4]; DI = cal[k].regs[5]; BP = cal[k].regs[6]; ES = cal[k].regs[7];
    DS = cal[k].regs[8]; FL = cal[k].regs[9];
    SP = cal[k].sp_after;
}

int main(int argc, char **argv) {
    if (argc != 5) { fprintf(stderr, "usage: ovl017f_body ENTRY_RAM ENTRY_REGS(13 regs + flags, hex) EFFECTS OUT\n"); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f || fread(ram, 1, RAM_SIZE, f) != RAM_SIZE) die("entry RAM must be 655360 bytes");
    fclose(f);
    unsigned r[14];
    f = fopen(argv[2], "r");
    if (!f) die("cannot open entry registers");
    for (int i = 0; i < 14; i++) r[i] = hex_token(f);
    fclose(f);
    AX = (uint16_t)r[0]; BX = (uint16_t)r[1]; CX = (uint16_t)r[2]; DX = (uint16_t)r[3];
    SI = (uint16_t)r[4]; DI = (uint16_t)r[5]; BP = (uint16_t)r[6]; SP = (uint16_t)r[7];
    CS = (uint16_t)r[8]; IP = (uint16_t)r[9]; DS = (uint16_t)r[10]; ES = (uint16_t)r[11]; SS = (uint16_t)r[12];
    FL = (uint16_t)r[13];
    load_effects(argv[3]);

    /* +0000..+0016: prologue and the first call's arguments */
    push(BP); BP = SP; SP = (uint16_t)(SP - 0x10);
    push(rw(lin(SS, (uint16_t)(BP + 0x0E))));          /* push word [bp+0Eh] */
    push(rw(lin(SS, (uint16_t)(BP + 0x0C))));          /* push word [bp+0Ch] */
    DI = (uint16_t)(BP - 0x10);                        /* lea di,[bp-10h] */
    push(SS); push(DI);
    DI = 0x016F; push(CS); push(DI);                   /* mov di,16Fh; push cs; push di */
    push(CS); push((uint16_t)0x019A);                  /* lcall 0AF8:0B0B: return CS:IP (017F+1Bh) */
    apply_callee(0);
    /* +001B..+0026: second call's arguments (AL=0Ah, AL=1, each pushed as a word) */
    AX = (uint16_t)((AX & 0xFF00) | 0x0A); push(AX);
    AX = (uint16_t)((AX & 0xFF00) | 0x01); push(AX);
    push(CS); push((uint16_t)0x01A5);                  /* lcall 00BA:16EC: return 017F+26h */
    apply_callee(1);
    /* +0026..+003E: third call's arguments */
    push(rw(lin(SS, (uint16_t)(BP + 0x0E))));
    push(rw(lin(SS, (uint16_t)(BP + 0x0C))));
    AX = (uint16_t)((AX & 0xFF00) | 0x07); push(AX);
    AX = 0xA8C0; push(AX);
    AX = (uint16_t)((AX & 0xFF00) | 0xFF); push(AX);
    AX = (uint16_t)((AX & 0xFF00) | 0x01); push(AX);
    push(CS); push((uint16_t)0x01BD);                  /* lcall 00B0:0052: return 017F+3Eh */
    apply_callee(2);
    /* +003E..+0041: mov sp,bp; pop bp; retf 0Ah */
    SP = BP; BP = pop();
    IP = pop(); CS = pop();
    SP = (uint16_t)(SP + 0x0A);

    f = fopen(argv[4], "wb");
    if (!f || fwrite(ram, 1, RAM_SIZE, f) != RAM_SIZE) die("cannot write RAM image");
    fclose(f);
    char regs_path[1024];
    if (snprintf(regs_path, sizeof regs_path, "%s.regs", argv[4]) >= (int)sizeof regs_path) die("path too long");
    f = fopen(regs_path, "w");
    if (!f) die("cannot write register file");
    fprintf(f, "%04X %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X\n",
            AX, BX, CX, DX, SI, DI, BP, SP, CS, IP, DS, ES, SS, FL);
    fclose(f);
    return 0;
}
