#ifndef YYSTYPE
#define YYSTYPE int
#endif
#define	NAME	257
#define	INTEGER	258

#ifdef SC_WIN32
# define FLOAT_COMPAT 259  
#else
# define	FLOAT	259
#endif

#define	ACCIDENTAL	260
#define	SYMBOL	261
#define	STRING	262
#define	ASCII	263
#define	PRIMITIVENAME	264
#define	CLASSNAME	265
#define	CURRYARG	266
#define	VAR	267
#define	ARG	268
#define	CLASSVAR	269
#ifdef SC_WIN32
# define CONST_COMPAT 270
#else
# define	CONST	270
#endif
#define	NILOBJ	271
#define	TRUEOBJ	272
#define	FALSEOBJ	273
#define	PSEUDOVAR	274
#define	ELLIPSIS	275
#define	DOTDOT	276
#define	PIE	277
#define	BEGINCLOSEDFUNC	278
#define	BADTOKEN	279
#define	INTERPRET	280
#define	BEGINGENERATOR	281
#define	LEFTARROW	282
#define	WHILE	283
#define	BINOP	284
#define	KEYBINOP	285
#define	READWRITEVAR	286
#define	UMINUS	287


extern YYSTYPE yylval;
