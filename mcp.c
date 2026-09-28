/*
 * mcp — multi-copy: copia un archivo en múltiples destinos.
 *
 * Uso:  mcp [OPCIONES] FUENTE DESTINO...
 *
 * Ejemplo:
 *   mcp text.txt ./test/test{1..5}.txt
 *   (el shell expande la llave {1..5} a 5 destinos y mcp copia la fuente a cada uno)
 *
 * Si un DESTINO es un directorio, la fuente se copia dentro de él
 * conservando su nombre.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>   /* basename */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PROGNAME   "mcp"
#define VERSION    "1.0.0"
#define COPY_BUF   (128 * 1024)

typedef struct {
    bool verbose;   /* -v: imprimir cada copia */
    bool preserve;  /* -p: preservar permisos y fechas */
    bool force;     /* -f: borrar y reintentar si no se puede abrir */
} options_t;

static void usage(FILE *out)
{
    fprintf(out,
        "Uso: %s [OPCIONES] FUENTE DESTINO...\n"
        "\n"
        "Copia FUENTE en múltiples DESTINO (un 'cp' de varios destinos).\n"
        "Si un DESTINO es un directorio, la fuente se copia dentro de él\n"
        "conservando su nombre.\n"
        "\n"
        "Opciones:\n"
        "  -v, --verbose    imprime cada copia realizada\n"
        "  -p, --preserve   preserva permisos y fechas de la fuente\n"
        "  -f, --force      si el destino no se puede abrir, lo borra y reintenta\n"
        "  -h, --help       muestra esta ayuda y sale\n"
        "  -V, --version    muestra la versión y sale\n"
        "\n"
        "Ejemplos:\n"
        "  %s text.txt ./test/test{1..5}.txt\n"
        "  %s -v informe.pdf copia1.pdf copia2.pdf respaldo/informe.pdf\n",
        PROGNAME, PROGNAME, PROGNAME);
}

/* Copia el contenido de 'src' hacia 'dst'. Devuelve 0 en éxito. */
static int copy_file(const char *src, const char *dst, const options_t *opt)
{
    int in = -1, out = -1;
    int ret = -1;
    bool from_stdin = (strcmp(src, "-") == 0);

    if (from_stdin) {
        in = STDIN_FILENO;
    } else {
        in = open(src, O_RDONLY);
        if (in < 0) {
            fprintf(stderr, "%s: no se puede abrir '%s': %s\n",
                    PROGNAME, src, strerror(errno));
            goto done;
        }
    }

    struct stat st;
    if (fstat(in, &st) < 0) {
        fprintf(stderr, "%s: no se puede obtener info de '%s': %s\n",
                PROGNAME, src, strerror(errno));
        goto done;
    }
    if (S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s: se omite el directorio '%s' "
                        "(no se copian directorios)\n", PROGNAME, src);
        goto done;
    }

    out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (out < 0 && opt->force) {
        if (unlink(dst) == 0)
            out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    }
    if (out < 0) {
        fprintf(stderr, "%s: no se puede crear '%s': %s\n",
                PROGNAME, dst, strerror(errno));
        goto done;
    }

    char buf[COPY_BUF];
    ssize_t n;
    while ((n = read(in, buf, sizeof buf)) > 0) {
        char *p = buf;
        while (n > 0) {
            ssize_t w = write(out, p, (size_t)n);
            if (w < 0) {
                if (errno == EINTR)
                    continue;
                fprintf(stderr, "%s: error al escribir '%s': %s\n",
                        PROGNAME, dst, strerror(errno));
                goto done;
            }
            p += w;
            n -= w;
        }
    }
    if (n < 0) {
        fprintf(stderr, "%s: error al leer '%s': %s\n",
                PROGNAME, src, strerror(errno));
        goto done;
    }

    if (opt->preserve) {
        (void)fchmod(out, st.st_mode & 07777);
        struct timespec ts[2];
        ts[0] = st.st_atim;
        ts[1] = st.st_mtim;
        (void)futimens(out, ts);
    }

    if (opt->verbose)
        fprintf(stderr, "'%s' -> '%s'\n", src, dst);

    ret = 0;

done:
    if (out >= 0 && out != STDIN_FILENO)
        close(out);
    if (in >= 0 && !from_stdin)
        close(in);
    return ret;
}

int main(int argc, char **argv)
{
    options_t opt = { false, false, false };

    static const struct option longopts[] = {
        { "verbose",  no_argument, NULL, 'v' },
        { "preserve", no_argument, NULL, 'p' },
        { "force",    no_argument, NULL, 'f' },
        { "help",     no_argument, NULL, 'h' },
        { "version",  no_argument, NULL, 'V' },
        { NULL, 0, NULL, 0 }
    };

    int c;
    while ((c = getopt_long(argc, argv, "vpfhV", longopts, NULL)) != -1) {
        switch (c) {
        case 'v': opt.verbose  = true; break;
        case 'p': opt.preserve = true; break;
        case 'f': opt.force    = true; break;
        case 'h': usage(stdout); return 0;
        case 'V': printf("%s %s\n", PROGNAME, VERSION); return 0;
        default:  usage(stderr); return 2;
        }
    }

    int rest = argc - optind;
    if (rest < 2) {
        usage(stderr);
        return 2;
    }

    const char *src = argv[optind];
    int failures = 0;

    for (int i = optind + 1; i < argc; i++) {
        const char *dest = argv[i];
        char *joined = NULL;

        /* Si el destino es un directorio, copia dentro con el nombre base. */
        struct stat dst_st;
        if (stat(dest, &dst_st) == 0 && S_ISDIR(dst_st.st_mode)) {
            char *src_copy = strdup(src);
            char *base = basename(src_copy);
            size_t len = strlen(dest) + 1 + strlen(base) + 1;
            joined = malloc(len);
            if (joined) {
                snprintf(joined, len, "%s/%s", dest, base);
                dest = joined;
            }
            free(src_copy);
        }

        if (copy_file(src, dest, &opt) != 0)
            failures++;

        free(joined);
    }

    return failures ? 1 : 0;
}
