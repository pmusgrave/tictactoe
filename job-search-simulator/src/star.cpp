#include "star.hpp"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_image.h>

#include "globals.hpp"

Star::Star()
: rotation(0.0f),
  rotation_rate(1.0f),
  frame(0),
  frame_index(0),
  frame_rate(rand()%10+10),
  position(Vec2(rand()%resolution.x,rand()%resolution.y))
{
  img[0] = al_load_bitmap("resources/star-frame-0.png");
  img[1] = al_load_bitmap("resources/star-frame-1.png");
  img[2] = al_load_bitmap("resources/star-frame-2.png");
  rotation = rand()%360;
}

Star::~Star() {}

void Star::draw() {
  ALLEGRO_TRANSFORM trans;
  al_identity_transform(&trans);
  al_translate_transform(&trans, -position.x, -position.y);
  al_rotate_transform(&trans, rotation);
  al_translate_transform(&trans, position.x, position.y);
  al_use_transform(&trans);
  al_draw_scaled_bitmap(img[frame_index],
                        0, 0,  // source origin
                        al_get_bitmap_width(img[frame_index]),  // source width
                        al_get_bitmap_height(img[frame_index]),  // source height
                        position.x, position.y,  // target origin
                        resolution.x*0.02, resolution.y*0.02,  // target dimensions
                        0  // flags
                        );
  al_identity_transform(&trans);
  al_use_transform(&trans);
}

void Star::update() {
  if (++frame >= frame_rate) {
    frame_index = (++frame_index)%frame_max;
    frame = 0;
  }
  // rotation += rotation_rate;
}
