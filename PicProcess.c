#include "PicProcess.h"
#include <pthread.h>
#include <unistd.h>

#define NO_RGB_COMPONENTS 3
#define BLUR_REGION_SIZE 9
#define THREAD_NUM 3000
struct task_args
{
  int i;
  int j;
  struct picture *input;
  struct picture *output;
};

void invert_picture(struct picture *pic)
{
  // iterate over each pixel in the picture
  for (int i = 0; i < pic->width; i++)
  {
    for (int j = 0; j < pic->height; j++)
    {
      struct pixel rgb = get_pixel(pic, i, j);

      // invert RGB values of pixel
      rgb.red = MAX_PIXEL_INTENSITY - rgb.red;
      rgb.green = MAX_PIXEL_INTENSITY - rgb.green;
      rgb.blue = MAX_PIXEL_INTENSITY - rgb.blue;

      // set pixel to inverted RBG values
      set_pixel(pic, i, j, &rgb);
    }
  }
}

void grayscale_picture(struct picture *pic)
{
  // iterate over each pixel in the picture
  for (int i = 0; i < pic->width; i++)
  {
    for (int j = 0; j < pic->height; j++)
    {
      struct pixel rgb = get_pixel(pic, i, j);

      // compute gray average of pixel's RGB values
      int avg = (rgb.red + rgb.green + rgb.blue) / NO_RGB_COMPONENTS;
      rgb.red = avg;
      rgb.green = avg;
      rgb.blue = avg;

      // set pixel to gray-scale RBG value
      set_pixel(pic, i, j, &rgb);
    }
  }
}

void rotate_picture(struct picture *pic, int angle)
{
  // capture current picture size
  int new_width = pic->width;
  int new_height = pic->height;

  // adjust output picture size as necessary
  if (angle == 90 || angle == 270)
  {
    new_width = pic->height;
    new_height = pic->width;
  }

  // make new temporary picture to work in
  struct picture tmp;
  init_picture_from_size(&tmp, new_width, new_height);

  // iterate over each pixel in the picture
  for (int i = 0; i < new_width; i++)
  {
    for (int j = 0; j < new_height; j++)
    {
      struct pixel rgb;
      // determine rotation angle and execute corresponding pixel update
      switch (angle)
      {
      case (90):
        rgb = get_pixel(pic, j, new_width - 1 - i);
        break;
      case (180):
        rgb = get_pixel(pic, new_width - 1 - i, new_height - 1 - j);
        break;
      case (270):
        rgb = get_pixel(pic, new_height - 1 - j, i);
        break;
      default:
        printf("[!] rotate is undefined for angle %i (must be 90, 180 or 270)\n", angle);
        clear_picture(&tmp);
        clear_picture(pic);
        exit(IO_ERROR);
      }
      set_pixel(&tmp, i, j, &rgb);
    }
  }

  // clean-up the old picture and replace with new picture
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

void flip_picture(struct picture *pic, char plane)
{
  // make new temporary picture to work in
  struct picture tmp;
  init_picture_from_size(&tmp, pic->width, pic->height);

  // iterate over each pixel in the picture
  for (int i = 0; i < tmp.width; i++)
  {
    for (int j = 0; j < tmp.height; j++)
    {
      struct pixel rgb;
      // determine flip plane and execute corresponding pixel update
      switch (plane)
      {
      case ('V'):
        rgb = get_pixel(pic, i, tmp.height - 1 - j);
        break;
      case ('H'):
        rgb = get_pixel(pic, tmp.width - 1 - i, j);
        break;
      default:
        printf("[!] flip is undefined for plane %c\n", plane);
        clear_picture(&tmp);
        clear_picture(pic);
        exit(IO_ERROR);
      }
      set_pixel(&tmp, i, j, &rgb);
    }
  }

  // clean-up the old picture and replace with new picture
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

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

      // set-up a local pixel on the stack
      struct pixel rgb = get_pixel(pic, i, j);

      // don't need to modify boundary pixels
      if (i != 0 && j != 0 && i != tmp.width - 1 && j != tmp.height - 1)
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
              rgb = get_pixel(pic, i + n, j + m);
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
      set_pixel(&tmp, i, j, &rgb);
    }
  }

  // clean-up the old picture and replace with new picture
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

pthread_mutex_t lock;
// record the number of threads running
int count = 0;
void *help_blur(struct task_args *args)
{

  int i = args->i;
  int j = args->j;
  struct picture *input = args->input;
  struct picture *output = args->output;
  pthread_mutex_lock(&lock);
  int width = input->width;
  int height = input->height;
  // set-up a local pixel on the stack
  struct pixel rgb = get_pixel(input, i, j);
  count++;
  pthread_mutex_unlock(&lock);

  // don't need to modify boundary pixels
  if (i != 0 && j != 0 && i != width - 1 && j != height - 1)
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
          pthread_mutex_lock(&lock);
          rgb = get_pixel(input, i + n, j + m);
          pthread_mutex_unlock(&lock);
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
  pthread_mutex_lock(&lock);
  set_pixel(output, i, j, &rgb);
  count--;
  pthread_mutex_unlock(&lock);
  free(args);
  pthread_exit(NULL);
}

void parallel_blur_picture(struct picture *pic)
{
  // make new temporary picture to work in
  struct picture tmp;
  init_picture_from_size(&tmp, pic->width, pic->height);
  int height = tmp.height;
  int width = tmp.width;
  int num_threads = width * height;
  pthread_t threads[num_threads];
  // iterate over each pixel in the picture
  printf("creating threads\n");
  pthread_mutex_init(&lock, NULL);
  int pre = 0;
  for (int k = 0; k < num_threads; k++)
  {
    // waiting for threads to finish
    if (k % THREAD_NUM == 0)
    {
      pthread_mutex_unlock(&lock);
      // 0.5s
      // usleep(500000);
      for (int i = pre; i < k; i++)
      {
        pthread_join(threads[i], NULL);
      }
      pre = k;
    }
    // lock will only release when waiting for threads
    pthread_mutex_trylock(&lock);
    struct task_args *args = malloc(sizeof(struct task_args));
    args->i = k / height;
    args->j = k % height;
    args->input = pic;
    args->output = &tmp;
    pthread_create(&threads[k], NULL, (void *(*)(void *))help_blur, args);
    //    pthread_mutex_unlock(&lock);
  }
  pthread_mutex_unlock(&lock);
  //  printf("waiting for threads to finish\n");
  for (int i = pre; i < num_threads; i++)
  {
    pthread_join(threads[i], NULL);
  }
  //  printf("done\n");
  // clean-up the old picture and replace with new picture
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
  pthread_mutex_destroy(&lock);
}
