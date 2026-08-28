/**
 * camera_capture - Capture a still image on Raspberry Pi Zero 2 W using libcamera.
 *
 * Usage:
 *   camera_capture [OPTIONS]
 *
 * Options:
 *   -o, --output <file>   Output file name (default: capture.jpg)
 *   -w, --width  <px>     Image width  in pixels (default: 1920)
 *   -H, --height <px>     Image height in pixels (default: 1080)
 *   -t, --timeout <ms>    Camera warm-up time in milliseconds (default: 2000)
 *   --help                Show this help message and exit
 *
 * The program uses the libcamera still-capture utility (libcamera-still) which
 * is included in the Raspberry Pi OS libcamera-apps package.  libcamera-still
 * handles all low-level camera initialisation, ISP tuning, and JPEG encoding;
 * this program is a thin C wrapper that provides a clean CLI and validates
 * arguments before delegating to that utility.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>

/* Maximum length for a file-name argument. */
#define MAX_FILENAME_LEN 256

/* Sentinel val for the --help long option (not a printable ASCII character). */
#define OPT_HELP 1

/* Default capture parameters. */
#define DEFAULT_OUTPUT   "capture.jpg"
#define DEFAULT_WIDTH    1920
#define DEFAULT_HEIGHT   1080
#define DEFAULT_TIMEOUT  2000   /* milliseconds */

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

typedef struct {
    char output[MAX_FILENAME_LEN];
    int  width;
    int  height;
    int  timeout_ms;
} CaptureConfig;

/* --------------------------------------------------------------------------
 * Helper functions
 * -------------------------------------------------------------------------- */

static void print_usage(const char *prog)
{
    fprintf(stdout,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Capture a still image using the Raspberry Pi camera interface.\n"
        "\n"
        "Options:\n"
        "  -o, --output <file>   Output file name          (default: %s)\n"
        "  -w, --width  <px>     Image width  in pixels    (default: %d)\n"
        "  -H, --height <px>     Image height in pixels    (default: %d)\n"
        "  -t, --timeout <ms>    Camera warm-up time (ms)  (default: %d)\n"
        "      --help            Show this help and exit\n"
        "\n"
        "Examples:\n"
        "  %s\n"
        "  %s -o photo.jpg -w 3280 -H 2464\n"
        "  %s --output my_photo.jpg --timeout 3000\n",
        prog,
        DEFAULT_OUTPUT, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_TIMEOUT,
        prog, prog, prog);
}

/**
 * Return 1 if @filename contains only safe characters for use as a shell
 * argument, or 0 if it contains potentially dangerous characters.
 *
 * Allowed: alphanumerics, '-', '_', '.', '/', '@', '+', '='
 * Everything else (including spaces, quotes, semicolons, etc.) is rejected.
 */
static int is_safe_filename(const char *filename)
{
    for (const char *p = filename; *p != '\0'; p++) {
        unsigned char c = (unsigned char)*p;
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' ||
            c == '/' || c == '@' || c == '+' || c == '=') {
            continue;
        }
        return 0;
    }
    return 1;
}

/**
 * Parse a positive integer from @str.
 * Returns the value on success, or -1 on error (prints a message to stderr).
 */
static int parse_positive_int(const char *str, const char *param_name)
{
    char *end = NULL;
    errno = 0;
    long val = strtol(str, &end, 10);

    if (errno != 0 || end == str || *end != '\0') {
        fprintf(stderr, "error: '%s' is not a valid integer for %s\n",
                str, param_name);
        return -1;
    }
    if (val <= 0 || val > INT_MAX) {
        fprintf(stderr, "error: %s must be a positive integer (got %ld)\n",
                param_name, val);
        return -1;
    }
    return (int)val;
}

/**
 * Parse command-line arguments and populate @cfg.
 * Returns 0 on success, 1 if --help was requested, -1 on error.
 */
static int parse_args(int argc, char *argv[], CaptureConfig *cfg)
{
    /* Initialise defaults. */
    strncpy(cfg->output, DEFAULT_OUTPUT, MAX_FILENAME_LEN - 1);
    cfg->output[MAX_FILENAME_LEN - 1] = '\0';
    cfg->width      = DEFAULT_WIDTH;
    cfg->height     = DEFAULT_HEIGHT;
    cfg->timeout_ms = DEFAULT_TIMEOUT;

    static const struct option long_opts[] = {
        { "output",  required_argument, NULL, 'o' },
        { "width",   required_argument, NULL, 'w' },
        { "height",  required_argument, NULL, 'H' },
        { "timeout", required_argument, NULL, 't' },
        { "help",    no_argument,       NULL, OPT_HELP },
        { NULL, 0, NULL, 0 }
    };

    int opt;
    int long_idx;
    while ((opt = getopt_long(argc, argv, "o:w:H:t:", long_opts, &long_idx)) != -1) {
        switch (opt) {
        case 'o':
            if (optarg[0] == '\0') {
                fprintf(stderr, "error: output file name cannot be empty\n");
                return -1;
            }
            if (strlen(optarg) >= MAX_FILENAME_LEN) {
                fprintf(stderr, "error: output file name is too long "
                        "(max %d characters)\n", MAX_FILENAME_LEN - 1);
                return -1;
            }
            if (!is_safe_filename(optarg)) {
                fprintf(stderr, "error: output file name contains unsafe "
                        "characters.\n"
                        "       Allowed: alphanumerics, - _ . / @ + =\n");
                return -1;
            }
            strncpy(cfg->output, optarg, MAX_FILENAME_LEN - 1);
            cfg->output[MAX_FILENAME_LEN - 1] = '\0';
            break;

        case 'w': {
            int v = parse_positive_int(optarg, "--width");
            if (v < 0) return -1;
            cfg->width = v;
            break;
        }

        case 'H': {
            int v = parse_positive_int(optarg, "--height");
            if (v < 0) return -1;
            cfg->height = v;
            break;
        }

        case 't': {
            int v = parse_positive_int(optarg, "--timeout");
            if (v < 0) return -1;
            cfg->timeout_ms = v;
            break;
        }

        case OPT_HELP:
            return 1; /* Caller should print help and exit 0. */

        default:
            fprintf(stderr, "Run '%s --help' for usage.\n", argv[0]);
            return -1;
        }
    }

    /* Reject unexpected positional arguments. */
    if (optind < argc) {
        fprintf(stderr, "error: unexpected argument '%s'\n", argv[optind]);
        fprintf(stderr, "Run '%s --help' for usage.\n", argv[0]);
        return -1;
    }

    return 0;
}

/**
 * Build the libcamera-still command string and execute it.
 * Returns 0 on success, non-zero on failure.
 */
static int capture_image(const CaptureConfig *cfg)
{
    /*
     * libcamera-still is part of the libcamera-apps package on Raspberry Pi OS.
     * Flags used:
     *   -o <file>           : output file path
     *   --width  <px>       : capture width
     *   --height <px>       : capture height
     *   -t <ms>             : time (ms) to run before capturing; allows the
     *                         AGC and AWB to settle.
     *   --nopreview         : disable the preview window (headless operation)
     */

    /* We construct the command into a fixed-size buffer.  The size is chosen
     * to comfortably hold all numeric values plus the file name. */
    char cmd[MAX_FILENAME_LEN + 256];
    int n = snprintf(cmd, sizeof(cmd),
                     "libcamera-still -o %s --width %d --height %d "
                     "-t %d --nopreview",
                     cfg->output, cfg->width, cfg->height, cfg->timeout_ms);

    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        fprintf(stderr, "error: failed to build capture command\n");
        return -1;
    }

    fprintf(stdout, "Capturing image: %s\n", cmd);

    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "error: libcamera-still exited with status %d\n", ret);
        return ret;
    }

    fprintf(stdout, "Image saved to: %s\n", cfg->output);
    return 0;
}

/* --------------------------------------------------------------------------
 * Entry point
 * -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    CaptureConfig cfg;
    int rc = parse_args(argc, argv, &cfg);

    if (rc == 1) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    if (rc != 0) {
        return EXIT_FAILURE;
    }

    fprintf(stdout,
            "camera_capture configuration:\n"
            "  Output  : %s\n"
            "  Width   : %d px\n"
            "  Height  : %d px\n"
            "  Timeout : %d ms\n",
            cfg.output, cfg.width, cfg.height, cfg.timeout_ms);

    return (capture_image(&cfg) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
