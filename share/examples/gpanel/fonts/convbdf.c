/*
 * Convert BDF files to C source
 *
 * Copyright (c) 2002 by Greg Haerr <greg@censoft.com>
 * Copyright (c) 2008-2015 by Serge Vakulenko <vak@cronyx.ru>
 *
 * What fun it is converting font data...
 *
 * 09/17/02 Version 1.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Glyph image helper macros.
 */
#define BITS_PER_WORD       (sizeof(unsigned short) * 8)
#define NIBBLES_PER_WORD    (BITS_PER_WORD/4)
#define WORDS(bits)         (((bits)+BITS_PER_WORD-1)/BITS_PER_WORD)
#define BYTES(bits)         (WORDS(bits) * sizeof(unsigned short))
#define TEST_HIGH_BIT(w)    ((w) >> (BITS_PER_WORD - 1) & 1)

/*
 * Builtin C-based proportional/fixed font structure.
 * Based on The Microwindows Project http://microwindows.org
 */
typedef struct {
    char *  name;           /* font name*/
    int     maxwidth;       /* max width in pixels*/
    int     height;         /* height in pixels*/
    int     ascent;         /* ascent (baseline) height*/
    int     firstchar;      /* first character in bitmap*/
    int     size;           /* font size in glyphs*/
    unsigned short *bits;   /* 16-bit right-padded bitmap data*/
    unsigned long *offset;  /* offsets into bitmap data*/
    unsigned char *width;   /* character widths or NULL if fixed*/
    int     defaultchar;    /* default char (not glyph index)*/
    long    bits_size;      /* # words of bits[]*/

    /* unused by runtime system, read in by convbdf*/
    char *  facename;       /* facename of font*/
    char *  copyright;      /* copyright info for loadable fonts*/
    int     pixel_size;
    int     descent;
    int     fbbw, fbbh, fbbx, fbby;
} font_t;

#define isprefix(buf,str)   (!strncmp(buf, str, strlen(str)))
#define strequal(s1,s2)     (!strcmp(s1, s2))

#define EXTRA   300     /* # bytes extra allocation for buggy .bdf files*/

int gen_map = 1;
int start_char = 0;
int default_char = 0;
int exclude_start = 0;
int exclude_end = 0;
int limit_char = 65535;
int ascent_correction = 0;
int descent_correction = 0;
char outfile[256];

void        usage(void);
void        getopts(int *pac, char ***pav);
int         convbdf(char *path);

void        free_font(font_t *pf);
font_t *    bdf_read_font(char *path);
int         bdf_read_header(FILE *fp, font_t *pf);
int         bdf_read_bitmaps(FILE *fp, font_t *pf);
char *      bdf_getline(FILE *fp, char *buf, int len);
unsigned short bdf_hexval(unsigned char *buf, int ndx1, int ndx2);

int         gen_c_source(font_t *pf, char *path);

void
usage(void)
{
    fprintf (stderr,
    "Usage: convbdf [options] [input-files]\n"
    "Options:\n"
    "    -s N   Start output at character encodings >= N\n"
    "    -l N   Limit output to character encodings <= N\n"
    "    -x N-M Exclude character range N...M\n"
    "    -u N   Use code N as default char \n"
    "    -a N   Decrease ascent by N\n"
    "    -d N   Decrease descent by N\n"
    "    -n     Don't generate bitmaps as comments in .c file\n"
    );
}

/*
 * parse command line options
 */
void
getopts(int *pac, char ***pav)
{
    char *p;
    char **av;
    int ac;

    ac = *pac;
    av = *pav;
    while (ac > 0 && av[0][0] == '-') {
        p = &av[0][1];
        while( *p)
            switch(*p++) {
        case ' ':               /* multiple -args on av[]*/
            while( *p && *p == ' ')
                p++;
            if( *p++ != '-')    /* next option must have dash*/
                p = "";
            break;              /* proceed to next option*/
        case 'n':               /* don't gen bitmap comments*/
            gen_map = 0;
            break;
        case 'l':               /* set encoding limit */
            if (*p) {
                limit_char = strtol (p, 0, 0);
                while (*p && *p != ' ')
                    p++;
            } else {
                av++; ac--;
                if (ac > 0)
                    limit_char = strtol (av[0], 0, 0);
            }
            break;
        case 'x':               /* exclude encoding range */
            if (*p) {
                char *e;
                exclude_start = strtol (p, &e, 0);
                if (*e != '-') {
                    fprintf(stderr, "Invalud option ignored: %s\r\n", p);
                    exclude_start = 0;
                    break;
                }
                exclude_end = strtol (e+1, 0, 0);
                while (*p && *p != ' ')
                    p++;
            } else {
                av++; ac--;
                if (ac > 0) {
                    char *e;
                    exclude_start = strtol (av[0], &e, 0);
                    if (*e != '-') {
                        fprintf(stderr, "Invalud option ignored: %s\r\n", av[0]);
                        exclude_start = 0;
                        break;
                    }
                    exclude_end = strtol (e+1, 0, 0);
                }
            }
            break;
        case 's':               /* set encoding start */
            if (*p) {
                start_char = strtol (p, 0, 0);
                while (*p && *p != ' ')
                    p++;
            } else {
                av++; ac--;
                if (ac > 0)
                    start_char = strtol (av[0], 0, 0);
            }
            break;
        case 'a':               /* ascent correction */
            if (*p) {
                ascent_correction = strtol (p, 0, 0);
                while (*p && *p != ' ')
                    p++;
            } else {
                av++; ac--;
                if (ac > 0)
                    ascent_correction = strtol (av[0], 0, 0);
            }
            break;
        case 'd':               /* descent correction */
            if (*p) {
                descent_correction = strtol (p, 0, 0);
                while (*p && *p != ' ')
                    p++;
            } else {
                av++; ac--;
                if (ac > 0)
                    descent_correction = strtol (av[0], 0, 0);
            }
            break;
        case 'u':               /* unknown char */
            if (*p) {
                default_char = strtol (p, 0, 0);
                while (*p && *p != ' ')
                    p++;
            } else {
                av++; ac--;
                if (ac > 0)
                    default_char = strtol (av[0], 0, 0);
            }
            break;
        default:
            fprintf(stderr, "Unknown option ignored: %c\r\n", *(p-1));
        }
        ++av; --ac;
    }
    *pac = ac;
    *pav = av;
}

/*
 * remove directory prefix and file suffix from full path
 */
char *
basename(char *path)
{
    char *p, *b;
    static char base[256];

    /* remove prepended path and extension*/
    b = path;
    for (p=path; *p; ++p) {
        if (*p == '/')
            b = p + 1;
    }
    strcpy(base, b);
    for (p=base; *p; ++p) {
        if (*p == '.') {
            *p = 0;
            break;
        }
    }
    return base;
}

int
convbdf(char *path)
{
    font_t *pf;
    int ret = 0;

    pf = bdf_read_font(path);
    if (!pf)
        exit(1);

    strcpy(outfile, basename(path));
    strcat(outfile, ".c");

    ret |= gen_c_source(pf, outfile);

    free_font(pf);
    return ret;
}

int
main(int ac, char **av)
{
    int ret = 0;

    ++av; --ac;         /* skip av[0] */
    getopts(&ac, &av);  /* read command line options */

    if (ac < 1) {
        usage();
        exit(1);
    }

    while (ac > 0) {
        ret |= convbdf(av[0]);
        ++av; --ac;
    }

    exit(ret);
}

/*
 * free font structure
 */
void
free_font(font_t *pf)
{
    if (!pf)
        return;
    if (pf->name)
        free(pf->name);
    if (pf->facename)
        free(pf->facename);
    if (pf->bits)
        free(pf->bits);
    if (pf->offset)
        free(pf->offset);
    if (pf->width)
        free(pf->width);
    free(pf);
}

/*
 * build incore structure from .bdf file
 */
font_t *
bdf_read_font(char *path)
{
    FILE *fp;
    font_t *pf;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Error opening file: %s\n", path);
        return NULL;
    }

    pf = (font_t *) calloc(1, sizeof(font_t));
    if (!pf)
        goto errout;

    pf->name = strdup(basename(path));

    if (!bdf_read_header(fp, pf)) {
        fprintf(stderr, "Error reading font header\n");
        goto errout;
    }

    if (!bdf_read_bitmaps(fp, pf)) {
        fprintf(stderr, "Error reading font bitmaps\n");
        goto errout;
    }

    fclose(fp);
    return pf;

errout:
    fclose(fp);
    free_font(pf);
    return NULL;
}

/*
 * read bdf font header information, return 0 on error
 */
int
bdf_read_header(FILE *fp, font_t *pf)
{
    int encoding;
    int nchars, maxwidth = 0;
    int firstchar = 65535;
    int lastchar = -1;
    char buf[256];
    char facename[256];
    char copyright[256];

    /* set certain values to errors for later error checking*/
    pf->defaultchar = -1;
    pf->ascent = -1;
    pf->descent = -1;

    for (;;) {
        if (!bdf_getline(fp, buf, sizeof(buf))) {
            fprintf(stderr, "Error: EOF on file\n");
            return 0;
        }
        if (isprefix(buf, "FONT ")) {       /* not required*/
            if (sscanf(buf, "FONT %[^\n]", facename) != 1) {
                fprintf(stderr, "Error: bad 'FONT'\n");
                return 0;
            }
            pf->facename = strdup(facename);
            continue;
        }
        if (isprefix(buf, "COPYRIGHT ")) {  /* not required*/
            if (sscanf(buf, "COPYRIGHT \"%[^\"]", copyright) != 1) {
                fprintf(stderr, "Error: bad 'COPYRIGHT'\n");
                return 0;
            }
            pf->copyright = strdup(copyright);
            continue;
        }
        if (isprefix(buf, "DEFAULT_CHAR ")) {   /* not required*/
            if (sscanf(buf, "DEFAULT_CHAR %d", &pf->defaultchar) != 1) {
                fprintf(stderr, "Error: bad 'DEFAULT_CHAR'\n");
                return 0;
            }
        }
        if (isprefix(buf, "FONT_DESCENT ")) {
            if (sscanf(buf, "FONT_DESCENT %d", &pf->descent) != 1) {
                fprintf(stderr, "Error: bad 'FONT_DESCENT'\n");
                return 0;
            }
            continue;
        }
        if (isprefix(buf, "FONT_ASCENT ")) {
            if (sscanf(buf, "FONT_ASCENT %d", &pf->ascent) != 1) {
                fprintf(stderr, "Error: bad 'FONT_ASCENT'\n");
                return 0;
            }
            continue;
        }
        if (isprefix(buf, "FONTBOUNDINGBOX ")) {
            if (sscanf(buf, "FONTBOUNDINGBOX %d %d %d %d",
                &pf->fbbw, &pf->fbbh, &pf->fbbx, &pf->fbby) != 4) {
                fprintf(stderr, "Error: bad 'FONTBOUNDINGBOX'\n");
                return 0;
            }
            continue;
        }
        if (isprefix(buf, "CHARS ")) {
            if (sscanf(buf, "CHARS %d", &nchars) != 1) {
                fprintf(stderr, "Error: bad 'CHARS'\n");
                return 0;
            }
            continue;
        }

        /*
         * Reading ENCODING is necessary to get firstchar/lastchar
         * which is needed to pre-calculate our offset and widths
         * array sizes.
         */
        if (isprefix(buf, "ENCODING ")) {
            if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
                fprintf(stderr, "Error: bad 'ENCODING'\n");
                return 0;
            }
            if (encoding > limit_char || encoding < start_char)
                continue;
            if (exclude_start && encoding >= exclude_start && encoding <= exclude_end)
                continue;
            if (firstchar > encoding)
                firstchar = encoding;
            if (lastchar < encoding)
                lastchar = encoding;
            continue;
        }
        if (isprefix(buf, "DWIDTH ")) {
            int width;
            if (sscanf(buf, "DWIDTH %d", &width) != 1) {
                fprintf(stderr, "Error: bad 'DWIDTH' for encoding %d\n",
                    encoding);
                return 0;
            }
            if (maxwidth < width)
                maxwidth = width;
            continue;
        }
        if (strequal(buf, "ENDFONT"))
            break;
    }

    /* calc font height*/
    if (pf->ascent < 0 || pf->descent < 0 || firstchar < 0) {
        fprintf(stderr, "Error: Invalid BDF file, requires FONT_ASCENT/FONT_DESCENT/ENCODING\n");
        return 0;
    }
    pf->height = pf->ascent + pf->descent;

    /* calc default char*/
    if (default_char)
        pf->defaultchar = default_char;
    if (pf->defaultchar < 0 || pf->defaultchar < firstchar ||
        pf->defaultchar > limit_char)
        pf->defaultchar = firstchar;

    /* calc font size (offset/width entries)*/
    pf->firstchar = firstchar;
    pf->size = lastchar - firstchar + 1;

    /* initially use font maxwidth * height for bits allocation*/
    pf->bits_size = nchars * WORDS(maxwidth) * pf->height;

    /* allocate bits, offset, and width arrays*/
    pf->bits = (unsigned short*) malloc(pf->bits_size * sizeof(unsigned short) + EXTRA);
    pf->offset = (unsigned long*) malloc(pf->size * sizeof(unsigned long));
    pf->width = (unsigned char*) malloc(pf->size * sizeof(unsigned char));

    if (!pf->bits || !pf->offset || !pf->width) {
        fprintf(stderr, "Error: no memory for font load\n");
        return 0;
    }

    return 1;
}

/*
 * read bdf font bitmaps, return 0 on error
 */
int
bdf_read_bitmaps(FILE *fp, font_t *pf)
{
    long ofs = 0;
    int maxwidth = 0;
    int i, k, encoding, width;
    int bbw, bbh, bbx, bby;
    int proportional = 0;
    int encodetable = 0;
    long l;
    char buf[256];

    /* reset file pointer*/
    fseek(fp, 0L, SEEK_SET);

    /* initially mark offsets as not used*/
    for (i=0; i<pf->size; ++i)
        pf->offset[i] = -1;

    for (;;) {
        if (!bdf_getline(fp, buf, sizeof(buf))) {
            fprintf(stderr, "Error: EOF on file\n");
            return 0;
        }
        if (isprefix(buf, "STARTCHAR")) {
            encoding = width = bbw = bbh = bbx = bby = -1;
            continue;
        }
        if (isprefix(buf, "ENCODING ")) {
            if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
                fprintf(stderr, "Error: bad 'ENCODING'\n");
                return 0;
            }
            if (encoding < start_char || encoding > limit_char)
                encoding = -1;
            if (exclude_start && encoding >= exclude_start && encoding <= exclude_end)
                encoding = -1;
            continue;
        }
        if (isprefix(buf, "DWIDTH ")) {
            if (sscanf(buf, "DWIDTH %d", &width) != 1) {
                fprintf(stderr, "Error: bad 'DWIDTH'\n");
                return 0;
            }
            /* use font boundingbox width if DWIDTH <= 0*/
            if (width <= 0)
                width = pf->fbbw - pf->fbbx;
            continue;
        }
        if (isprefix(buf, "BBX ")) {
            if (sscanf(buf, "BBX %d %d %d %d", &bbw, &bbh, &bbx, &bby) != 4) {
                fprintf(stderr, "Error: bad 'BBX'\n");
                return 0;
            }
            continue;
        }
        if (strequal(buf, "BITMAP")) {
            unsigned short *ch_bitmap = pf->bits + ofs;
            int ch_words;

            if (encoding < 0)
                continue;

            /* set bits offset in encode map*/
            if (pf->offset[encoding-pf->firstchar] != (unsigned long)-1) {
                fprintf(stderr, "Error: duplicate encoding for character %d (0x%02x), ignoring duplicate\n",
                    encoding, encoding);
                continue;
            }
            pf->offset[encoding-pf->firstchar] = ofs;

            /* calc char width*/
            if (bbx < 0) {
                width -= bbx;
                /*if (width > maxwidth)
                    width = maxwidth;*/
                bbx = 0;
            }
            if (width > maxwidth)
                maxwidth = width;
            pf->width[encoding-pf->firstchar] = width;

            /* clear bitmap*/
            memset(ch_bitmap, 0, BYTES(width) * pf->height);

            ch_words = WORDS(width);
#define BM(row,col) (*(ch_bitmap + ((row)*ch_words) + (col)))

            /* read bitmaps*/
            for (i=0; ; ++i) {
                int hexnibbles;

                if (!bdf_getline(fp, buf, sizeof(buf))) {
                    fprintf(stderr, "Error: EOF reading BITMAP data\n");
                    return 0;
                }
                if (isprefix(buf, "ENDCHAR"))
                    break;

                hexnibbles = strlen(buf);
                for (k=0; k<ch_words; ++k) {
                    int ndx = k * NIBBLES_PER_WORD;
                    int padnibbles = hexnibbles - ndx;
                    unsigned short value;

                    if (padnibbles <= 0)
                        break;
                    if (padnibbles >= NIBBLES_PER_WORD)
                        padnibbles = 0;

                    value = bdf_hexval((unsigned char *)buf,
                        ndx, ndx+NIBBLES_PER_WORD-1-padnibbles);
                    value <<= padnibbles * NIBBLES_PER_WORD;

                    BM(pf->height - pf->descent - bby - bbh + i, k) |=
                        value >> bbx;
                    /* handle overflow into next image word*/
                    if (bbx) {
                        BM(pf->height - pf->descent - bby - bbh + i, k+1) =
                            value << (BITS_PER_WORD - bbx);
                    }
                }
            }

            ofs += WORDS(width) * pf->height;

            continue;
        }
        if (strequal(buf, "ENDFONT"))
            break;
    }

    /* set max width*/
    pf->maxwidth = maxwidth;

    /* change unused offset/width values to default char values*/
    for (i=0; i<pf->size; ++i) {
        int defchar = pf->defaultchar - pf->firstchar;

        if (pf->offset[i] == (unsigned long)-1) {
            pf->offset[i] = pf->offset[defchar];
            pf->width[i] = pf->width[defchar];
        }
    }

    /* determine whether font doesn't require encode table*/
    l = 0;
    for (i=0; i<pf->size; ++i) {
        if (pf->offset[i] != l) {
            encodetable = 1;
            break;
        }
        l += WORDS(pf->width[i]) * pf->height;
    }
    if (!encodetable) {
        free(pf->offset);
        pf->offset = NULL;
    }

    /* determine whether font is fixed-width*/
    for (i=0; i<pf->size; ++i) {
        if (pf->width[i] != maxwidth) {
            proportional = 1;
            break;
        }
    }
    if (!proportional) {
        free(pf->width);
        pf->width = NULL;
    }

    /* reallocate bits array to actual bits used*/
    if (ofs < pf->bits_size) {
        pf->bits = realloc(pf->bits, ofs * sizeof(unsigned short));
        pf->bits_size = ofs;
    } else if (ofs > pf->bits_size) {
        fprintf(stderr, "Warning: DWIDTH spec > max FONTBOUNDINGBOX\n");
        if (ofs > pf->bits_size+EXTRA) {
            fprintf(stderr, "Error: Not enough bits initially allocated\n");
            return 0;
        }
        pf->bits_size = ofs;
    }

    return 1;
}

/*
 * read the next non-comment line, returns buf or NULL if EOF
 */
char *
bdf_getline(FILE *fp, char *buf, int len)
{
    int c;
    char *b;

    for (;;) {
        b = buf;
        while ((c = getc(fp)) != EOF) {
            if (c == '\r')
                continue;
            if (c == '\n')
                break;
            if (b - buf >= (len - 1))
                break;
            *b++ = c;
        }
        *b = '\0';
        if (c == EOF && b == buf)
            return NULL;
        if (b != buf && !isprefix(buf, "COMMENT"))
            break;
    }
    return buf;
}

/*
 * return hex value of portion of buffer
 */
unsigned short
bdf_hexval(unsigned char *buf, int ndx1, int ndx2)
{
    unsigned short val = 0;
    int i, c;

    for (i=ndx1; i<=ndx2; ++i) {
        c = buf[i];
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'F')
            c = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            c = c - 'a' + 10;
        else c = 0;
        val = (val << 4) | c;
    }
    return val;
}

/*
 * generate C source from in-core font
 */
int
gen_c_source(font_t *pf, char *path)
{
    FILE *ofp;
    int i;
    int did_defaultchar = 0;
    time_t t = time(0);
    char buf[256];
    char obuf[256];
    char hdr1[] = {
        "/* Generated by convbdf on %s. */\n"
        "#include <sys/gpanel.h>\n"
        "\n"
        "/* Font information:\n"
        "   name: %s\n"
        "   facename: %s\n"
        "   w x h: %dx%d\n"
        "   size: %d\n"
        "   ascent: %d\n"
        "   descent: %d\n"
        "   first char: %d (0x%02x)\n"
        "   last char: %d (0x%02x)\n"
        "   default char: %d (0x%02x)\n"
        "   proportional: %s\n"
        "   %s\n"
        "*/\n"
        "\n"
        "/* Font character bitmap data. */\n"
        "static const unsigned short _%s_bits[] = {\n"
    };

    ofp = fopen(path, "w");
    if (!ofp) {
        fprintf(stderr, "Can't create %s\n", path);
        return 1;
    }
    fprintf(stderr, "Generating %s\n", path);

    strcpy(buf, ctime(&t));
    buf[strlen(buf)-1] = 0;

    fprintf(ofp, hdr1, buf,
        pf->name,
        pf->facename? pf->facename: "",
        pf->maxwidth, pf->height - ascent_correction - descent_correction,
        pf->size,
        pf->ascent - ascent_correction, pf->descent - descent_correction,
        pf->firstchar, pf->firstchar,
        pf->firstchar+pf->size-1, pf->firstchar+pf->size-1,
        pf->defaultchar, pf->defaultchar,
        pf->width? "yes": "no",
        pf->copyright? pf->copyright: "",
        pf->name);

    /* generate bitmaps*/
    for (i=0; i<pf->size; ++i) {
        int x;
        int bitcount = 0;
        int width = pf->width ? pf->width[i] : pf->maxwidth;
        int height = pf->height - ascent_correction - descent_correction;
        unsigned short *bits = pf->bits + (pf->offset? pf->offset[i]: (pf->height * i));
        unsigned short bitvalue = 0;

        /*
         * Generate bitmap bits only if not this index isn't
         * the default character in encode map, or the default
         * character hasn't been generated yet.
         */
        if (pf->offset && (pf->offset[i] == pf->offset[pf->defaultchar-pf->firstchar])) {
            if (did_defaultchar)
                continue;
            did_defaultchar = 1;
        }

        fprintf(ofp, "\n/* Character %d (0x%02x):\n   width %d",
            i+pf->firstchar, i+pf->firstchar, width);

        bits += WORDS(width) * ascent_correction;
        if (gen_map) {
            fprintf(ofp, "\n   +");
            for (x=0; x<width; ++x) fprintf(ofp, "-");
            fprintf(ofp, "+\n");

            x = 0;
            while (height > 0) {
                if (x == 0) fprintf(ofp, "   |");

                if (bitcount <= 0) {
                    bitcount = BITS_PER_WORD;
                    bitvalue = *bits++;
                }

                fprintf(ofp, TEST_HIGH_BIT(bitvalue)? "*": " ");

                bitvalue <<= 1;
                --bitcount;
                if (++x == width) {
                    fprintf(ofp, "|\n");
                    --height;
                    x = 0;
                    bitcount = 0;
                }
            }
            fprintf(ofp, "   +");
            for (x=0; x<width; ++x) fprintf(ofp, "-");
            fprintf(ofp, "+ */\n");
        } else
            fprintf(ofp, " */\n");

        bits = pf->bits + (pf->offset? pf->offset[i]: (pf->height * i));
        bits += WORDS(width) * ascent_correction;
        height = pf->height - ascent_correction - descent_correction;
        for (; height>0; --height) {
            for (x=WORDS(width); x>0; --x) {
                fprintf(ofp, "0x%04x,", *bits++);
            }
            fprintf(ofp, "\n");
        }
    }
    fprintf(ofp,    "};\n\n");

    if (pf->offset) {
        long offset, default_offset;

        /* output offset table*/
        fprintf(ofp, "/* Character->glyph mapping. */\n"
            "static const unsigned short _%s_offset[] = {\n",
            pf->name);
        offset = 0;
        did_defaultchar = 0;
        default_offset = 0;
        for (i=0; i<pf->size; ++i) {
            int width = pf->width ? pf->width[i] : pf->maxwidth;
            if (pf->offset && (pf->offset[i] ==
                pf->offset[pf->defaultchar-pf->firstchar])) {
                if (did_defaultchar) {
                    fprintf(ofp, "  %ld,\t/* (0x%02x) */\n",
                        default_offset,
                        i + pf->firstchar);
                    continue;
                }
                did_defaultchar = 1;
                default_offset = offset;
            }
            fprintf(ofp, "  %ld,\t/* (0x%02x) */\n", offset,
                i + pf->firstchar);
            offset += WORDS(width) * (pf->height - ascent_correction -
                descent_correction);
        }
        fprintf(ofp, "};\n\n");
    }

    /* output width table for proportional fonts*/
    if (pf->width) {
        fprintf(ofp,    "/* Character width data. */\n"
            "static const unsigned char _%s_width[] = {\n",
            pf->name);

        for (i=0; i<pf->size; ++i)
            fprintf(ofp, "  %d,\t/* (0x%02x) */\n",
                pf->width[i], i+pf->firstchar);
        fprintf(ofp, "};\n\n");
    }

    /* output font_t struct*/
    if (pf->offset)
        sprintf(obuf, "_%s_offset,", pf->name);
    else sprintf(obuf, "0,  /* no encode table*/");
    if (pf->width)
        sprintf(buf, "_%s_width,", pf->name);
    else sprintf(buf, "0,  /* fixed width*/");
    fprintf(ofp,    "/* Exported structure definition. */\n"
        "const struct gpanel_font_t font_%s = {\n"
        "  \"%s\",\n"
        "  %d,\n"
        "  %d,\n"
        "  %d,\n"
        "  %d,\n"
        "  %d,\n"
        "  _%s_bits,\n"
        "  %s\n"
        "  %s\n"
        "  %d,\n"
        "  sizeof(_%s_bits) / sizeof(_%s_bits[0]),\n"
        "};\n",
        pf->name, pf->name,
        pf->maxwidth, pf->height - ascent_correction - descent_correction,
        pf->ascent - ascent_correction,
        pf->firstchar,
        pf->size,
        pf->name,
        obuf,
        buf,
        pf->defaultchar,
        pf->name, pf->name);

    return 0;
}
