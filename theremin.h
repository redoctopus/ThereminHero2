/* Theremin Interface Code */

typedef struct {
  int pitch;
  double duration;
} *note;

int readFromTheremin();
