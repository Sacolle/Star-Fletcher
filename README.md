# Metas com a aplicação funcionando

O Objetivo agora é portar a aplicação [fletcher](https://github.com/gabrielfrtg/fletcher-base) para esse modelo 3d do StarPU e validar numericamente com as implementações já existentes. Neste primeiro momento, a aplicação será unicamente executada na CPU. Tendo executado e os testes realizados no PCAD, pode-se então adicionar as implementações com GPGPU e as possíveis otimizações pensadas, como separação miolo e borda.

Um desafio agora é fazer o programa gerar uma saída e ler essa saída. A geração está no âmbito de tentativa e erro. A leitura precisa de entendimento de como as ferramentas [Madagascar](https://ahay.org/wiki/Main_Page) funcionam.


# Problema (ramblings of a madman)

O algoritmo do kernel do fletcher não apenas escreve na iteração futura, mas lê ela também para obter a iteração seguinte.
O funcionamente é: dadas iterações A e B, lê-se A e realiza-se B := ops(A) - B. A e B então são depências de leitura e B é a dependência de escrita. O problema vem que para realizar a próxima iteração, troca-se A e B de lugar. Então o valor da computação é escrito sobre o valor previamente computado que não pode ser descartado. Uma demonstação disso é o seguinte, assumindo ops(A) = 2 * A + 1:
1. A = 1, B = ops(A) - B -> A = 1, B = 2 <|> (swap) A = 2, B = 1
2. A = 2, B = ops(A) - B -> A = 2, B = 5 <|> (swap) A = 5, B = 2
3. A = 5, B = ops(A) - B -> A = 5, B = 11 <|> (swap) A = 11, B = 5
...

Se A fosse a única dependêcia de leitura, e B de escrita, teriamos:
B := F(A) -> A :=: B.
B := F(F(A)) -> A :=: B.
B := F(F(F(A))) -> A :=: B.
B := F(F(F(F(A)))) -> A :=: B.
em que o B nunca é lido diretamente, só como A depois do swap

1. A = 1, B = ops(A) -> A = 1, B = 3 <|> (swap) A = 3, B = \
Em que não importa o valor de B salvo, pois ele só depende de A. Isso resulta em uma cadeia de tarefas:
A -> A -> A -> A -> ... 
No qual B é só um intermediário e a implementação em flecha fica muito mais simples.

Porém com B sendo uma depedência de leitura também, nenhum elemento da iteração pode ser descartado
B := F(A) - B -> A :=: B.
B := F(F(A) - B) - B -> A :=: B.
B := F(F(F(A) - B) - B) - B -> A :=: B.
B := F(F(F(F(A) - B) - B) - B) - B -> A :=: B.


## TLDR:

Não é tão simples como na aplicação da média gausiana o reaproveitamento dos cubos. Isso pois para calcular um cubo
C(i, j, k, t) é necssário não só dos vizinhos e do cubo C(i, j, k, t - 1), 
como o cubo C(i, j, k, t) deve ser conter o valor do cubo C(i, j, k, t - 2). 

Essa ultima fraze tem que ser validada meio que algebrigamente antes que eu me sinta seguro de fazer o próximo passo.

## Formulação

Formulação base:
$$
\begin{align}
    & B := F(A) - B \\
    & A\; swap \; B
\end{align}
$$

Com isso, pode-se formular a versão usando a notação com $t$, sendo $A_t$ o valor de $A$ na iteração $t$.
$$
\begin{align}
    & B' = F(A_{t - 1}) - B_{t - 1} \\
    & A_t = B' \\
    & B_t = A_{t - 1} 
\end{align}
$$
As expressões (4) e (5) representam o _swap_, em que $B_t$ assume o valor de $A$ usado na sua computação ($A_{t - 1}$), neste caso representado por $B'$, e $A_t$ recebe esse $B$ computado.

Agora, pode-se quebrar a dependência em $B$ ao realizar um passo em segunda ordem. Se $B_t$ é igual a $A_{t - 1}$, então $B_{t - 1} = A_{t - 2}$. Assim, simplificando (4) com (3) e subtituindo com (5) para $t - 1$, reduz-se a computação para: 
$$
\begin{align}
    & A_t = F(A_{t - 1}) - A_{t - 2} \\
\end{align}
$$


### Inicialização

No modelo atual, os buffers `pp` (B), `pc` (A), `qp` (B) e `qc` (A) são inicializados zerados. A perturbação é inserida em um buffer do tipo (A). Inserindo isso na formulação apenas baseada em A, o valor incial $A_0$ é completamente zerado, e o valor $A_1$ também, com exeção da perturabação inserida. Assim, tem-se:
$$
\begin{align}
    & A_2 = F(A_{1}) - A_0 \\
    & A_0 = 0^n \\
    & A_1 = 0^n + perturb
\end{align}
$$