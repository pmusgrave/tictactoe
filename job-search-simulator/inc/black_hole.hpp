#ifndef INC_BLACK_HOLE_HPP_
#define INC_BLACK_HOLE_HPP_

#include "entity.hpp"

class BlackHole final : public Entity {
public:
  BlackHole();
  ~BlackHole() {}
  void draw() override;
  void update() override;
  int x;
  int y;
  int radius;
  long m;
  static constexpr double G = 6.67430e-11;
};

#endif  // INC_BLACK_HOLE_HPP
