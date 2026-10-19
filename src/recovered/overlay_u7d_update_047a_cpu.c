/* Complete U7D [047A,0563); docs/re/U7D_UPDATE_047A_CPU.md. */
#include "poolrad/recovered/overlay_u7d_update_047a_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#define MASK ((1u << 20) - 1u)
static size_t phys(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & MASK; }
static uint16_t get(const uint8_t *m, uint16_t s, uint16_t o, unsigned bits)
{
    size_t p = phys(s, o);
    return (uint16_t)(m[p] | (bits == 16 ? (unsigned)m[(p + 1u) & MASK] << 8 : 0u));
}
static void put(uint8_t *m, uint16_t s, uint16_t o, unsigned bits, uint16_t v)
{
    size_t p = phys(s, o); m[p] = (uint8_t)v;
    if (bits == 16) m[(p + 1u) & MASK] = (uint8_t)(v >> 8);
}
static void push(uint8_t *m, pr_u7d_047a_cpu *c, uint16_t v)
{ c->sp = (uint16_t)(c->sp - 2u); put(m, c->ss, c->sp, 16, v); }
static uint16_t alu(pr_u7d_047a_cpu *c, uint16_t a, uint16_t b, unsigned bits, int sub)
{
    unsigned mask = bits == 8 ? 255u : 65535u, sign = (mask + 1u) >> 1;
    unsigned r = (sub ? (unsigned)a - b : (unsigned)a + b) & mask;
    uint16_t f = (uint16_t)(c->flags & ~0x08D5u);
    unsigned p = r & 255u; p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    if (sub ? a < b : (unsigned)a + b > mask) f |= 1u;
    if (!(p & 1u)) f |= 4u;
    if ((a ^ b ^ r) & 16u) f |= 16u;
    if (!r) f |= 64u;
    if (r & sign) f |= 128u;
    if ((sub ? a ^ b : ~(a ^ b)) & (a ^ r) & sign) f |= 0x800u;
    c->flags = f; return (uint16_t)r;
}
#define COPY_FIELDS(D,S) do { \
 (D).ax=(S).ax; (D).bx=(S).bx; (D).cx=(S).cx; (D).dx=(S).dx; \
 (D).si=(S).si; (D).di=(S).di; (D).bp=(S).bp; (D).sp=(S).sp; \
 (D).cs=(S).cs; (D).ip=(S).ip; (D).ds=(S).ds; (D).es=(S).es; \
 (D).ss=(S).ss; (D).flags=(S).flags; } while (0)
pr_u7d_047a_status pr_u7d_update_047a_cpu(
    uint8_t *m, size_t size, pr_u7d_047a_cpu *c, unsigned max_steps,
    pr_u7d_047a_diagnostic *d)
{
    unsigned steps = 0; uint16_t v, carry, source_cs, segment; int status;
    pr_system_leaf_cpu inner;
    if (d != NULL) { d->callee = -1; d->status = -1; }
    if (m == NULL || c == NULL || size != (1u << 20) || c->ip != 0x047Au ||
        c->sp < 0x20u || c->sp > 0xFFF8u) return PR_U7D_047A_ARGUMENT;
    for (;;) {
        if (steps == max_steps) return PR_U7D_047A_LIMIT;
        ++steps;
        switch (c->ip) {
        case 0x047Au: /* push bp */
            push(m, c, c->bp);
            c->ip = 0x047Bu;
            break;
        case 0x047Bu: /* mov bp, sp */
            c->bp = (uint16_t)(c->sp);
            c->ip = 0x047Du;
            break;
        case 0x047Du: /* sub sp, 4 */
            c->sp = (uint16_t)(alu(c, c->sp, 0x4u, 16, 1));
            c->ip = 0x0480u;
            break;
        case 0x0480u: /* cmp word ptr [0x6dca], 0 */
            (void)alu(c, get(m, c->ds, (uint16_t)(0x6DCAu), 16), 0x0u, 16, 1);
            c->ip = 0x0485u;
            break;
        case 0x0485u: /* jne 0x49f */
            c->ip = (!(c->flags & 64u)) ? 0x49Fu : 0x0487u;
            break;
        case 0x0487u: /* cmp word ptr [0x6dc8], 0 */
            (void)alu(c, get(m, c->ds, (uint16_t)(0x6DC8u), 16), 0x0u, 16, 1);
            c->ip = 0x048Cu;
            break;
        case 0x048Cu: /* jne 0x49f */
            c->ip = (!(c->flags & 64u)) ? 0x49Fu : 0x048Eu;
            break;
        case 0x048Eu: /* cmp word ptr [0x6dc6], 0 */
            (void)alu(c, get(m, c->ds, (uint16_t)(0x6DC6u), 16), 0x0u, 16, 1);
            c->ip = 0x0493u;
            break;
        case 0x0493u: /* jne 0x49f */
            c->ip = (!(c->flags & 64u)) ? 0x49Fu : 0x0495u;
            break;
        case 0x0495u: /* cmp word ptr [0x6dc4], 0 */
            (void)alu(c, get(m, c->ds, (uint16_t)(0x6DC4u), 16), 0x0u, 16, 1);
            c->ip = 0x049Au;
            break;
        case 0x049Au: /* jne 0x49f */
            c->ip = (!(c->flags & 64u)) ? 0x49Fu : 0x049Cu;
            break;
        case 0x049Cu: /* jmp 0x55d */
            c->ip = 0x55Du;
            break;
        case 0x049Fu: /* mov al, byte ptr [bp + 6] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp + 6), 8)));
            c->ip = 0x04A2u;
            break;
        case 0x04A2u: /* CBW: 16-bit opcode 98, sign-extend AL */
            c->ax = (uint16_t)((c->ax & 128u) ? (c->ax | 0xFF00u) : (c->ax & 255u));
            c->ip = 0x04A3u;
            break;
        case 0x04A3u: /* mov di, ax */
            c->di = (uint16_t)(c->ax);
            c->ip = 0x04A5u;
            break;
        case 0x04A5u: /* shl di, 1 */
            v = c->di;
            c->di = alu(c, v, v, 16, 0);
            c->flags &= (uint16_t)~16u;
            c->ip = 0x04A7u;
            break;
        case 0x04A7u: /* mov dx, word ptr [di + 0x6dc2] */
            c->dx = (uint16_t)(get(m, c->ds, (uint16_t)(c->di + 0x6DC2u), 16));
            c->ip = 0x04ABu;
            break;
        case 0x04ABu: /* mov al, byte ptr [bp + 8] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp + 8), 8)));
            c->ip = 0x04AEu;
            break;
        case 0x04AEu: /* xor ah, ah */
            c->ax &= 255u;
            c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x44u);
            c->ip = 0x04B0u;
            break;
        case 0x04B0u: /* cmp ax, dx */
            (void)alu(c, c->ax, c->dx, 16, 1);
            c->ip = 0x04B2u;
            break;
        case 0x04B2u: /* ja 0x4b7 */
            c->ip = (!(c->flags & 65u)) ? 0x4B7u : 0x04B4u;
            break;
        case 0x04B4u: /* jmp 0x546 */
            c->ip = 0x546u;
            break;
        case 0x04B7u: /* mov al, byte ptr [bp + 6] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp + 6), 8)));
            c->ip = 0x04BAu;
            break;
        case 0x04BAu: /* inc al */
            carry = c->flags & 1u;
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(alu(c, (uint8_t)(c->ax), 1u, 8, 0)));
            c->flags = (uint16_t)((c->flags & ~1u) | carry);
            c->ip = 0x04BCu;
            break;
        case 0x04BCu: /* mov byte ptr [bp - 1], al */
            put(m, c->ss, (uint16_t)(c->bp - 1), 8, (uint8_t)(c->ax));
            c->ip = 0x04BFu;
            break;
        case 0x04BFu: /* mov al, byte ptr [bp - 1] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp - 1), 8)));
            c->ip = 0x04C2u;
            break;
        case 0x04C2u: /* CBW: 16-bit opcode 98, sign-extend AL */
            c->ax = (uint16_t)((c->ax & 128u) ? (c->ax | 0xFF00u) : (c->ax & 255u));
            c->ip = 0x04C3u;
            break;
        case 0x04C3u: /* mov di, ax */
            c->di = (uint16_t)(c->ax);
            c->ip = 0x04C5u;
            break;
        case 0x04C5u: /* shl di, 1 */
            v = c->di;
            c->di = alu(c, v, v, 16, 0);
            c->flags &= (uint16_t)~16u;
            c->ip = 0x04C7u;
            break;
        case 0x04C7u: /* cmp word ptr [di + 0x6dc2], 0 */
            (void)alu(c, get(m, c->ds, (uint16_t)(c->di + 0x6DC2u), 16), 0x0u, 16, 1);
            c->ip = 0x04CCu;
            break;
        case 0x04CCu: /* jne 0x4de */
            c->ip = (!(c->flags & 64u)) ? 0x4DEu : 0x04CEu;
            break;
        case 0x04CEu: /* cmp byte ptr [bp - 1], 5 */
            (void)alu(c, get(m, c->ss, (uint16_t)(c->bp - 1), 8), 0x5u, 8, 1);
            c->ip = 0x04D2u;
            break;
        case 0x04D2u: /* jge 0x4de */
            c->ip = (!(((c->flags >> 7) ^ (c->flags >> 11)) & 1u)) ? 0x4DEu : 0x04D4u;
            break;
        case 0x04D4u: /* mov al, byte ptr [bp - 1] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp - 1), 8)));
            c->ip = 0x04D7u;
            break;
        case 0x04D7u: /* inc al */
            carry = c->flags & 1u;
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(alu(c, (uint8_t)(c->ax), 1u, 8, 0)));
            c->flags = (uint16_t)((c->flags & ~1u) | carry);
            c->ip = 0x04D9u;
            break;
        case 0x04D9u: /* mov byte ptr [bp - 1], al */
            put(m, c->ss, (uint16_t)(c->bp - 1), 8, (uint8_t)(c->ax));
            c->ip = 0x04DCu;
            break;
        case 0x04DCu: /* jmp 0x4bf */
            c->ip = 0x4BFu;
            break;
        case 0x04DEu: /* cmp byte ptr [bp - 1], 5 */
            (void)alu(c, get(m, c->ss, (uint16_t)(c->bp - 1), 8), 0x5u, 8, 1);
            c->ip = 0x04E2u;
            break;
        case 0x04E2u: /* jne 0x4fb */
            c->ip = (!(c->flags & 64u)) ? 0x4FBu : 0x04E4u;
            break;
        case 0x04E4u: /* mov di, 0x6dc2 */
            c->di = (uint16_t)(0x6DC2u);
            c->ip = 0x04E7u;
            break;
        case 0x04E7u: /* push ds */
            push(m, c, c->ds);
            c->ip = 0x04E8u;
            break;
        case 0x04E8u: /* push di */
            push(m, c, c->di);
            c->ip = 0x04E9u;
            break;
        case 0x04E9u: /* mov ax, 0xe */
            c->ax = (uint16_t)(0xEu);
            c->ip = 0x04ECu;
            break;
        case 0x04ECu: /* push ax */
            push(m, c, c->ax);
            c->ip = 0x04EDu;
            break;
        case 0x04EDu: /* mov al, 0 */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(0x0u));
            c->ip = 0x04EFu;
            break;
        case 0x04EFu: /* push ax */
            push(m, c, c->ax);
            c->ip = 0x04F0u;
            break;
        case 0x04F0u: /* lcall 0xaf8, 0x16a8 */
            if (get(m, c->cs, 0x04F0u, 8) != 0x9Au || get(m, c->cs, 0x04F1u, 16) != 0x16A8u) return PR_U7D_047A_CODE;
            source_cs = c->cs;
            segment = get(m, c->cs, 0x04F3u, 16);
            push(m, c, source_cs);
            push(m, c, 0x04F5u);
            c->ip = 0x16A8u;
            c->cs = segment;
            COPY_FIELDS(inner, (*c));
            status = pr_system_fillchar_16a8(m, size, &inner);
            COPY_FIELDS((*c), inner);
            if (d != NULL) { d->callee = 0; d->status = status; }
            if (status != 0) return PR_U7D_047A_CALLEE;
            if (c->cs != source_cs || c->ip != 0x04F5u) return PR_U7D_047A_TRANSFER;
            break;
        case 0x04F5u: /* mov byte ptr [bp + 8], 0 */
            put(m, c->ss, (uint16_t)(c->bp + 8), 8, 0x0u);
            c->ip = 0x04F9u;
            break;
        case 0x04F9u: /* jmp 0x543 */
            c->ip = 0x543u;
            break;
        case 0x04FBu: /* mov al, byte ptr [bp + 6] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp + 6), 8)));
            c->ip = 0x04FEu;
            break;
        case 0x04FEu: /* inc al */
            carry = c->flags & 1u;
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(alu(c, (uint8_t)(c->ax), 1u, 8, 0)));
            c->flags = (uint16_t)((c->flags & ~1u) | carry);
            c->ip = 0x0500u;
            break;
        case 0x0500u: /* mov byte ptr [bp - 3], al */
            put(m, c->ss, (uint16_t)(c->bp - 3), 8, (uint8_t)(c->ax));
            c->ip = 0x0503u;
            break;
        case 0x0503u: /* mov al, byte ptr [bp - 1] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp - 1), 8)));
            c->ip = 0x0506u;
            break;
        case 0x0506u: /* cmp al, byte ptr [bp - 3] */
            (void)alu(c, (uint8_t)(c->ax), get(m, c->ss, (uint16_t)(c->bp - 3), 8), 8, 1);
            c->ip = 0x0509u;
            break;
        case 0x0509u: /* jl 0x543 */
            c->ip = (((c->flags >> 7) ^ (c->flags >> 11)) & 1u) ? 0x543u : 0x050Bu;
            break;
        case 0x050Bu: /* mov byte ptr [bp - 2], al */
            put(m, c->ss, (uint16_t)(c->bp - 2), 8, (uint8_t)(c->ax));
            c->ip = 0x050Eu;
            break;
        case 0x050Eu: /* jmp 0x513 */
            c->ip = 0x513u;
            break;
        case 0x0510u: /* dec byte ptr [bp - 2] */
            carry = c->flags & 1u;
            put(m, c->ss, (uint16_t)(c->bp - 2), 8, alu(c, get(m, c->ss, (uint16_t)(c->bp - 2), 8), 1u, 8, 1));
            c->flags = (uint16_t)((c->flags & ~1u) | carry);
            c->ip = 0x0513u;
            break;
        case 0x0513u: /* mov al, byte ptr [bp - 2] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp - 2), 8)));
            c->ip = 0x0516u;
            break;
        case 0x0516u: /* CBW: 16-bit opcode 98, sign-extend AL */
            c->ax = (uint16_t)((c->ax & 128u) ? (c->ax | 0xFF00u) : (c->ax & 255u));
            c->ip = 0x0517u;
            break;
        case 0x0517u: /* mov di, ax */
            c->di = (uint16_t)(c->ax);
            c->ip = 0x0519u;
            break;
        case 0x0519u: /* shl di, 1 */
            v = c->di;
            c->di = alu(c, v, v, 16, 0);
            c->flags &= (uint16_t)~16u;
            c->ip = 0x051Bu;
            break;
        case 0x051Bu: /* dec word ptr [di + 0x6dc2] */
            carry = c->flags & 1u;
            put(m, c->ds, (uint16_t)(c->di + 0x6DC2u), 16, alu(c, get(m, c->ds, (uint16_t)(c->di + 0x6DC2u), 16), 1u, 16, 1));
            c->flags = (uint16_t)((c->flags & ~1u) | carry);
            c->ip = 0x051Fu;
            break;
        case 0x051Fu: /* mov al, byte ptr [bp - 2] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp - 2), 8)));
            c->ip = 0x0522u;
            break;
        case 0x0522u: /* dec al */
            carry = c->flags & 1u;
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(alu(c, (uint8_t)(c->ax), 1u, 8, 1)));
            c->flags = (uint16_t)((c->flags & ~1u) | carry);
            c->ip = 0x0524u;
            break;
        case 0x0524u: /* CBW: 16-bit opcode 98, sign-extend AL */
            c->ax = (uint16_t)((c->ax & 128u) ? (c->ax | 0xFF00u) : (c->ax & 255u));
            c->ip = 0x0525u;
            break;
        case 0x0525u: /* mov di, ax */
            c->di = (uint16_t)(c->ax);
            c->ip = 0x0527u;
            break;
        case 0x0527u: /* shl di, 1 */
            v = c->di;
            c->di = alu(c, v, v, 16, 0);
            c->flags &= (uint16_t)~16u;
            c->ip = 0x0529u;
            break;
        case 0x0529u: /* mov dx, word ptr [di + 0x363a] */
            c->dx = (uint16_t)(get(m, c->ds, (uint16_t)(c->di + 0x363Au), 16));
            c->ip = 0x052Du;
            break;
        case 0x052Du: /* mov al, byte ptr [bp - 2] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp - 2), 8)));
            c->ip = 0x0530u;
            break;
        case 0x0530u: /* dec al */
            carry = c->flags & 1u;
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(alu(c, (uint8_t)(c->ax), 1u, 8, 1)));
            c->flags = (uint16_t)((c->flags & ~1u) | carry);
            c->ip = 0x0532u;
            break;
        case 0x0532u: /* CBW: 16-bit opcode 98, sign-extend AL */
            c->ax = (uint16_t)((c->ax & 128u) ? (c->ax | 0xFF00u) : (c->ax & 255u));
            c->ip = 0x0533u;
            break;
        case 0x0533u: /* mov di, ax */
            c->di = (uint16_t)(c->ax);
            c->ip = 0x0535u;
            break;
        case 0x0535u: /* shl di, 1 */
            v = c->di;
            c->di = alu(c, v, v, 16, 0);
            c->flags &= (uint16_t)~16u;
            c->ip = 0x0537u;
            break;
        case 0x0537u: /* add word ptr [di + 0x6dc2], dx */
            put(m, c->ds, (uint16_t)(c->di + 0x6DC2u), 16, alu(c, get(m, c->ds, (uint16_t)(c->di + 0x6DC2u), 16), c->dx, 16, 0));
            c->ip = 0x053Bu;
            break;
        case 0x053Bu: /* mov al, byte ptr [bp - 2] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp - 2), 8)));
            c->ip = 0x053Eu;
            break;
        case 0x053Eu: /* cmp al, byte ptr [bp - 3] */
            (void)alu(c, (uint8_t)(c->ax), get(m, c->ss, (uint16_t)(c->bp - 3), 8), 8, 1);
            c->ip = 0x0541u;
            break;
        case 0x0541u: /* jne 0x510 */
            c->ip = (!(c->flags & 64u)) ? 0x510u : 0x0543u;
            break;
        case 0x0543u: /* jmp 0x49f */
            c->ip = 0x49Fu;
            break;
        case 0x0546u: /* mov al, byte ptr [bp + 8] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp + 8), 8)));
            c->ip = 0x0549u;
            break;
        case 0x0549u: /* xor ah, ah */
            c->ax &= 255u;
            c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x44u);
            c->ip = 0x054Bu;
            break;
        case 0x054Bu: /* mov dx, ax */
            c->dx = (uint16_t)(c->ax);
            c->ip = 0x054Du;
            break;
        case 0x054Du: /* mov al, byte ptr [bp + 6] */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | (uint8_t)(get(m, c->ss, (uint16_t)(c->bp + 6), 8)));
            c->ip = 0x0550u;
            break;
        case 0x0550u: /* CBW: 16-bit opcode 98, sign-extend AL */
            c->ax = (uint16_t)((c->ax & 128u) ? (c->ax | 0xFF00u) : (c->ax & 255u));
            c->ip = 0x0551u;
            break;
        case 0x0551u: /* mov di, ax */
            c->di = (uint16_t)(c->ax);
            c->ip = 0x0553u;
            break;
        case 0x0553u: /* shl di, 1 */
            v = c->di;
            c->di = alu(c, v, v, 16, 0);
            c->flags &= (uint16_t)~16u;
            c->ip = 0x0555u;
            break;
        case 0x0555u: /* sub word ptr [di + 0x6dc2], dx */
            put(m, c->ds, (uint16_t)(c->di + 0x6DC2u), 16, alu(c, get(m, c->ds, (uint16_t)(c->di + 0x6DC2u), 16), c->dx, 16, 1));
            c->ip = 0x0559u;
            break;
        case 0x0559u: /* push cs */
            push(m, c, c->cs);
            c->ip = 0x055Au;
            break;
        case 0x055Au: /* call 0x3a3 */
            if (get(m, c->cs, 0x055Au, 8) != 0xE8u || get(m, c->cs, 0x055Bu, 16) != 0xFE46u) return PR_U7D_047A_CODE;
            source_cs = c->cs;
            push(m, c, 0x055Du);
            c->ip = 0x03A3u;
            status = pr_u7d_update_03a3(m, size, c);
            if (d != NULL) { d->callee = 1; d->status = status; }
            if (status != 0) return PR_U7D_047A_CALLEE;
            if (c->cs != source_cs || c->ip != 0x055Du) return PR_U7D_047A_TRANSFER;
            break;
        case 0x055Du: /* mov sp, bp */
            c->sp = (uint16_t)(c->bp);
            c->ip = 0x055Fu;
            break;
        case 0x055Fu: /* pop bp */
            c->bp = (uint16_t)(get(m, c->ss, c->sp, 16));
            c->sp = (uint16_t)(c->sp + 2u);
            c->ip = 0x0560u;
            break;
        case 0x0560u: /* retf 4 */
            v = c->sp;
            c->ip = get(m, c->ss, v, 16);
            c->cs = get(m, c->ss, (uint16_t)(v + 2u), 16);
            c->sp = (uint16_t)(v + 8u);
            return PR_U7D_047A_RETURNED;
        default: return PR_U7D_047A_CODE;
        }
    }
}
