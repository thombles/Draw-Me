#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wand/MagickWand.h>

#include "palette.h"

void fail(char *reason)
{
    printf("Failure: %s\n", reason);
    exit(1);
}

int get_terminal_width()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

MagickWand* load_image(char *file)
{
    MagickWand *wand = NewMagickWand();
    MagickBooleanType status = MagickReadImage(wand, file);
    if (status == MagickFalse) {
        fail("Couldn't load image");
    }
    return wand;
}

void resize(MagickWand* wand, size_t width)
{
    size_t image_width = MagickGetImageWidth(wand);
    size_t image_height = MagickGetImageHeight(wand);
    if (!image_width || !image_height) {
        fail("Image has 0 dimension");
    }
    double aspect = (double)image_width / (double)image_height;
    size_t height = (int)((double)width / aspect + 0.5);

    /* Now we have 'width' and 'height' for our target image */
    MagickBooleanType success;
    success = MagickResizeImage(wand, width, height, LanczosFilter, 1.0);
    if (success == MagickFalse) {
        fail("Resizing source image failed");
    }
}

void remap(MagickWand *image, MagickWand *palette)
{
    MagickBooleanType success = MagickRemapImage(image, palette, NoDitherMethod);
    if (success == MagickFalse) {
        fail("Problem remapping image");
    }
}

int code_for_pixel(PixelWand *pixel)
{
    char *colour_string = PixelGetColorAsNormalizedString(pixel);
    double r, g, b;
    sscanf(colour_string, "%lf,%lf,%lf", &r, &g, &b);
    free(colour_string);
    int ri, gi, bi;
    ri = (int)(r*255+0.5);
    gi = (int)(g*255+0.5);
    bi = (int)(b*255+0.5);
    char rgb[16];
    sprintf(rgb, "#%02x%02x%02x", ri, gi, bi);
    return code_for_colour(rgb);
}

void render(MagickWand *image)
{
    size_t image_width = MagickGetImageWidth(image);
    size_t image_height = MagickGetImageHeight(image);

    PixelWand *lower_pixel = NewPixelWand();
    PixelWand *upper_pixel = NewPixelWand();

    for (int i = 0; i < image_height; i += 2) {
        for (int j = 0; j < image_width; j++) {
            MagickGetImagePixelColor(image, j, i, upper_pixel);
            int upper_code = code_for_pixel(upper_pixel);
            int lower_code = 0; /* black */
            if (i+1 < image_height) { /* if a second row is present */
                MagickGetImagePixelColor(image, j, i+1, lower_pixel);
                lower_code = code_for_pixel(lower_pixel);
            }
            /* Upper = background, lower = foreground */
            printf("\x1b[48;5;%dm", upper_code);
            printf("\x1b[38;5;%dm", lower_code);
            printf("▅");
        }
    }

    /* Reset colours */
    printf("\x1b[0m");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fail("Bad argument");
    }

    /* Init imagemagick */
    MagickWandGenesis();

    /* Figure out how many "pixels" wide our image can be */
    size_t width = get_terminal_width();

    /* Load the source image into wand */
    MagickWand *wand = load_image(argv[1]);

    /* Resize it to the available width */
    resize(wand, width);

    /* Generate an in-memory palette of the colours we want to use */
    MagickWand *palette = generate_palette();

    /* Quantise the colours in the source image to match our reference palette */
    remap(wand, palette);

    /* Display the processed image on screen */
    render(wand);

    /* Clean up */
    wand = DestroyMagickWand(wand);
    MagickWandTerminus();
}
