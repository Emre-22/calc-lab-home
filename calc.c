// Emre Elaziz 251ADB139
// Repository: https://github.com/Emre-22/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: OpenAI Codex (ChatGPT), assignment and supplied calc-tests; see AI_HISTORY.md.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define NAME "emre"
#define LASTNAME "elaziz"
#define STUDENT "251ADB139"

typedef struct { long long i; double d; int floating; size_t pos; } Value;
typedef struct { int kind; size_t pos; } Operator;
typedef struct {
    Value *values; Operator *ops;
    size_t nv, no, cv, co, error;
} Parser;

static void fatal(const char *message) {
    fprintf(stderr, "calc: %s\n", message);
    exit(EXIT_FAILURE);
}
static void *resize(void *ptr, size_t count, size_t width) {
    void *p;
    if (count > (size_t)-1 / width) fatal("input too large");
    p = realloc(ptr, count * width);
    if (!p) fatal("out of memory");
    return p;
}
static size_t grow(size_t n) {
    if (n > (size_t)-1 / 2) fatal("input too large");
    return n ? n * 2 : 64;
}
static void push_value(Parser *p, Value v) {
    if (p->nv == p->cv) {
        p->cv = grow(p->cv);
        p->values = resize(p->values, p->cv, sizeof *p->values);
    }
    p->values[p->nv++] = v;
}
static void push_op(Parser *p, int kind, size_t pos) {
    if (p->no == p->co) {
        p->co = grow(p->co);
        p->ops = resize(p->ops, p->co, sizeof *p->ops);
    }
    p->ops[p->no++] = (Operator){kind, pos};
}
static int precedence(int op) {
    return op == '^' ? 3 : (op == '*' || op == '/') ? 2 : 1;
}
/* AI (OpenAI Codex): prompt: "Do all the tasks that are required to get a
   grade of 10. Ensure everything works, running the tests as instructed.
   Provide the final files." Full provided prompt and disclosure: AI_HISTORY.md.
   The explicit type flag preserves integer division until either operand is
   floating. RHS positions survive reductions so zero division and negative
   exponents point to the operand, while nonfinite results point to the operator.
   Integer overflow is excluded by the assignment's input contract. */
static void reduce(Parser *p) {
    Operator op = p->ops[--p->no];
    Value b = p->values[--p->nv], a = p->values[--p->nv];
    Value r = {0, 0.0, a.floating || b.floating, a.pos};
    double x = a.floating ? a.d : (double)a.i;
    double y = b.floating ? b.d : (double)b.i;
    if (op.kind == '/' && (b.floating ? b.d == 0.0 : b.i == 0)) {
        p->error = b.pos; return;
    }
    if (op.kind == '^' && !r.floating && b.i < 0) {
        p->error = b.pos; return;
    }
    if (r.floating) {
        switch (op.kind) {
            case '+': r.d = x + y; break;
            case '-': r.d = x - y; break;
            case '*': r.d = x * y; break;
            case '/': r.d = x / y; break;
            default: r.d = pow(x, y); break;
        }
        /* Force double rounding on x87 targets before checking finiteness. */
        { volatile double rounded = r.d;
          r.d = rounded;
          if (!isfinite(rounded)) { p->error = op.pos; return; } }
    } else {
        switch (op.kind) {
            case '+': r.i = a.i + b.i; break;
            case '-': r.i = a.i - b.i; break;
            case '*': r.i = a.i * b.i; break;
            case '/': r.i = a.i / b.i; break;
            default: {
                long long base = a.i, exponent = b.i;
                r.i = 1;
                /* Avoid an unnecessary final square: it could overflow even
                   when the requested result itself is representable. */
                while (exponent) {
                    if (exponent & 1) r.i *= base;
                    exponent /= 2;
                    if (exponent) base *= base;
                }
                break;
            }
        }
    }
    push_value(p, r);
}
/* AI (OpenAI Codex), same prompt, AI_HISTORY.md: tokenizer/parser design.
   A byte index plus the line's absolute offset counts comments and newlines
   without rescanning. The token is consumed only when grammar allows it, so a
   later bad character cannot hide an earlier syntax or arithmetic error.
   Operator stacks implement expr/term/power precedence. Equal precedence is
   reduced for +,-,*,/ but retained for **, giving 2**(3**2).
   Parentheses use the heap stack too, avoiding call-stack limits on long input. */
static int evaluate(Parser *p, char *s, size_t n, size_t offset, Value *answer) {
    size_t k = 0;
    int operand = 1;
    p->nv = p->no = p->error = 0;
    while (k < n && (s[k] == ' ' || s[k] == '\t')) ++k;
    if (k == n || s[k] == '#') return 0;
    for (;;) {
        size_t pos;
        int c;
        while (k < n && (s[k] == ' ' || s[k] == '\t')) ++k;
        pos = offset + k + 1;
        c = k < n ? (unsigned char)s[k] : 0;
        if (operand) {
            if (k < n && c == '(') { push_op(p, c, pos); ++k; continue; }
            if (k < n && c >= '0' && c <= '9') {
                size_t start = k;
                Value v = {0, 0.0, 0, pos};
                char saved;
                while (k < n && s[k] >= '0' && s[k] <= '9') ++k;
                if (k + 1 < n && s[k] == '.' && s[k+1] >= '0' && s[k+1] <= '9') {
                    v.floating = 1; ++k;
                    while (k < n && s[k] >= '0' && s[k] <= '9') ++k;
                }
                saved = s[k]; s[k] = '\0'; errno = 0;
                if (v.floating) { volatile double rounded = strtod(s + start, NULL); v.d = rounded; }
                else v.i = strtoll(s + start, NULL, 10);
                s[k] = saved;
                if ((!v.floating && errno == ERANGE) || (v.floating && !isfinite(v.d))) {
                    p->error = pos; return -1;
                }
                push_value(p, v); operand = 0; continue;
            }
            /* An expected primary cannot be empty, unary, or at end of line. */
            p->error = pos; return -1;
        }
        if (k == n || c == ')') {
            while (p->no && p->ops[p->no-1].kind != '(') {
                reduce(p); if (p->error) return -1;
            }
            if (k == n) {
                if (p->no) { p->error = pos; return -1; }
                *answer = p->values[0]; return 1;
            }
            if (!p->no) { p->error = pos; return -1; }
            /* A grouped operand starts at '(' for division/exponent errors. */
            p->values[p->nv-1].pos = p->ops[--p->no].pos;
            ++k; continue;
        }
        if (c == '+' || c == '-' || c == '*' || c == '/') {
            int op = c;
            size_t width = 1;
            if (c == '*' && k+1 < n && s[k+1] == '*') { op = '^'; width = 2; }
            while (p->no && p->ops[p->no-1].kind != '(' &&
                   (precedence(p->ops[p->no-1].kind) > precedence(op) ||
                    (precedence(p->ops[p->no-1].kind) == precedence(op) && op != '^'))) {
                reduce(p); if (p->error) return -1;
            }
            push_op(p, op, pos); k += width; operand = 1; continue;
        }
        p->error = pos; return -1;
    }
}
static char *join(const char *a, const char *b) {
    size_t x = strlen(a), y = strlen(b);
    char *s;
    if (x > (size_t)-1 - y - 2) fatal("path too long");
    s = resize(NULL, x+y+2, 1);
    memcpy(s, a, x); s[x] = '/'; memcpy(s+x+1, b, y+1);
    return s;
}
static char *base_name(const char *path, int strip_txt) {
    size_t end = strlen(path), start, n;
    char *s;
    while (end > 1 && (path[end-1] == '/' || path[end-1] == '\\')) --end;
    start = end;
    while (start && path[start-1] != '/' && path[start-1] != '\\') --start;
    n = end-start;
    if (strip_txt && n >= 4 && memcmp(path+end-4, ".txt", 4) == 0) n -= 4;
    s = resize(NULL, n+1, 1); memcpy(s, path+start, n); s[n] = '\0';
    return s;
}
static char *suffix(const char *base, const char *tail) {
    size_t n = strlen(base), m = strlen(tail);
    char *s;
    if (n > (size_t)-1 - m - 1) fatal("path too long");
    s = resize(NULL, n+m+1, 1);
    memcpy(s, base, n); memcpy(s+n, tail, m+1); return s;
}
static void ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) fatal("output path is not a directory");
        return;
    }
#ifdef _WIN32
    if (mkdir(path) != 0) fatal("cannot create output directory");
#else
    if (mkdir(path, 0777) != 0) fatal("cannot create output directory");
#endif
}
/* AI (OpenAI Codex), same prompt, AI_HISTORY.md: dynamic line reading and
   staged output. Capacity doubles, so realloc copying is amortized linear.
   Only one line and its stacks occupy RAM; earlier results go to a temporary
   stream. The destination is opened after parsing, so ERROR replaces every
   partial result and even a destination equal to the input cannot truncate it. */
static void process_file(const char *path, const char *outdir) {
    FILE *in = fopen(path, "rb"), *staged, *out;
    char *line = NULL, *base, *name, *destination;
    size_t capacity = 0, n = 0, offset = 0;
    Parser p = {0};
    int c;
    if (!in) fatal("cannot open input file");
    staged = tmpfile();
    if (!staged) fatal("cannot create temporary result file");
    for (;;) {
        c = fgetc(in);
        if (c == '\n' || c == EOF) {
            Value answer;
            int status;
            if (n+1 > capacity) { capacity = grow(capacity); line = resize(line, capacity, 1); }
            line[n] = '\0';
            status = evaluate(&p, line, n, offset, &answer);
            if (status < 0) break;
            if (status > 0) {
                int rc = answer.floating ? fprintf(staged, "%.15g\n", answer.d) : fprintf(staged, "%lld\n", answer.i);
                if (rc < 0) fatal("cannot stage results");
            }
            if (c == EOF) break;
            if (offset > (size_t)-1 - n - 1) fatal("input too large");
            offset += n+1; n = 0;
        } else {
            if (n+1 >= capacity) { capacity = grow(capacity); line = resize(line, capacity, 1); }
            line[n++] = (char)c;
        }
    }
    if (ferror(in)) fatal("cannot read input file");
    if (fclose(in) != 0) fatal("cannot close input file");
    free(line); free(p.values); free(p.ops);
    base = base_name(path, 1);
    name = suffix(base, "_" NAME "_" LASTNAME "_" STUDENT ".txt");
    destination = join(outdir, name); free(base); free(name);
    out = fopen(destination, "wb"); free(destination);
    if (!out) fatal("cannot write output file");
    if (p.error) {
        if (fprintf(out, "ERROR:%zu\n", p.error) < 0) fatal("cannot write error result");
    } else {
        char buffer[16384]; size_t count;
        if (fflush(staged) != 0 || fseek(staged, 0, SEEK_SET) != 0) fatal("cannot rewind results");
        while ((count = fread(buffer, 1, sizeof buffer, staged)) != 0)
            if (fwrite(buffer, 1, count, out) != count) fatal("cannot write results");
        if (ferror(staged)) fatal("cannot read staged results");
    }
    if (fclose(staged) != 0) fatal("cannot close temporary results");
    if (fclose(out) != 0) fatal("cannot close output file");
}
/* AI (OpenAI Codex), same prompt, AI_HISTORY.md: CLI and directory traversal.
   Snapshot regular .txt paths before writing: an output folder equal to the
   input folder must not cause newly generated results to be visited forever. */
int main(int argc, char **argv) {
    const char *input = NULL, *dir = NULL, *outdir = NULL;
    char *default_dir = NULL;
    int i;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dir")) {
            if (dir || i+1 == argc) fatal("directory option requires one argument");
            dir = argv[++i];
        } else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output-dir")) {
            if (outdir || i+1 == argc) fatal("output option requires one argument");
            outdir = argv[++i];
        } else if (argv[i][0] == '-') fatal("unknown option");
        else if (input) fatal("only one input file is allowed");
        else input = argv[i];
    }
    if ((!input && !dir) || (input && dir))
        fatal("usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]");
    if (!outdir) {
        char *base = base_name(dir ? dir : input, dir == NULL);
        default_dir = suffix(base, "_" NAME "_" STUDENT); free(base);
        outdir = default_dir;
    }
    if (dir) {
        DIR *d = opendir(dir);
        struct dirent *entry;
        char **files = NULL;
        size_t count = 0, cap = 0, j;
        if (!d) fatal("cannot open input directory");
        for (;;) {
            size_t len; char *path; struct stat st;
            errno = 0; entry = readdir(d);
            if (!entry) { if (errno) fatal("cannot read input directory"); break; }
            len = strlen(entry->d_name);
            if (len < 4 || strcmp(entry->d_name+len-4, ".txt")) continue;
            path = join(dir, entry->d_name);
            if (stat(path, &st) != 0) fatal("cannot inspect input file");
            if (!S_ISREG(st.st_mode)) { free(path); continue; }
            if (count == cap) { cap = grow(cap); files = resize(files, cap, sizeof *files); }
            files[count++] = path;
        }
        if (closedir(d) != 0) fatal("cannot close input directory");
        ensure_dir(outdir);
        for (j = 0; j < count; ++j) { process_file(files[j], outdir); free(files[j]); }
        free(files);
    } else {
        struct stat st;
        if (stat(input, &st) != 0 || !S_ISREG(st.st_mode)) fatal("input is not a readable regular file");
        ensure_dir(outdir); process_file(input, outdir);
    }
    free(default_dir);
    return EXIT_SUCCESS;
}


