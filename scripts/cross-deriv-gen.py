from functools import partial

def tab(text, amount):
    return '\n'.join([('\t' * amount) + t for t in text.splitlines()])

def depth_calc(w1, w2):
    out = ''
    match w1:
        case 'neg':
            out += f'''
const int depth1 = dir1;
const size_t border_idx_1 = flip(0, base_idx - depth1 * stride1, stride1, cube_width);
'''
        case 'pos':
            out += f'''
const int depth1 = cube_width - 1 - dir1;
const size_t border_idx_1 = flip(cube_width - 1, base_idx + depth1 * stride1, stride1, cube_width);
'''
    match w2:
        case 'neg':
            out += f'''
const int depth2 = dir2;
const size_t border_idx_2 = flip(0, base_idx - depth2 * stride2, stride2, cube_width);
'''
        case 'pos':
            out += f'''
const int depth2 = cube_width - 1 - dir2;
const size_t border_idx_2 = flip(cube_width - 1, base_idx + depth2 * stride2, stride2, cube_width);
'''
    if w1 == 'center' or w2 == 'center':
        return out
    
    match w2:
        case 'neg':
            out += f'''
const size_t border_idx_3 = flip(0, border_idx_1 - depth2 * stride2, stride2, cube_width);
'''
        case 'pos':
            out += f'''
const size_t border_idx_3 = flip(cube_width - 1, border_idx_1 + depth2 * stride2, stride2, cube_width);
'''
    return out

def is_case(val, w):
    match w:
        case 'pos':
            return val > 0
        case 'neg':
            return val < 0
        case 'center':
            return False

def compute_case(x, y, depth_x, depth_y, case1, case2):
    dist_x = abs(x) - depth_x
    dist_y = abs(y) - depth_y
    change_x = is_case(x, case1) and dist_x > 0 # nums go from -4 to 4
    change_y = is_case(y, case2) and dist_y > 0
    match (change_x, change_y):
        case (False, False):
            return ('block', 'base_idx', x, y)
        case (True, False):
            return (f'block_{case1}_center', 'border_idx_1', dist_x - 1, y)
        case (False, True):
            return (f'block_center_{case2}', 'border_idx_2', x, dist_y - 1)
        case (True, True):
            return (f"block_{case1}_{case2}", 'border_idx_3', dist_x - 1, dist_y - 1)

def function_impl(w1, w2, lamb):
    out = ''
    if w1 == 'center':
        if w2 == 'center':
            out += tab(make_cross_comp(build_mat(4, 4, lamb)), 1)
        else:
            out += 'switch (depth2){'
            for d2 in range(0, 4):
                out += f'''
    case {d2}:
{tab(make_cross_comp(build_mat(4, d2, lamb)), 2)}'''
            out += '\n\tdefault: UNREACHABLE;\n\t}'
    else:
        if w2 == 'center':
            out += '''switch (depth1){'''
            for d1 in range(0, 4):
                out += f'''
    case {d1}:
{tab(make_cross_comp(build_mat(d1, 4, lamb)),2)}'''
            out += '\n\tdefault: UNREACHABLE;\n\t}'
        else:
            out += function_impl_full(lamb)
            
    return out


def function_impl_full(lamb):
    out = '''switch (BITMASK_PAIR(depth1, depth2)){'''
    for d1 in range(0, 4):
        for d2 in range(0, 4):
            out += f'''
    case BITMASK_PAIR({d1}, {d2}):
{tab(make_cross_comp(build_mat(d1, d2, lamb)), 2)}'''
    out += '\n\tdefault: UNREACHABLE; \n\t}'
    return out

def build_mat(d1, d2, lamb):
    mat = [[None] * 9 for _ in range(9)]
    for y in range(-4, 5):
        if y == 0:
            continue
        for x in range(-4, 5):
            if x == 0:
                continue
            mat[y + 4][x + 4] = lamb(x, y, d1, d2)
    return mat
        

def p(m, x, y):
    block, idx, n_x, n_y = m[y + 4][x + 4]
    return f"{block}[{idx} + ({n_x} * stride1) + ({n_y} * stride2)]"
            
def make_cross_comp(mat):
    return f'''return (
    (L11*({p(mat, +1, +1)}-{p(mat, +1, -1)}-{p(mat, -1, +1)}+{p(mat, -1, -1)})+
    L12*({p(mat, +1, +2)}-{p(mat, +1, -2)}-{p(mat, -1, +2)}+{p(mat, -1, -2)}+{p(mat, +2, +1)}-{p(mat, +2, -1)}-{p(mat, -2, +1)}+{p(mat, -2, -1)})+
    L13*({p(mat, +1, +3)}-{p(mat, +1, -3)}-{p(mat, -1, +3)}+{p(mat, -1, -3)}+{p(mat, +3, +1)}-{p(mat, +3, -1)}-{p(mat, -3, +1)}+{p(mat, -3, -1)})+
    L14*({p(mat, +1, +4)}-{p(mat, +1, -4)}-{p(mat, -1, +4)}+{p(mat, -1, -4)}+{p(mat, +4, +1)}-{p(mat, +4, -1)}-{p(mat, -4, +1)}+{p(mat, -4, -1)})+
    L22*({p(mat, +2, +2)}-{p(mat, +2, -2)}-{p(mat, -2, +2)}+{p(mat, -2, -2)})+
    L23*({p(mat, +2, +3)}-{p(mat, +2, -3)}-{p(mat, -2, +3)}+{p(mat, -2, -3)}+{p(mat, +3, +2)}-{p(mat, +3, -2)}-{p(mat, -3, +2)}+{p(mat, -3, -2)})+ 
    L24*({p(mat, +2, +4)}-{p(mat, +2, -4)}-{p(mat, -2, +4)}+{p(mat, -2, -4)}+{p(mat, +4, +2)}-{p(mat, +4, -2)}-{p(mat, -4, +2)}+{p(mat, -4, -2)})+ 
    L33*({p(mat, +3, +3)}-{p(mat, +3, -3)}-{p(mat, -3, +3)}+{p(mat, -3, -3)})+
    L34*({p(mat, +3, +4)}-{p(mat, +3, -4)}-{p(mat, -3, +4)}+{p(mat, -3, -4)}+{p(mat, +4, +3)}-{p(mat, +4, -3)}-{p(mat, -4, +3)}+{p(mat, -4, -3)})+
    L44*({p(mat, +4, +4)}-{p(mat, +4, -4)}-{p(mat, -4, +4)}+{p(mat, -4, -4)}))*(dinv)
);'''


def main():
    print(f'''/*
    Este é um arquivo gerado automaticamente pelo script `cross-deriv-gen.py`
    A meta é lidar discernir os casos antes da computação, de forma que na hora do cáculo, 
    este possa tomar uma forma idêntica a do fletcher base.
          
    Na pipeline de compilação, ele é incluido no arquivo derivatives.c, que diferencia os casos
    entre os nove quadrantes definidos
*/
#include <assert.h>
          
#ifdef RELEASE        
#define UNREACHABLE __builtin_unreachable()
#else
#define UNREACHABLE assert(0 && "Unreachable!"); return 0.0
#endif
          
#include "floatingpoint.h"
#include "derivatives.h"
          
#define MAX_MASK_NUM 4
''')
    # default_case = make_cross_comp(build_mat('center', 'center', partial(compute_case, case1='center', case2='center')))
    for w1 in ['neg', 'pos', 'center']:
        for w2 in ['neg', 'pos', 'center']:
            curr_lambda = partial(compute_case, case1=w1, case2=w2)
            print(f'''
FP cross_deriv_{w1}_{w2}(
    const FP* block, const size_t base_idx,
    const size_t dir1, const FP* block_neg_center, const FP* block_pos_center, const int stride1,
    const size_t dir2, const FP* block_center_neg, const FP* block_center_pos, const int stride2,
    const FP* block_pos_pos, const FP* block_pos_neg, 
    const FP* block_neg_pos, const FP* block_neg_neg,
    const int cube_width, const FP dinv
){{{tab(depth_calc(w1, w2), 1)}
    {function_impl(w1, w2, curr_lambda)}
}}''')
    print('#undef MAX_MASK_NUM')

main()