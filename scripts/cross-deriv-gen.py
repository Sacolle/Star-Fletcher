


for w1 in ['neg', 'pos', 'center']:
    for w2 in ['neg', 'pos', 'center']:
        print(f"FP cross_deriv_{w1}_{w2}(){{")
        if w1 == 'center':
            if w2 == 'center':
                print("\tbase")

def p(m, x, y):
    block, idx = m[x][y]
    return f"{block}[{idx} + {x} * stride1 + {y} * stride2]"
            
def make_cross_comp(mat):
    return f'''
       return (
       (L11*({p(mat, +1, +1)}-{p(mat, +1, -1)}-{p(mat, -1, +1)}+{p(mat, -1, -1)})+
       L12*({p(mat, +1, +2)}-{p(mat, +1, -2)}-{p(mat, -1, +2)}+{p(mat, -1, -2)}+{p(mat, +2, +1)}-{p(mat, +2, -1)}-{p(mat, -2, +1)}+{p(mat, -2, -1)})+
       L13*({p(mat, +1, +3)}-{p(mat, +1, -3)}-{p(mat, -1, +3)}+{p(mat, -1, -3)}+{p(mat, +3, +1)}-{p(mat, +3, -1)}-{p(mat, -3, +1)}+{p(mat, -3, -1)})+
       L14*({p(mat, +1, +4)}-{p(mat, +1, -4)}-{p(mat, -1, +4)}+{p(mat, -1, -4)}+{p(mat, +4, +1)}-{p(mat, +4, -1)}-{p(mat, -4, +1)}+{p(mat, -4, -1)})+
       L22*({p(mat, +2, +2)}-{p(mat, +2, -2)}-{p(mat, -2, +2)}+{p(mat, -2, -2)})+
       L23*({p(mat, +2, +3)}-{p(mat, +2, -3)}-{p(mat, -2, +3)}+{p(mat, -2, -3)}+{p(mat, +3, +2)}-{p(mat, +3, -2)}-{p(mat, -3, +2)}+{p(mat, -3, -2)})+ 
       L24*({p(mat, +2, +4)}-{p(mat, +2, -4)}-{p(mat, -2, +4)}+{p(mat, -2, -4)}+{p(mat, +4, +2)}-{p(mat, +4, -2)}-{p(mat, -4, +2)}+{p(mat, -4, -2)})+ 
       L33*({p(mat, +3, +3)}-{p(mat, +3, -3)}-{p(mat, -3, +3)}+{p(mat, -3, -3)})+
       L34*({p(mat, +3, +4)}-{p(mat, +3, -4)}-{p(mat, -3, +4)}+{p(mat, -3, -4)}+{p(mat, +4, +3)}-{p(mat, +4, -3)}-{p(mat, -4, +3)}+{p(mat, -4, -3)})+
       L44*({p(mat, +4, +4)}-{p(mat, +4, -4)}-{p(mat, -4, +4)}+{p(mat, -4, -4)}))*(dinv))
    '''