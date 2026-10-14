#include "poolrad/recovered/resident_time_caller_return_08ae_002a.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    pr_rec_res_08ae_001f_cpu at_call = {
        0x10d2u, 0x002au, 0x14a0u, 0x1d16u, 0x1d16u, 0x3d9au,
        0x3da6u, 0x0001u, 0x0005u, 0x7e40u, 0x0000u, 0x0001u,
        0x3da2u, 0x7246u
    };
    pr_rec_res_08ae_001f_cpu at_return = {
        0x10d2u, 0x002fu, 0x14a0u, 0x1d16u, 0x1d16u, 0x3d9au,
        0x3da6u, 0x7e40u, 0x0005u, 0x0000u, 0x0005u, 0x0001u,
        0x0000u, 0x7206u
    };
    uint8_t return_frame[4] = {0xa0u, 0x3du, 0x16u, 0x1du};
    uint8_t wrapped_frame[4] = {0u, 0u, 0u, 0u};
    pr_rec_res_08ae_001f_cpu wrapped = at_call;

    pr_rec_res_08ae_002a_call_multiply(&at_call, return_frame, 0x131cu);
    assert(at_call.cs == 0x131cu && at_call.ip == 0x0a5bu);
    assert(at_call.sp == 0x3d96u && at_call.flags == 0x7246u);
    assert(at_call.ax == 1u && at_call.cx == 0x7e40u);
    assert(return_frame[0] == 0x2fu && return_frame[1] == 0u);
    assert(return_frame[2] == 0xd2u && return_frame[3] == 0x10u);

    wrapped.cs = 0x3456u;
    wrapped.sp = 0x0002u;
    pr_rec_res_08ae_002a_call_multiply(&wrapped, wrapped_frame, 0x789au);
    assert(wrapped.cs == 0x789au && wrapped.ip == 0x0a5bu);
    assert(wrapped.sp == 0xfffeu);
    assert(wrapped_frame[0] == 0x2fu && wrapped_frame[1] == 0u);
    assert(wrapped_frame[2] == 0x56u && wrapped_frame[3] == 0x34u);

    pr_rec_res_08ae_002f_copy_result(&at_return);
    assert(at_return.cs == 0x10d2u && at_return.ip == 0x0031u);
    assert(at_return.ax == 0x7e40u && at_return.cx == 0x7e40u);
    assert(at_return.dx == 5u && at_return.sp == 0x3d9au);
    assert(at_return.flags == 0x7206u);

    at_return.ax = 0xa55au;
    at_return.cx = 0x1234u;
    at_return.ip = 0x002fu;
    pr_rec_res_08ae_002f_copy_result(&at_return);
    assert(at_return.cx == 0xa55au && at_return.ip == 0x0031u);
    assert(at_return.ax == 0xa55au && at_return.flags == 0x7206u);
    return 0;
}
