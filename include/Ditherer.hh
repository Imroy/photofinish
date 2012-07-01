#ifndef __DITHERER_HH__
#define __DITHERER_HH__

namespace PhotoFinish {

  class Ditherer {
  private:
    long int width;
    unsigned char channels;
    short int **error_rows;
    int curr_row, next_row;

  public:
    Ditherer(long int w, unsigned char c);
    ~Ditherer();

    void dither(short unsigned int *inrow, unsigned char *outrow);
  };

}

#endif /* __DITHERER_HH__ */
