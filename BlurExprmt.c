#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "Utils.h"
#include "Picture.h"
#include "time.h"
#include "thpool.h"

/* ---------- definitions ---------- */
#define NO_RGB_COMPONENTS 3
#define BLUR_REGION_SIZE 9
#define BILLION 1000000000.0
#define NUM_THREADS 16
struct task_args
{
  int i;
  int j;
  struct picture *input;
  struct picture *output;
};

struct sector_args
{
  int i;
  int j;
  struct picture *input;
  struct picture *output;
  int sector_size;
};

/* ---------- picture transformation functions ---------- */

/* helper function to calculate new pixel value for blur */
void calculate_new_blur_pixel(int i, int j, struct picture *input, struct picture *output)
{
  // set-up a local pixel on the stack
  struct pixel rgb = get_pixel(input, i, j);

  // don't need to modify boundary pixels
  if (i != 0 && j != 0 && i != output->width - 1 && j != output->height - 1)
  {

    // set up running RGB component totals for pixel region
    int sum_red = rgb.red;
    int sum_green = rgb.green;
    int sum_blue = rgb.blue;

    // check the surrounding pixel region
    for (int n = -1; n <= 1; n++)
    {
      for (int m = -1; m <= 1; m++)
      {
        if (n != 0 || m != 0)
        {
          rgb = get_pixel(input, i + n, j + m);
          sum_red += rgb.red;
          sum_green += rgb.green;
          sum_blue += rgb.blue;
        }
      }
    }

    // compute average pixel RGB value
    rgb.red = sum_red / BLUR_REGION_SIZE;
    rgb.green = sum_green / BLUR_REGION_SIZE;
    rgb.blue = sum_blue / BLUR_REGION_SIZE;
  }

  // set pixel to computed region RBG value (unmodified if boundary)
  set_pixel(output, i, j, &rgb);
}

/* sequential verison */
void blur_picture(struct picture *pic)
{
  // make new temporary picture to work in
  struct picture tmp;
  init_picture_from_size(&tmp, pic->width, pic->height);

  // iterate over each pixel in the picture
  for (int i = 0; i < tmp.width; i++)
  {
    for (int j = 0; j < tmp.height; j++)
    {
      calculate_new_blur_pixel(i, j, pic, &tmp);
    }
  }

  // clean-up the old picture and replace with new picture
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

/* column by column version */
// helper function runs by child for column blur
void *help_column_blur(struct task_args *args)
{
  int i = args->i;
  struct picture *input = args->input;
  struct picture *output = args->output;
  int height = output->height;

  for (int j = 0; j < height; j++)
  {
    calculate_new_blur_pixel(i, j, input, output);
  }

  free(args);
  return NULL;
}

void column_blur_picture(struct picture *pic)
{
  struct picture tmp;
  init_picture_from_size(&tmp, pic->width, pic->height);
  int height = tmp.height;
  int width = tmp.width;
  // init a thread pool
  threadpool thpool = thpool_init(NUM_THREADS);

  for (int i = 0; i < width; i++)
  {
    struct task_args *args = malloc(sizeof(struct task_args));
    args->i = i;
    args->input = pic;
    args->output = &tmp;
    thpool_add_work(thpool, (void (*)(void *))help_column_blur, args);
  }
  // cleaning up
  thpool_wait(thpool);
  thpool_destroy(thpool);
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

/* row by row version */
// helper function runs by child for row blur
void *help_row_blur(struct task_args *args)
{
  int j = args->j;
  struct picture *input = args->input;
  struct picture *output = args->output;
  int width = output->width;

  for (int i = 0; i < width; i++)
  {
    calculate_new_blur_pixel(i, j, input, output);
  }

  free(args);
  return NULL;
}

void row_blur_picture(struct picture *pic)
{
  struct picture tmp;
  init_picture_from_size(&tmp, pic->width, pic->height);
  int height = tmp.height;
  int width = tmp.width;
  threadpool thpool = thpool_init(NUM_THREADS);

  for (int j = 0; j < height; j++)
  {
    struct task_args *args = malloc(sizeof(struct task_args));
    args->j = j;
    args->input = pic;
    args->output = &tmp;
    thpool_add_work(thpool, (void (*)(void *))help_row_blur, args);
  }
  thpool_wait(thpool);
  thpool_destroy(thpool);
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

/* pixel by pixel version */
// helper function runs by child for parallel blur
void *help_parallel_blur(struct task_args *args)
{
  int i = args->i;
  int j = args->j;
  struct picture *input = args->input;
  struct picture *output = args->output;

  calculate_new_blur_pixel(i, j, input, output);

  free(args);
  return NULL;
}

void parallel_blur_picture(struct picture *pic)
{
  struct picture tmp;
  init_picture_from_size(&tmp, pic->width, pic->height);
  int height = tmp.height;
  int width = tmp.width;
  threadpool thpool = thpool_init(NUM_THREADS);

  for (int i = 0; i < width; i++)
  {
    for (int j = 0; j < height; j++)
    {
      struct task_args *args = malloc(sizeof(struct task_args));
      args->i = i;
      args->j = j;
      args->input = pic;
      args->output = &tmp;
      thpool_add_work(thpool, (void (*)(void *))help_parallel_blur, args);
    }
  }
  thpool_wait(thpool);
  thpool_destroy(thpool);
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

/* sector by sector version */
// helper function runs by child for sector blur
void *help_sector_blur(struct sector_args *args)
{
  int x = args->i;
  int y = args->j;
  struct picture *input = args->input;
  struct picture *output = args->output;
  int sector_size = args->sector_size;

  for (int i = x; i < x + sector_size; i++)
  {
    for (int j = y; j < y + sector_size; j++)
    {
      if (i >= 0 && i < output->width && j >= 0 && j < output->height)
      {
        calculate_new_blur_pixel(i, j, input, output);
      }
    }
  }
  free(args);
  return NULL;
}

void sector_blur_picture(struct picture *pic, int sector_size)
{
  struct picture tmp;
  init_picture_from_size(&tmp, pic->width, pic->height);
  int height = tmp.height;
  int width = tmp.width;
  threadpool thpool = thpool_init(NUM_THREADS);

  for (int i = 0; i < width; i += sector_size)
  {
    for (int j = 0; j < height; j += sector_size)
    {

      struct sector_args *args = malloc(sizeof(struct task_args));
      args->i = i;
      args->j = j;
      args->input = pic;
      args->output = &tmp;
      args->sector_size = sector_size;
      thpool_add_work(thpool, (void (*)(void *))help_sector_blur, args);
    }
  }
  thpool_wait(thpool);
  thpool_destroy(thpool);
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

/* ---------- MAIN PROGRAM ---------- */

void adjust_time(struct timespec *start, struct timespec *end)
{
  if (end->tv_nsec < start->tv_nsec)
  {
    end->tv_nsec += BILLION;
    end->tv_sec--;
  }
}

/* functions for general test */
// function pointer look-up table for picture transformation functions
typedef void (*cmd)(struct picture *);
cmd cmds[] = {
    blur_picture,
    column_blur_picture,
    row_blur_picture,
    parallel_blur_picture};

// names for output message and file
char *names[] = {
    "sequential",
    "column",
    "row",
    "parallel"};

/* functions for sector test */
// value of sector size
int sector_sizes[] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
// names for output message and file
char *sector_names[] = {
    "sector_2",
    "sector_4",
    "sector_8",
    "sector_16",
    "sector_32",
    "sector_64",
    "sector_128",
    "sector_256",
    "sector_512",
    "sector_1024",
    "sector_2048",
    "sector_4096"};

// image will be compared with sequential version
void img_cmp(char *name)
{

  // create provided image objects
  struct picture pic1;
  struct picture pic2;

  init_picture_from_file(&pic1, "sequential.jpg");
  init_picture_from_file(&pic2, name);

  int width = pic1.width;
  int height = pic1.height;

  if (width != pic2.width || height != pic2.height)
  {
    printf("[!] fail - pictures do not have equal dimensions\n");
    clear_picture(&pic1);
    clear_picture(&pic2);
    return;
  }

  // iterate over the picture pixel-by-pixel and compare RGB values
  for (int i = 0; i < width; i++)
  {
    for (int j = 0; j < height; j++)
    {
      struct pixel pixel1 = get_pixel(&pic1, i, j);
      struct pixel pixel2 = get_pixel(&pic2, i, j);

      int red_diff = pixel1.red - pixel2.red;
      int green_diff = pixel1.green - pixel2.green;
      int blue_diff = pixel1.blue - pixel2.blue;

      if (red_diff > 1 || red_diff < -1 || green_diff > 1 || green_diff < -1 || blue_diff > 1 || blue_diff < -1)
      {
        printf("[!] fail - pictures not equal at cell (%i,%i)\n", i, j);
        printf("    pixel1 RGB = \t(%i,\t %i,\t %i)\n", pixel1.red, pixel1.green, pixel1.blue);
        printf("    pixel2 RGB = \t(%i,\t %i,\t %i)\n", pixel2.red, pixel2.green, pixel2.blue);
        clear_picture(&pic1);
        clear_picture(&pic2);
        return;
      }
    }
  }
  clear_picture(&pic1);
  clear_picture(&pic2);
  printf("success - pictures identical!\n");
}

int main(int argc, char **argv)
{

  // define variables
  struct picture pic;
  struct timespec start, end;

  char *file_name = "images/test_large.jpg";

  // run each picture transformation function except for sector blur
  for (int i = 0; i < sizeof(names) / sizeof(names[0]); i++)
  {
    printf("Running %s blur...\n", names[i]);
    clock_gettime(CLOCK_MONOTONIC, &start);
    init_picture_from_file(&pic, file_name);
    // run each function 10 times
    for (int j = 0; j < 10; j++)
    {
      cmds[i](&pic);
    }
    char out[80];
    strcpy(out, names[i]);
    strcat(out, ".jpg");
    save_picture_to_file(&pic, out);
    clock_gettime(CLOCK_MONOTONIC, &end);
    adjust_time(&start, &end);
    printf("time taken: %ld.%ld\n", end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);
    // comparing output image with sequential version
    img_cmp(out);
  }

  // run each picture transformation function with different sector sizes
  for (int i = 0; i < sizeof(sector_sizes) / sizeof(sector_sizes[0]); i++)
  {
    printf("Running %s blur...\n", sector_names[i]);
    clock_gettime(CLOCK_MONOTONIC, &start);
    init_picture_from_file(&pic, file_name);
    for (int j = 0; j < 10; j++)
    {
      sector_blur_picture(&pic, sector_sizes[i]);
    }
    char out[80];
    strcpy(out, sector_names[i]);
    strcat(out, ".jpg");
    save_picture_to_file(&pic, out);
    clock_gettime(CLOCK_MONOTONIC, &end);
    adjust_time(&start, &end);
    printf("time taken: %ld.%ld\n", end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);
    img_cmp(out);
  }

  // choose a optimal image
  // whether blur is obsevable
  // recording execution times
  // whether it is observable
}
