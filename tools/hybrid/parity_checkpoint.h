/* Opt-in t116 checkpoint, included after pr_hybrid_dispatch.h. Linux oracle only.
 * Captures backing RAM separately from VGA memory; neither is a machine save state.
 * Stops this disposable emulator so its displayed frame can be captured without
 * guest progress. Does not assert that the backend has presented a new frame.
 */
#include <signal.h>
#include "vga.h"

static void PR_ParityCheckpoint(const char *id, uint64_t arrival) {
    const char *target = getenv("PR_PARITY_TARGET");
    const char *prefix = getenv("PR_PARITY_PREFIX");
    if (!target || !prefix || strcmp(target, id)) return;
    const std::string base(prefix);
    struct stat st;
    if (stat((base + ".arm").c_str(), &st)) return;
    if (unlink((base + ".arm").c_str())) return;
    struct Blob {
        static void write(const std::string &name, const void *p, size_t n) {
            FILE *f = fopen(name.c_str(), "wb");
            if (!f) { perror("parity checkpoint"); exit(90); }
            if (fwrite(p, 1, n, f) != n || fclose(f)) exit(91);
        }
    };
    Blob::write(base + ".ram", MemBase, 1u << 20);
    Blob::write(base + ".vram", vga.mem.linear, vga.mem.memsize);
    pr_disp::Regs r = pr_disp::grab();
    FILE *f = fopen((base + ".json").c_str(), "w");
    if (!f) exit(92);
    fprintf(f, "{\"schema\":1,\"id\":\"%s\",\"pid\":%u,\"arrival\":%llu,\"instruction\":%llu,"
            "\"routed_returns\":%llu,\"routed_stops\":%llu,"
            "\"registers\":{", id, (unsigned)getpid(), (unsigned long long)arrival,
            (unsigned long long)pr_obs::insns, (unsigned long long)pr_disp::routed_returned,
            (unsigned long long)pr_disp::routed_stops);
#define PR_REG(name) fprintf(f, "\"" #name "\":%u,", (unsigned)r.name)
    PR_REG(eax); PR_REG(ebx); PR_REG(ecx); PR_REG(edx);
    PR_REG(esi); PR_REG(edi); PR_REG(ebp); PR_REG(esp);
    PR_REG(cs); PR_REG(ds); PR_REG(es); PR_REG(ss); PR_REG(eip); PR_REG(fs); PR_REG(gs);
#undef PR_REG
    fprintf(f, "\"flags\":%u},\"video\":{\"mode\":%u,\"display_start\":%u,"
            "\"real_start\":%u,\"scan_len\":%u,\"width\":%u,\"height\":%u,"
            "\"vmemsize\":%u}}\n", (unsigned)r.flags, (unsigned)vga.mode,
            (unsigned)vga.config.display_start, (unsigned)vga.config.real_start,
            (unsigned)vga.config.scan_len, (unsigned)vga.draw.width,
            (unsigned)vga.draw.height, (unsigned)vga.mem.memsize);
    if (fclose(f)) exit(93);
    pr_obs::report();
    if (pr_disp::log) fflush(pr_disp::log);
    raise(SIGSTOP);
}
