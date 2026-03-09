# Star-Fletcher

Star-Fletcher é uma implementação da propagação de onda com o tempo do algoritmo de Reverse Time Migration (RTM) [Fletcher](https://doi.org/10.1190/1.3269902).
Ele utiliza paralelismo de tarefas em primeira ordem com o framework [StarPU](https://starpu.gitlabpages.inria.fr/). 
O flake [nix-starpu](https://github.com/Sacolle/nix-starpu) é usado para compilar esse programa em sistemas nix.


## Execução

Este é um projeto nix, então se tiver o package manager instalado ou usar o OS, 
basta rodar `nix develop` para entrar no ambiente de desenvolvimento. Após isso,
`make` compila o código e `make run` roda com uma entrada de exemplo.
