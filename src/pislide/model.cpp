
#include "model.h"

std::ostream &operator<<(std::ostream &os, Photo const &photo) {
    os << photo.id << ", " << photo.pathname;
    return os;
}
