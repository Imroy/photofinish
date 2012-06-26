#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include "yaml-cpp/yaml.h"
#include "Image.hh"
#include "ImageFile.hh"
#include "Destination.hh"

int main(int argc, char* argv[]) {
  map<std::string, Destination*> destinations;
  {
    std::ifstream fin("destinations.yml");
    YAML::Parser parser(fin);
    YAML::Node doc;

    parser.GetNextDocument(doc);
    for (YAML::Iterator it = doc.begin(); it != doc.end(); it++) {
      std::string destname;
      it.first() >> destname;
      fprintf(stderr, "Destination \"%s\".\n", destname.c_str());
      Destination *destination = new Destination;
      it.second() >> *destination;
      destinations[destname] = destination;
    }
  }

  deque<std::string> arg_destinations;
  deque<std::string> arg_filenames;
  for (int i = 1; i < argc; i++) {
    struct stat s;
    if (destinations.count(argv[i]))
      arg_destinations.push_back(argv[i]);
    else if (stat(argv[i], &s) == 0)
      arg_filenames.push_back(argv[i]);
    else
      fprintf(stderr, "Argument \"%s\" is neither a destination name, nor a filename.\n", argv[i]);
  }

  for (deque<std::string>::iterator fi = arg_filenames.begin(); fi != arg_filenames.end(); fi++) {
    _ImageFile *infile = ImageFile(*fi);
    if (infile == NULL) {
      fprintf(stderr, "Could not determine the type of file \"%s\".\n", fi->c_str());
      continue;
    }

    Image *image = infile->read();
    if (image == NULL)
      continue;

    for (deque<std::string>::iterator di = arg_destinations.begin(); di != arg_destinations.end(); di++) {
      fprintf(stderr, "Destination: \"%s\".\n", di->c_str());

      Destination *destination = destinations[*di];
      Frame *frame = destination->best_frame(image);
      Image *outimage = frame->crop_resize(image, 3);

      JPEGFile outfile(*fi + "." + *di + ".jpeg");
      outfile.write(outimage, *destination);
      delete outimage;
    }
    delete image;
    delete infile;
  }

  for (map<string, Destination*>::iterator di = destinations.begin(); di != destinations.end(); di++)
    delete di->second;

  return 0;
}
