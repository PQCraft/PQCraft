#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/fb.h>

#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define VERSTR "FBDraw 1.1"

static struct fb_var_screeninfo varinfo;
static struct fb_fix_screeninfo fixinfo;
static void* buf;
static uint32_t colorconv[3][256];
#define PUTPIX_FAST(PUTPIX_x, PUTPIX_y, PUTPIX_r, PUTPIX_g, PUTPIX_b) do {\
    unsigned PUTPIX_off = (PUTPIX_x) + (PUTPIX_y) * varinfo.xres_virtual;\
    switch ((unsigned char)varinfo.bits_per_pixel) {\
        case 16:\
            ((uint16_t*)buf)[PUTPIX_off] = colorconv[0][(PUTPIX_r)] | colorconv[1][(PUTPIX_g)] | colorconv[2][(PUTPIX_b)];\
            break;\
        case 32:\
            ((uint32_t*)buf)[PUTPIX_off] = colorconv[0][(PUTPIX_r)] | colorconv[1][(PUTPIX_g)] | colorconv[2][(PUTPIX_b)];\
            break;\
    }\
} while (0)
#define PUTPIX(PUTPIX_x, PUTPIX_y, PUTPIX_r, PUTPIX_g, PUTPIX_b) do {\
    if ((PUTPIX_x) >= 0 && (PUTPIX_x) < varinfo.xres && (PUTPIX_y) >= 0 && (PUTPIX_y) < varinfo.yres) {\
        PUTPIX_FAST((PUTPIX_x), (PUTPIX_y), (PUTPIX_r), (PUTPIX_g), (PUTPIX_b));\
    }\
} while (0)
#define PUTLINE_FAST(PUTLINE_y, PUTLINE_x1, PUTLINE_x2, PUTLINE_r, PUTLINE_g, PUTLINE_b) do {\
    register uint32_t PUTLINE_color = colorconv[0][(PUTLINE_r)] | colorconv[1][(PUTLINE_g)] | colorconv[2][(PUTLINE_b)];\
    switch ((unsigned char)varinfo.bits_per_pixel) {\
        case 16: {\
            register uint16_t* buf_begin = buf;\
            buf_begin += (PUTLINE_y) * varinfo.xres_virtual;\
            register uint16_t* buf_end = buf_begin + (PUTLINE_x2);\
            buf_begin += (PUTLINE_x1);\
            while (1) {\
                *buf_begin = PUTLINE_color;\
                if (buf_begin == buf_end) break;\
                ++buf_begin;\
            }\
        } break;\
        case 32: {\
            register uint32_t* buf_begin = buf;\
            buf_begin += (PUTLINE_y) * varinfo.xres_virtual;\
            register uint32_t* buf_end = buf_begin + (PUTLINE_x2);\
            buf_begin += (PUTLINE_x1);\
            while (1) {\
                *buf_begin = PUTLINE_color;\
                if (buf_begin == buf_end) break;\
                ++buf_begin;\
            }\
        } break;\
    }\
} while (0)
#define PUTLINE(PUTLINE_y, PUTLINE_x1, PUTLINE_x2, PUTLINE_r, PUTLINE_g, PUTLINE_b) do {\
    if ((PUTLINE_y) >= 0 && (PUTLINE_y) < varinfo.yres) {\
        PUTLINE_FAST(\
            (PUTLINE_y),\
            ((PUTLINE_x1) >= 0) ? (((PUTLINE_x1) < varinfo.xres) ? (PUTLINE_x1) : varinfo.xres - 1) : 0,\
            ((PUTLINE_x2) >= 0) ? (((PUTLINE_x2) < varinfo.xres) ? (PUTLINE_x2) : varinfo.xres - 1) : 0,\
            (PUTLINE_r), (PUTLINE_g), (PUTLINE_b)\
        );\
    }\
} while (0)

static char* cmd;
#define DEFCMD(DEFCMD_name) static bool cmd_##DEFCMD_name(void)
DEFCMD(help);
DEFCMD(version);
DEFCMD(verbose);
DEFCMD(info);
DEFCMD(color);
DEFCMD(pixel);
DEFCMD(line);
DEFCMD(rect);
DEFCMD(tri);

int main(int argc, char** argv) {
    if (argc != 3 || !*argv[1] || !*argv[2]) {
        fputs(VERSTR "\n", stderr);
        fprintf(stderr, "%s FRAMEBUFFER COMMANDS\n", argv[0]);
        fprintf(stderr, "Use the h or help command for more info.\n");
        return 1;
    }
    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open framebuffer %s\n", argv[1]);
        return 1;
    }
    if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) || ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo)) {
        fprintf(stderr, "Failed to get framebuffer info for %s\n", argv[1]);
        return 1;
    }
    buf = mmap(NULL, varinfo.yres_virtual * fixinfo.line_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf == MAP_FAILED) {
        fprintf(stderr, "Failed to map framebuffer %s into memory\n", argv[1]);
        return 1;
    }
    unsigned rgbmax[3] = {(1 << varinfo.red.length) - 1, (1 << varinfo.green.length) - 1, (1 << varinfo.blue.length) - 1};
    for (uint32_t i = 0; i < 256; ++i) {
        colorconv[0][i] = ((i * rgbmax[0] + 127) / 255) << varinfo.red.offset;
        colorconv[1][i] = ((i * rgbmax[1] + 127) / 255) << varinfo.green.offset;
        colorconv[2][i] = ((i * rgbmax[2] + 127) / 255) << varinfo.blue.offset;
    }
    cmd = argv[2];
    int retval = 0;
    while (1) {
        nextcmd:;
        while (*cmd == ' ' || *cmd == '\n' || *cmd == ';') ++cmd;
        int cmdnamelen = 0;
        while (cmd[cmdnamelen] && cmd[cmdnamelen] != ' ' && cmd[cmdnamelen] != '\n' && cmd[cmdnamelen] != ';') ++cmdnamelen;
        --cmdnamelen;
        #define SKIPTOARGS() do {cmd += cmdnamelen; while (*cmd == ' ') ++cmd;} while(0)
        #define CALLCMD(CALLCMD_name) do {SKIPTOARGS(); if (!cmd_##CALLCMD_name()) {retval = 1; goto ret;} goto nextcmd;} while (0)
        switch (*cmd++) {
            case 'c':
                if (!cmdnamelen || (cmdnamelen == 4 && !strncmp(cmd, "olor", 4))) CALLCMD(color);
                break;
            case 'h':
                if (!cmdnamelen || (cmdnamelen == 3 && !strncmp(cmd, "elp", 3))) CALLCMD(help);
                break;
            case 'i':
                if (!cmdnamelen || (cmdnamelen == 3 && !strncmp(cmd, "nfo", 3))) CALLCMD(info);
                break;
            case 'l':
                if (!cmdnamelen || (cmdnamelen == 3 && !strncmp(cmd, "ine", 3))) CALLCMD(line);
                break;
            case 'p':
                if (!cmdnamelen || (cmdnamelen == 4 && !strncmp(cmd, "ixel", 4))) CALLCMD(pixel);
                break;
            case 'r':
                if (!cmdnamelen || (cmdnamelen == 3 && !strncmp(cmd, "ect", 3))) CALLCMD(rect);
                break;
            case 't':
                if (!cmdnamelen || (cmdnamelen == 2 && !strncmp(cmd, "ri", 2))) CALLCMD(tri);
                break;
            case 'v':
                if (!cmdnamelen || (cmdnamelen == 6 && !strncmp(cmd, "ersion", 6))) CALLCMD(version);
                else if (cmdnamelen == 6 && !strncmp(cmd, "erbose", 6)) CALLCMD(verbose);
                break;
            case 'V':
                if (!cmdnamelen) CALLCMD(verbose);
                break;
            case 0:
                goto ret;
        }
        --cmd;
        ++cmdnamelen;
        fprintf(stderr, "Invalid command: %.*s\n", cmdnamelen, cmd);
        retval = 1;
        break;
    }
    ret:;
    munmap(buf, varinfo.yres_virtual * fixinfo.line_length);
    close(fd);
    return retval;
}

#define PARSEINT(PARSEINT_out) do {\
    bool PARSEINT_n;\
    if (*cmd == '-') {\
        PARSEINT_n = true;\
        ++cmd;\
    } else if (*cmd == '+') {\
        PARSEINT_n = false;\
        ++cmd;\
    } else {\
        PARSEINT_n = false;\
    }\
    PARSEINT_out = 0;\
    while (*cmd >= '0' && *cmd <= '9') {\
        PARSEINT_out *= 10;\
        PARSEINT_out += *cmd++ - '0';\
    }\
    if (PARSEINT_n) PARSEINT_out *= -1;\
} while (0)
#define NEXTARG() do {\
    while (*cmd == ' ') ++cmd;\
    if (*cmd == ',') {\
        ++cmd;\
        while (*cmd == ' ') ++cmd;\
    } else if (!*cmd || *cmd == '\n' || *cmd == ';') {\
        fputs("Expected argument\n", stderr);\
        return false;\
    } else {\
        fputs("Expected next argument\n", stderr);\
        return false;\
    }\
} while (0)
#define NEXTARG_OPT(NEXTARG_has) do {\
    while (*cmd == ' ') ++cmd;\
    if (*cmd == ',') {\
        ++cmd;\
        NEXTARG_has = 1;\
        while (*cmd == ' ') ++cmd;\
    } else if (!*cmd || *cmd == '\n' || *cmd == ';') {\
        NEXTARG_has = 0;\
    } else {\
        fputs("Expected next argument\n", stderr);\
        return false;\
    }\
} while (0)
#define ENDARGS() do {\
    while (*cmd == ' ') ++cmd;\
    if (*cmd && *cmd != '\n' && *cmd != ';') {\
        fputs("Unexpected character\n", stderr);\
        return false;\
    }\
} while (0)

static bool verbose = false;
static uint8_t color[3] = {255, 255, 255};

DEFCMD(help) {
    ENDARGS();
    puts("Commands use NAME ARG1, ARG2, ... and are separated by a newline or semicolon.");
    puts("List of commands:");
    puts("  h/help - Shows this text.");
    puts("  v/version - Prints the version.");
    puts("  V/verbose - Prints out what was drawn.");
    puts("  i/info - Prints out framebuffer info.");
    puts("  c/color hhhhhh/hhh - Sets the active color (#ffffff by default).");
    puts("  p/pixel x, y - Sets a pixel to the active color.");
    puts("  l/line x1, y1, x2, y2 - Draws a line.");
    puts("  r/rect x1, y1, x2, y2 - Draws a rectangle.");
    puts("  t/tri x1, y1, x2, y2, x3, y3 - Draws a triangle.");
    return true;
}
DEFCMD(version) {
    ENDARGS();
    puts(VERSTR);
    return true;
}
DEFCMD(verbose) {
    ENDARGS();
    verbose = !verbose;
    printf("%s VERBOSE MODE\n", (verbose) ? "ENABLED" : "DISABLED");
    return true;
}
DEFCMD(info) {
    ENDARGS();
    printf(
        "INFO:\n"
        "  ID: %s\n"
        "  Type: %u (%u)\n"
        "  Line length: %u\n"
        "  Capabilities: 0x%04x\n"
        "  Visible resolution: %ux%u\n"
        "  Virtual resolution: %ux%u\n"
        "  Offset: %u, %u\n"
        "  BPP: %u\n"
        "  Grayscale: %s\n"
        "  Nonstandard: %s\n"
        "  Red: off=%u, len=%u, msb_right=%s\n"
        "  Green: off=%u, len=%u, msb_right=%s\n"
        "  Blue: off=%u, len=%u, msb_right=%s\n"
        "  Alpha: off=%u, len=%u, msb_right=%s\n",
        fixinfo.id,
        (unsigned)fixinfo.type, (unsigned)fixinfo.type_aux,
        (unsigned)fixinfo.line_length,
        (unsigned)fixinfo.capabilities,
        (unsigned)varinfo.xres, (unsigned)varinfo.yres,
        (unsigned)varinfo.xres_virtual, (unsigned)varinfo.yres_virtual,
        (unsigned)varinfo.xoffset, (unsigned)varinfo.yoffset,
        (unsigned)varinfo.bits_per_pixel,
        (varinfo.grayscale) ? "Yes" : "No",
        (varinfo.nonstd) ? "Yes" : "No",
        (unsigned)varinfo.red.offset, (unsigned)varinfo.red.length, (varinfo.red.msb_right) ? "true" : "false",
        (unsigned)varinfo.green.offset, (unsigned)varinfo.green.length, (varinfo.green.msb_right) ? "true" : "false",
        (unsigned)varinfo.blue.offset, (unsigned)varinfo.blue.length, (varinfo.blue.msb_right) ? "true" : "false",
        (unsigned)varinfo.transp.offset, (unsigned)varinfo.transp.length, (varinfo.transp.msb_right) ? "true" : "false"
    );
    return true;
}
DEFCMD(color) {
    char hexcode[6];
    int hexlen = 0;
    while (1) {
        char c = tolower(*cmd);
        if (c >= '0' && c <= '9') {
            hexcode[hexlen++] = c;
            ++cmd;
        } else if (c >= 'a' && c <= 'f') {
            hexcode[hexlen++] = c;
            ++cmd;
        } else {
            if (hexlen == 3) {
                if (hexcode[0] >= '0' && hexcode[0] <= '9') color[0] = hexcode[0] - '0';
                else color[0] = hexcode[0] - 'a' + 10;
                color[0] |= color[0] << 4;
                if (hexcode[1] >= '0' && hexcode[1] <= '9') color[1] = hexcode[1] - '0';
                else color[1] = hexcode[1] - 'a' + 10;
                color[1] |= color[1] << 4;
                if (hexcode[2] >= '0' && hexcode[2] <= '9') color[2] = hexcode[2] - '0';
                else color[2] = hexcode[2] - 'a' + 10;
                color[2] |= color[2] << 4;
            }
            break;
        }
        if (hexlen == 6) {
            if (hexcode[0] >= '0' && hexcode[0] <= '9') color[0] = (hexcode[0] - '0') << 4;
            else color[0] = (hexcode[0] - 'a' + 10) << 4;
            if (hexcode[1] >= '0' && hexcode[1] <= '9') color[0] |= hexcode[1] - '0';
            else color[0] |= hexcode[1] - 'a' + 10;
            if (hexcode[2] >= '0' && hexcode[2] <= '9') color[1] = (hexcode[2] - '0') << 4;
            else color[1] = (hexcode[2] - 'a' + 10) << 4;
            if (hexcode[3] >= '0' && hexcode[3] <= '9') color[1] |= hexcode[3] - '0';
            else color[1] |= hexcode[3] - 'a' + 10;
            if (hexcode[4] >= '0' && hexcode[4] <= '9') color[2] = (hexcode[4] - '0') << 4;
            else color[2] = (hexcode[4] - 'a' + 10) << 4;
            if (hexcode[5] >= '0' && hexcode[5] <= '9') color[2] |= hexcode[5] - '0';
            else color[2] |= hexcode[5] - 'a' + 10;
            break;
        }
    }
    ENDARGS();
    if (verbose) printf("SET COLOR TO #%02X%02X%02X\n", color[0], color[1], color[2]);
    return true;
}
DEFCMD(pixel) {
    int x, y;
    PARSEINT(x);
    NEXTARG();
    PARSEINT(y);
    ENDARGS();
    PUTPIX(x, y, color[0], color[1], color[2]);
    if (verbose) printf("SET PIXEL (%d, %d)\n", x, y);
    return true;
}
DEFCMD(line) {
    int x1, y1, x2, y2;
    PARSEINT(x1);
    NEXTARG();
    PARSEINT(y1);
    NEXTARG();
    PARSEINT(x2);
    NEXTARG();
    PARSEINT(y2);
    ENDARGS();
    bool usefast;
    if (x1 < 0 || x1 >= varinfo.xres) usefast = false;
    else if (y1 < 0 || y1 >= varinfo.yres) usefast = false;
    else if (x2 < 0 || x2 >= varinfo.xres) usefast = false;
    else if (y2 < 0 || y2 >= varinfo.yres) usefast = false;
    else usefast = true;
    if (y1 == y2) {
        if (x1 > x2) {int tmp = x1; x1 = x2; x2 = tmp; tmp = y1; y1 = y2; y2 = tmp;}
        PUTLINE(y1, x1, x2, color[0], color[1], color[2]);
    } else if (x1 == x2) {
        for (int y = y1; y <= y2; ++y) {
            PUTPIX(x1, y, color[0], color[1], color[2]);
        }
    } else {
        int dx = abs(x2 - x1);
        int sx = (x1 < x2) ? 1 : -1;
        int dy = -abs(y2 - y1);
        int sy = (y1 < y2) ? 1 : -1;
        int e = dx + dy;
        int x = x1, y = y1;
        if (usefast) {
            while (1) {
                PUTPIX(x, y, color[0], color[1], color[2]);
                if (x == x2 && y == y2) break;
                int e2 = e * 2;
                if (e2 >= dy) {
                    e += dy;
                    x += sx;
                }
                if (e2 <= dx) {
                    e += dx;
                    y += sy;
                }
            }
        }
    }
    if (verbose) printf("DREW LINE FROM (%d, %d) TO (%d, %d)%s\n", x1, y1, x2, y2, (usefast) ? "" : " (bounds checked)");
    return true;
}
DEFCMD(rect) {
    int x1, y1, x2, y2;
    PARSEINT(x1);
    NEXTARG();
    PARSEINT(y1);
    NEXTARG();
    PARSEINT(x2);
    NEXTARG();
    PARSEINT(y2);
    ENDARGS();
    if (x1 < 0) x1 = 0;
    else if (x1 >= varinfo.xres) x1 = varinfo.xres - 1;
    if (y1 < 0) y1 = 0;
    else if (y1 >= varinfo.yres) y1 = varinfo.yres - 1;
    if (x2 < 0) x2 = 0;
    else if (x2 >= varinfo.xres) x2 = varinfo.xres - 1;
    if (y2 < 0) y2 = 0;
    else if (y2 >= varinfo.yres) y2 = varinfo.yres - 1;
    if (x1 > x2) {int tmp = x1; x1 = x2; x2 = tmp;}
    if (y1 > y2) {int tmp = y1; y1 = y2; y2 = tmp;}
    for (int y = y1; y <= y2; ++y) {
        PUTLINE_FAST(y, x1, x2, color[0], color[1], color[2]);
    }
    if (verbose) printf("DREW RECT FROM (%d, %d) TO (%d, %d)\n", x1, y1, x2, y2);
    return true;
}
#if 0
static inline bool cmd_tri_istopleft(int startx, int starty, int endx, int endy) {
    int edgex = endx - startx;
    int edgey = endy - starty;
    return (!edgey && edgex > 0) || edgey < 0;
}
#endif
DEFCMD(tri) {
    int x1, y1, x2, y2, x3, y3;
    PARSEINT(x1);
    NEXTARG();
    PARSEINT(y1);
    NEXTARG();
    PARSEINT(x2);
    NEXTARG();
    PARSEINT(y2);
    NEXTARG();
    PARSEINT(x3);
    NEXTARG();
    PARSEINT(y3);
    ENDARGS();
    bool usefast;
    if (x1 < 0 || x1 >= varinfo.xres) usefast = false;
    else if (y1 < 0 || y1 >= varinfo.yres) usefast = false;
    else if (x2 < 0 || x2 >= varinfo.xres) usefast = false;
    else if (y2 < 0 || y2 >= varinfo.yres) usefast = false;
    else if (x3 < 0 || x3 >= varinfo.xres) usefast = false;
    else if (y3 < 0 || y3 >= varinfo.yres) usefast = false;
    else usefast = true;
    if (y1 > y2) {int tmp = x1; x1 = x2; x2 = tmp; tmp = y1; y1 = y2; y2 = tmp;}
    if (y1 > y3) {int tmp = x1; x1 = x3; x3 = tmp; tmp = y1; y1 = y3; y3 = tmp;}
    if (y2 > y3) {int tmp = x2; x2 = x3; x3 = tmp; tmp = y2; y2 = y3; y3 = tmp;}

    if (y3 == y1) return true;

    int dx1 = x3 - x1;
    int dy1 = y3 - y1;
    int d1 = dx1 / dy1;
    int r1 = dx1 % dy1;
    int e1 = 0;
    int ec1 = (r1 >= 0 && d1 >= 0) ? 1 : -1;
    //printf("DR1: [%d, %d/%d]\n", d1, r1, dy1);

    int outx1 = x1;

    int dx2, dy2, d2, r2, e2, ec2;

    if (y2 != y1) {

        dx2 = x2 - x1;
        dy2 = y2 - y1;
        d2 = dx2 / dy2;
        r2 = dx2 % dy2;
        e2 = 0;
        ec2 = (r2 >= 0 && d2 >= 0) ? 1 : -1;
        //printf("DR2: [%d, %d/%d]\n", d2, r2, dy2);

        for (int y = y1, outx2 = x1; y < y2; ++y) {
            //PUTPIX(outx1, y, color[0], color[1], color[2]);
            //PUTPIX(outx2, y, color[0], color[1], color[2]);
            if (outx2 > outx1) {
                //printf("Line [%d]: [%d - %d]\n", y, outx1, outx2);
                if (usefast) {
                    PUTLINE_FAST(y, outx1, outx2, color[0], color[1], color[2]);
                } else {
                    PUTLINE(y, outx1, outx2, color[0], color[1], color[2]);
                }
            } else {
                //printf("Line [%d]: [%d - %d]\n", y, outx2, outx1);
                if (usefast) {
                    PUTLINE_FAST(y, outx2, outx1, color[0], color[1], color[2]);
                } else {
                    PUTLINE(y, outx2, outx1, color[0], color[1], color[2]);
                }
            }
            outx1 += d1;
            e1 += r1;
            if (ec1 == 1 && e1 >= dy1) {outx1 += ec1; e1 -= dy1;}
            else if (e1 <= -dy1) {outx1 += ec1; e1 += dy1;}
            outx2 += d2;
            e2 += r2;
            if (ec2 == 1 && e2 >= dy2) {outx2 += ec2; e2 -= dy2;}
            else if (e2 <= -dy2) {outx2 += ec2; e2 += dy2;}
        }

    }

    if (y3 == y2) return true;

    dx2 = x3 - x2;
    dy2 = y3 - y2;
    d2 = dx2 / dy2;
    r2 = dx2 % dy2;
    e2 = 0;
    ec2 = (r2 >= 0 && d2 >= 0) ? 1 : -1;
    //printf("DR3: [%d, %d/%d]\n", d2, r2, dy2);

    for (int y = y2, outx2 = x2; y <= y3; ++y) {
        PUTPIX(outx1, y, color[0], color[1], color[2]);
        PUTPIX(outx2, y, color[0], color[1], color[2]);
        if (outx2 > outx1) {
            //printf("Line [%d]: [%d - %d]\n", y, outx1, outx2);
            if (usefast) {
                PUTLINE_FAST(y, outx1, outx2, color[0], color[1], color[2]);
            } else {
                PUTLINE(y, outx1, outx2, color[0], color[1], color[2]);
            }
        } else {
            //printf("Line [%d]: [%d - %d]\n", y, outx2, outx1);
            if (usefast) {
                PUTLINE_FAST(y, outx2, outx1, color[0], color[1], color[2]);
            } else {
                PUTLINE(y, outx2, outx1, color[0], color[1], color[2]);
            }
        }
        outx1 += d1;
        e1 += r1;
        if (ec1 == 1 && e1 >= dy1) {outx1 += ec1; e1 -= dy1;}
        else if (e1 <= -dy1) {outx1 += ec1; e1 += dy1;}
        outx2 += d2;
        e2 += r2;
        if (ec2 == 1 && e2 >= dy2) {outx2 += ec2; e2 -= dy2;}
        else if (e2 <= -dy2) {outx2 += ec2; e2 += dy2;}
    }

    if (verbose) printf("DREW TRI (%d, %d), (%d, %d), (%d, %d)%s\n", x1, y1, x2, y2, x3, y3, (usefast) ? "" : " (bounds checked)");
    return true;
}
