/* Conformance driver for pr_codeword_assist.h: prints the same transcripts as tests/hybrid/conformance_transcript.py.
 * Synthetic fixtures only (generated letters and tails). Build: g++ -std=c++11 -Wall -Wextra -Werror -I<this dir>. */
#include <cstdio>
#include <map>
#include <string>
#include "pr_codeword_assist.h"

static const uint16_t CHILD_BP = 0x3A72, FRAME_BP = 0x3DA4, UNIT_BP = 0x3EF8, UNIT_SEG = 0x38D3;

struct TestMem : pr_ca::Mem {
    std::map<uint16_t, uint16_t> words;
    std::map<uint16_t, uint8_t> bytes;
    uint8_t table[pr_ca::kTableBytes];
    bool ss_word(uint16_t off, uint16_t &v) override {
        std::map<uint16_t, uint16_t>::iterator it = words.find(off);
        if (it == words.end()) return false;
        v = it->second;
        return true;
    }
    bool ss_byte(uint16_t off, uint8_t &v) override {
        std::map<uint16_t, uint8_t>::iterator it = bytes.find(off);
        if (it == bytes.end()) return false;
        v = it->second;
        return true;
    }
    bool ds_table(uint8_t *out) override {
        for (int i = 0; i < pr_ca::kTableBytes; ++i) out[i] = table[i];
        return true;
    }
};

static std::string synthetic_text(int i) {
    std::string s;
    for (int k = 0; k < pr_ca::kRecordTextLen; ++k) s.push_back((char)(65 + ((i * 7 + k * 3) % 26)));
    return s;
}

static void build_table(uint8_t *table, bool ok) {
    for (int i = 0; i < pr_ca::kTableBytes; ++i) table[i] = 0;
    for (int i = 0; i < pr_ca::kRecordCount; ++i) {
        int slot = pr_ca::record_slot(i);
        table[slot] = (uint8_t)pr_ca::kRecordTextLen;
        std::string t = synthetic_text(i);
        for (int k = 0; k < pr_ca::kRecordTextLen; ++k) table[slot + 1 + k] = (uint8_t)t[k];
        for (int k = 0; k < 14; ++k) table[slot + 1 + pr_ca::kRecordTextLen + k] = (uint8_t)((i * 11 + k) & 0xFF);
    }
    if (!ok) table[pr_ca::record_slot(5) + 1] ^= 1;
}

struct Prompt {
    int index, attempt;
    bool rows, table_ok;
    int break_frame_after;                   /* -1 never; else after that many sends */
    bool key_buffers;
    int max_steps;
    int nonempty_polls;                      /* first polls see head behind tail */
};

static void run_nonprompt(pr_ca::Gate &g, int n, FILE *out) {
    for (int k = 0; k < n; ++k) {
        TestMem m;
        build_table(m.table, true);
        pr_ca::Snap s;
        s.bp = CHILD_BP; s.keyboard_poll = true; s.bda_head = 0x1E; s.bda_tail = 0x1E;
        m.words[CHILD_BP] = FRAME_BP;  m.words[CHILD_BP + 2] = 0x0A30; m.words[CHILD_BP + 4] = 0x0F2D;
        m.words[FRAME_BP] = 0x0000;    m.words[FRAME_BP + 2] = 0x0355; m.words[FRAME_BP + 4] = UNIT_SEG;
        pr_ca::Action a = g.step(s, m);
        fprintf(out, "%s|%s|%s\n", a.kind.c_str(), a.key.c_str(), a.reason.c_str());
    }
}

static void run_prompt(pr_ca::Gate &g, const Prompt &p, FILE *out) {
    uint16_t head = 0x1E, tail = 0x1E;
    int sends = 0;
    bool broken = false;
    for (int step = 0; step < p.max_steps; ++step) {
        TestMem m;
        build_table(m.table, p.table_ok);
        pr_ca::Snap s;
        s.bp = CHILD_BP; s.keyboard_poll = true;
        if (step < p.nonempty_polls) { head = 0x1E; tail = 0x20; }
        else if (step == p.nonempty_polls && p.nonempty_polls > 0) { head = tail; }
        s.bda_head = head; s.bda_tail = tail;
        uint16_t frame_ret = (broken) ? (uint16_t)0x0355 : (uint16_t)pr_ca::kUnitRetIp;
        m.words[CHILD_BP] = FRAME_BP;  m.words[CHILD_BP + 2] = 0x0A30; m.words[CHILD_BP + 4] = 0x0F2D;
        m.words[FRAME_BP] = UNIT_BP;   m.words[FRAME_BP + 2] = frame_ret; m.words[FRAME_BP + 4] = UNIT_SEG;
        m.words[UNIT_BP] = 0; m.words[UNIT_BP + 2] = 0; m.words[UNIT_BP + 4] = 0;
        m.bytes[(uint16_t)(UNIT_BP - pr_ca::kIndexOffset)] = (uint8_t)p.index;
        m.bytes[(uint16_t)(UNIT_BP - pr_ca::kAttemptOffset)] = (uint8_t)p.attempt;
        if (p.rows) {
            for (int k = 0; k < 6; ++k) s.printer_calls.push_back(std::make_pair((uint16_t)UNIT_SEG, pr_ca::kFixedRows[k]));
        } else {
            for (int k = 0; k < 5; ++k) s.printer_calls.push_back(std::make_pair((uint16_t)UNIT_SEG, pr_ca::kFixedRows[k]));
        }
        pr_ca::Action a = g.step(s, m);
        fprintf(out, "%s|%s|%s\n", a.kind.c_str(), a.key.c_str(), a.reason.c_str());
        if (a.kind == "SEND") {
            ++sends;
            if (p.key_buffers) tail = (uint16_t)(tail + 2);
            if (p.break_frame_after > 0 && sends == p.break_frame_after) broken = true;
        } else if (a.kind == "WAIT") {
            head = tail;
        }
        if (a.reason == "completed" || a.reason.compare(0, 6, "abort:") == 0) break;
    }
}

int main() {
    FILE *out = stdout;
    uint8_t synth[pr_ca::kTableBytes];
    build_table(synth, true);
    const std::string synth_sha = pr_ca::sha256_hex(synth, pr_ca::kTableBytes);
    fprintf(out, "digest %s\n", synth_sha.c_str());
    /* A: all 13 indices, one occurrence each */
    fprintf(out, "== complete-all\n");
    {
        pr_ca::Gate g(true, synth_sha);
        for (int i = 0; i < pr_ca::kRecordCount; ++i) {
            Prompt p = {i, 1, true, true, false, true, 80, 0};
            run_prompt(g, p, out);
        }
    }
    /* B: the same occurrence again after completion */
    fprintf(out, "== repeat-same\n");
    {
        pr_ca::Gate g(true, synth_sha);
        Prompt p1 = {4, 1, true, true, false, true, 80, 0};
        run_prompt(g, p1, out);
        Prompt p2 = {4, 1, true, true, false, true, 3, 0};
        run_prompt(g, p2, out);
    }
    /* C: attempt two is a new occurrence */
    fprintf(out, "== attempt-two\n");
    {
        pr_ca::Gate g(true, synth_sha);
        Prompt p1 = {4, 1, true, true, false, true, 80, 0};
        run_prompt(g, p1, out);
        Prompt p2 = {7, 2, true, true, false, true, 80, 0};
        run_prompt(g, p2, out);
    }
    /* D: frame lost after two sends: abort, then the occurrence stays handled */
    fprintf(out, "== guard-break\n");
    {
        pr_ca::Gate g(true, synth_sha);
        Prompt p1 = {2, 1, true, true, 2, true, 80, 0};
        run_prompt(g, p1, out);
        Prompt p2 = {2, 1, true, true, false, true, 3, 0};
        run_prompt(g, p2, out);
    }
    /* E: the key never buffers: waits, then abort */
    fprintf(out, "== never-buffers\n");
    {
        pr_ca::Gate g(true, synth_sha);
        Prompt p = {6, 1, true, true, false, false, 260, 0};
        run_prompt(g, p, out);
    }
    /* F: buffer non-empty at first, then consumed */
    fprintf(out, "== buffer-not-empty\n");
    {
        pr_ca::Gate g(true, synth_sha);
        Prompt p = {3, 1, true, true, false, true, 80, 3};
        run_prompt(g, p, out);
    }
    /* G: fixed rows missing: silence */
    fprintf(out, "== fixed-rows-missing\n");
    {
        pr_ca::Gate g(true, synth_sha);
        Prompt p = {5, 1, false, true, false, true, 3, 0};
        run_prompt(g, p, out);
    }
    /* H: table digest mismatch: silence */
    fprintf(out, "== table-digest\n");
    {
        pr_ca::Gate g(true, synth_sha);
        Prompt p = {5, 1, true, false, false, true, 2, 0};
        run_prompt(g, p, out);
    }
    /* I: assist off: silence, no keys */
    fprintf(out, "== assist-off\n");
    {
        pr_ca::Gate g(false, synth_sha);
        Prompt p = {5, 1, true, true, false, true, 3, 0};
        run_prompt(g, p, out);
    }
    /* J: non-prompt polls, then a prompt */
    fprintf(out, "== nonprompt-then-prompt\n");
    {
        pr_ca::Gate g(true, synth_sha);
        run_nonprompt(g, 5, out);
        Prompt p = {9, 1, true, true, false, true, 80, 0};
        run_prompt(g, p, out);
    }
    return 0;
}
