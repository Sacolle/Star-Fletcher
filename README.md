# Metas com a aplicação funcionando

O Objetivo agora é portar a aplicação [fletcher](https://github.com/gabrielfrtg/fletcher-base) para esse modelo 3d do StarPU e validar numericamente com as implementações já existentes. Neste primeiro momento, a aplicação será unicamente executada na CPU. Tendo executado e os testes realizados no PCAD, pode-se então adicionar as implementações com GPGPU e as possíveis otimizações pensadas, como separação miolo e borda.

Um desafio agora é fazer o programa gerar uma saída e ler essa saída. A geração está no âmbito de tentativa e erro. A leitura precisa de entendimento de como as ferramentas [Madagascar](https://ahay.org/wiki/Main_Page) funcionam.