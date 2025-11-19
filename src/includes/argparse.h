#ifndef _ARGPARSE_GUARD
#define _ARGPARSE_GUARD

#define ARG_i32 1
#define ARG_i64 2
#define ARG_u32 3
#define ARG_u64 4
#define ARG_f32 5
#define ARG_f64 6
#define ARG_str 7

#define ARG_usize 8

// TODO: mover para uma classe de string, que dai pode ser mais genérico?
// e dai pode-se usar um parâmetro de saída que é oculto na implementação dos casos específicos

// retorna o `int` seguinte a string passada nos argumentos variádicos se 
// `word` for igual a ele.
// count deve ser o número de opções passadas, sendo cada opção
// uma string seguida de um int
// str_to_enum("b", 3, "a", 21, "b", 67, "c", 47) -> 67
// se err != 0, indica erro na função
int str_to_enum(const char* word, int* err, int count, ...);

// dado uma sequência de `count` ARG_type tag, pointeiro para o valor,
// realiza-se a leitura desses argumentos usando argv, realizando as checagem apropriadas
// caso não seja possível, retorna um err != 0, que é composto de duas partes:
// os 3 primeiros bits são o código de erro e o restantante é o índice do erro
int read_args(int argc, char** argv, int count, ...);

// get the index part of the error
int get_parse_errors_local(int err);

// get the name of the error
char* get_parse_errors_name(int err);

#endif