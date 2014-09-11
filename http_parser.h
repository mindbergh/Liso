#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H


extern int yylex(void);
extern char *yytext;
extern void scan_string(const char *str);
extern FILE* yyin;
extern int yylex_destroy(void);



#endif

