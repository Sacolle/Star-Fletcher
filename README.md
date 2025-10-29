# Metas com a aplicação funcionando

O Objetivo agora é portar a aplicação [fletcher](https://github.com/gabrielfrtg/fletcher-base) para esse modelo 3d do StarPU e validar numericamente com as implementações já existentes. Neste primeiro momento, a aplicação será unicamente executada na CPU. Tendo executado e os testes realizados no PCAD, pode-se então adicionar as implementações com GPGPU e as possíveis otimizações pensadas, como separação miolo e borda.

Um desafio agora é fazer o programa gerar uma saída e ler essa saída. A geração está no âmbito de tentativa e erro. A leitura precisa de entendimento de como as ferramentas [Madagascar](https://ahay.org/wiki/Main_Page) funcionam.

```c
// assumindo s11 sendo o stride de x e s21 sendo o stride em Y
// com uma notação vetorial tem a redução do https://github.com/gabrielfrtg/fletcher-base/blob/main/original/derivatives.h
DerCross(p, i, strideX, strideY) 
L11 * (
    p[i + (1,1,0)] - p[i + (-1,1,0)] - p[i + (1,-1,0)] + p[i + (-1,-1,0)]
) +
L12 * (
    p[i + (2,1,0)] - p[i + (-2,1,0)] - p[i +(2,-1,0)] + p[i + (-2,-1,0)] + 
    p[i + (1,2,0)] - p[i + (-1,2,0)] - p[i +(1,-2,0)] + p[i + (-1,-2,0)]
) + 
L13 * (
    p[i + (3,1,0)] - p[i + (-3,1,0)] - p[i +(3,-1,0)] + p[i + (-3,-1,0)] + 
    p[i + (1,3,0)] - p[i + (-1,3,0)] - p[i +(1,-3,0)] + p[i + (-1,-3,0)]
) +
L14 * (
    p[i + (4,1,0)] - p[i + (-4,1,0)] - p[i +(4,-1,0)] + p[i + (-4,-1,0)] + 
    p[i + (1,4,0)] - p[i + (-4,3,0)] - p[i +(1,-4,0)] + p[i + (-1,-4,0)]
) +
L22 * (
    p[i + (2,2,0)] - p[i + (-2,2,0)] - p[i +(2,-2,0)] + p[i + (-2,-2,0)]
) + 
L23 * (
    p[i + (3,2,0)] - p[i + (-3,2,0)] - p[i +(3,-2,0)] + p[i + (-3,-2,0)] + 
    p[i + (2,3,0)] - p[i + (-2,3,0)] - p[i +(2,-3,0)] + p[i + (-2,-3,0)]
) + 
L24 * (
    p[i + (4,2,0)] - p[i + (-4,2,0)] - p[i +(4,-2,0)] + p[i + (-4,-2,0)] + 
    p[i + (2,4,0)] - p[i + (-2,4,0)] - p[i +(2,-4,0)] + p[i + (-2,-4,0)]
) +
L33 * (
    p[i + (3,3,0)] - p[i + (-3,3,0)] - p[i +(3,-3,0)] + p[i + (-3,-3,0)]
) +
L34 * (
    p[i + (4,3,0)] - p[i + (-4,3,0)] - p[i +(4,-3,0)] + p[i + (-4,-3,0)] +
    p[i + (3,4,0)] - p[i + (-3,4,0)] - p[i +(3,-4,0)] + p[i + (-3,-4,0)] 
) +
L44 * (
    p[i + (4,4,0)] - p[i + (-4,4,0)] - p[i +(4,-4,0)] + p[i + (-4,-4,0)]
) 
```