# Metas com a aplicação funcionando

O Objetivo agora é portar a aplicação [fletcher](https://github.com/gabrielfrtg/fletcher-base) para esse modelo 3d do StarPU e validar numericamente com as implementações já existentes. Neste primeiro momento, a aplicação será unicamente executada na CPU. Tendo executado e os testes realizados no PCAD, pode-se então adicionar as implementações com GPGPU e as possíveis otimizações pensadas, como separação miolo e borda.

Um desafio agora é fazer o programa gerar uma saída e ler essa saída. A geração está no âmbito de tentativa e erro. A leitura precisa de entendimento de como as ferramentas [Madagascar](https://ahay.org/wiki/Main_Page) funcionam.

## Execução

Para compilar o código, basta executar 
```
make
```

Uma versão com parâmetros base pode ser executada com:
```
make run
```
Que logo depois executa o script que junta todos os chunks e gera o arquivo de saída `out-TTI.rsf@` para complementar o `out-TTI.rsf`.
Com ele pode-se chamar `./scripts/visuzalize.sh ./result/out-TTI.rsf` para ver a visualização da onda.

> como o arquivo `.rsf` aponta para um arquivo `./result/*.rsf@`, deve-se sempre passá-lo estando no diretório acima de result (`../result`)


## Formulação

Reformulação das iterações quebrando a dependência RW:

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