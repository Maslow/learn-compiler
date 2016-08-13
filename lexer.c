
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ID_MAX 32

#define TT_KEYWORD 0
#define TT_OPERATOR 100
#define TT_ID 200
#define TT_L_BRACE 300
#define TT_R_BRACE 301
#define TT_L_PARENTHESE 302
#define TT_R_PARENTHESE 303
#define TT_L_ANGLE_BRACKET 304
#define TT_R_ANGLE_BRACKET 305
#define TT_CONST_STRING 400
#define TT_CONST_NUMBER 410
#define TT_CONST_CHAR 420
#define TT_SEMICOLON 500

const char *keywords[] = {
    "void", "int",  "char", "float", "double", "struct", "typedef",  "return",
    "if",   "else", "do",   "while", "goto",   "NULL",   "#include", "#define"};

typedef struct {
  char text[ID_MAX];
  int type;
  int code;
} token_t;

typedef struct __token_node_t {
  token_t t;
  struct __token_node_t *next;
} token_node_t;

typedef struct __token_list_t {
  token_node_t *root;
  token_node_t *tail;
} token_list_t;

void token_list_append(token_list_t *l, token_t t) {
  token_node_t *pn = (token_node_t *)malloc(sizeof(token_node_t));
  pn->t = t;
  pn->next = NULL;
  l->tail = (l->root == NULL) ? (l->root = pn) : (l->tail->next = pn);
}

void token_list_print(token_node_t *ptn, FILE *fout) {
  if (ptn == NULL)
    return;
  token_t *pt = &(ptn->t);
  fprintf(fout, "<`%s`,%d> ", pt->text, pt->type);
  token_list_print(ptn->next, fout);
}

token_t new_token(char text[ID_MAX], int type, int code) {
  token_t t;
  strcpy(t.text, text);
  t.type = type;
  t.code = code;
  return t;
}

int is_letter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int is_whitespace(char ch) { return ch == '\t' || ch == '\n' || ch == ' '; }

int is_digital(char ch) { return ch >= '0' && ch <= '9'; }

int is_id(char c, FILE *fi, token_list_t *tkl) {
  char name[ID_MAX];
  size_t i = 0;
  while ((is_letter(c) || c == '_' || is_digital(c)) && i < ID_MAX - 1) {
    name[i++] = c;
    c = getc(fi);
  }
  ungetc(c, fi);
  if (is_letter(c) || c == '_' || is_digital(c)) {
    printf("id's length is too long > %d\n", ID_MAX);
    exit(0);
  }
  name[i] = '\0';
  token_list_append(tkl, new_token(name, TT_ID, -1));
  return 1;
}

int is_keyword(char c, FILE *fi, token_list_t *tkl) {
  long fpos = ftell(fi);
  char name[ID_MAX];
  size_t i = 0;
  while (i < ID_MAX - 1 && (is_letter(c) || (i == 0 && c == '#'))) {
    name[i++] = c;
    c = getc(fi);
  }
  ungetc(c, fi);
  if (i == 0 || is_letter(c))
    goto fail;
  name[i] = '\0';
  for (size_t k = 0; k < sizeof(keywords) / sizeof(char *); k++) {
    size_t j;
    for (j = 0; j < i; j++)
      if (name[j] != keywords[k][j])
        break;

    if (j == i && keywords[k][j] == '\0') {
      token_list_append(tkl, new_token(name, TT_KEYWORD, -1));
      return 1;
    }
  }
fail:
  fseek(fi, fpos, SEEK_SET);
  return 0;
}

void scan(FILE *fsource, token_list_t *tkl) {
  int line = 0;
  char ch = getc(fsource);
  while (ch != EOF) {
    if (ch == '\n') {
      line++;
      goto next;
    }
    if (is_whitespace(ch))
      goto next;

    if (is_digital(ch))
      goto next;

    if ((is_letter(ch) || ch == '#') && is_keyword(ch, fsource, tkl))
      goto next;

    if ((is_letter(ch) || ch == '_') && is_id(ch, fsource, tkl))
      goto next;

    if (ch == '+' || ch == '-' || ch == '/' || ch == '*' || ch == '|' ||
        ch == '?' || ch == '~' || ch == '&' || ch == '!' || ch == '>' ||
        ch == '<' || ch == '=' || ch == ',' || ch == '.' || ch == '[' ||
        ch == ']') {
      char name[2] = {ch, '\0'};
      token_list_append(tkl, new_token(name, TT_OPERATOR, -1));
      goto next;
    }
    if (ch == '(') {
      token_list_append(tkl, new_token("(", TT_L_PARENTHESE, -1));
      goto next;
    }
    if (ch == ')') {
      token_list_append(tkl, new_token(")", TT_R_PARENTHESE, -1));
      goto next;
    }
    if (ch == '{') {
      token_list_append(tkl, new_token("{", TT_L_BRACE, -1));
      goto next;
    }
    if (ch == '}') {
      token_list_append(tkl, new_token("}", TT_R_BRACE, -1));
      goto next;
    }
    if (ch == ';') {
      token_list_append(tkl, new_token(";", TT_SEMICOLON, -1));
      goto next;
    }
  next:
    ch = getc(fsource);
  }
}

int main(int argc, char const *argv[]) {
  FILE *fsource;
  FILE *ftokens;
  token_list_t tkl;
  tkl.root = NULL;
  tkl.tail = NULL;

  if (argc < 2) {
    fprintf(stderr, "usage: lexer <source file> [output file]\n");
    exit(1);
  } else if (argc == 2)
    ftokens = stdout;
  else if ((ftokens = fopen(argv[2], "w")) == NULL) {
    fprintf(stderr, "cannot open : %s\n", argv[3]);
    exit(1);
  };
  if ((fsource = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "cannot open : %s\n", argv[1]);
    exit(1);
  }
  scan(fsource, &tkl);
  token_list_print(tkl.root, ftokens);
  fclose(fsource);
  fclose(ftokens);
  return 0;
}
