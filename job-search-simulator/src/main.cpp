#include <allegro5/allegro5.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>

#include <time.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "black_hole.hpp"
#include "entity.hpp"
#include "globals.hpp"
#include "interviewer.hpp"
#include "manager.hpp"
#include "resume.hpp"

// struct screen_resolution resolution { 2560, 1440 };
// struct screen_resolution resolution { 1920, 1080 };
struct screen_resolution resolution;
int line_height;
const double c_squared = 8.98755179e16;

enum GameState {
  intro_screen = 0,
  normal,
  interlude,
  interlude_fail,
  interlude_win,
  failure,
};

struct GameContext {
 public:
  GameContext(GameState state) : state(state) {}
  void reset_time() {
    time_remaining = tick_rate * 3000;
    reference_time_remaining = tick_rate * 3000;
  }
  GameState state;
  static constexpr float tick_rate = 1.0f / 30.0f;
  float time_remaining;
  float reference_time_remaining;
};

enum Powerup {
  rocket_boost = 0,
  gravity_reduction,
  gravity_increase,
  init_speed_reduction,
  init_speed_increase,
  max_speed_reduction,
  max_speed_increase,
};
std::vector<Powerup> powerups {
  Powerup::rocket_boost,
  Powerup::gravity_reduction,
  Powerup::gravity_increase,
  Powerup::rocket_boost,
  Powerup::max_speed_reduction,
  Powerup::max_speed_increase,
  init_speed_reduction,
  init_speed_increase,
};

void must_init(bool test, const char *description) {
  if(test) return;

  printf("couldn't initialize %s\n", description);
  exit(1);
}

template <typename T>
void clear_entities(typename std::vector<T*>& entities_to_clear) {
  typename std::vector<T*>::iterator itr;
  for (itr = entities_to_clear.begin(); itr < entities_to_clear.end(); itr++) {
    delete (*itr);
  }
  entities_to_clear.clear();
}

void init_level(
  std::vector<Entity*> &entities,
  Resume* resume,
  Manager* manager)
{
  clear_entities<Entity>(entities);
  entities.push_back(resume);
  entities.push_back(manager);
}

int main(int argc, char **argv) {
  must_init(al_init(), "allegro");
  must_init(al_install_keyboard(), "keyboard");

  ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);
  must_init(timer, "timer");

  ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
  must_init(queue, "queue");

  al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
  al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
  al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
  al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
  ALLEGRO_MONITOR_INFO info;
  al_get_monitor_info(0, &info);
  int w = info.x2 - info.x1;
  int h = info.y2 - info.y1;
  resolution.x = w;
  resolution.y = h;
  ALLEGRO_DISPLAY* disp = al_create_display(resolution.x, resolution.y);
  must_init(disp, "display");

  must_init(al_init_font_addon(), "init font");
  must_init(al_init_ttf_addon(), "ttf");
  ALLEGRO_FONT* font = al_load_font("resources/Comfortaa/static/Comfortaa-Regular.ttf", 16, 0);
  must_init(font, "load font");
  line_height = al_get_font_line_height(font);

  must_init(al_init_primitives_addon(), "primitives");
  must_init(al_init_image_addon(), "image addon");

  // AUDIO INITIALIZATION
  must_init(al_install_audio(), "install audio addon");
  must_init(al_init_acodec_addon(), "audio codec addon");
  must_init(al_reserve_samples(10), "reserve audio samples");
  ALLEGRO_SAMPLE* intro_music = al_load_sample("resources/soundtrack/intro.ogg");
  ALLEGRO_SAMPLE* level_music = al_load_sample("resources/soundtrack/level.ogg");
  if(intro_music == nullptr || level_music == nullptr) {
    std::cout << "error loading music file" << std::endl;
  }
  al_stop_samples();

  al_register_event_source(queue, al_get_keyboard_event_source());
  al_register_event_source(queue, al_get_display_event_source(disp));
  al_register_event_source(queue, al_get_timer_event_source(timer));

  std::vector<BlackHole*> black_holes;
  std::vector<Bullet*> bullets;
  std::vector<Entity*> entities;
  std::vector<Interviewer*> interviewers;
  srand(time(NULL));
  // srand(0);
  int current_level = 1;
  BlackHole::G = 6.67430e-11;
  for (int i = 0; i < current_level; i++) {
    BlackHole* bh = new BlackHole();
    black_holes.push_back(bh);
  }

  Manager manager(resolution.x - resolution.x*0.0508, resolution.y / 2);
  entities.push_back(&manager);

  Resume resume(0, resolution.y / 2, &black_holes, &manager);
  entities.push_back(&resume);

  //  bool show_intro_screen = true;
  GameState state = intro_screen;
  // GameState state = GameState::interlude;
  bool done = false;
  bool redraw = true;
  ALLEGRO_EVENT event;
  std::string level_string = "Level ";
  level_string += std::to_string(current_level);
  GameContext context(state);
  context.reset_time();

#define KEY_SEEN     1
#define KEY_RELEASED 2

  unsigned char key[ALLEGRO_KEY_MAX];
  memset(key, 0, sizeof(key));

  al_start_timer(timer);
  while(1) {
    al_wait_for_event(queue, &event);

    switch(event.type) {
    case ALLEGRO_EVENT_TIMER:
      if(key[ALLEGRO_KEY_W])
        resume.handle_w();
      if(key[ALLEGRO_KEY_S])
        resume.handle_s();
      if(key[ALLEGRO_KEY_A])
        resume.handle_a();
      if(key[ALLEGRO_KEY_D])
        resume.handle_d();
      if(key[ALLEGRO_KEY_R])
        resume.reset();
      if(key[ALLEGRO_KEY_SPACE])
        resume.handle_space();

      if(key[ALLEGRO_KEY_ESCAPE])
        done = true;

      for(int i = 0; i < ALLEGRO_KEY_MAX; i++)
        key[i] &= KEY_SEEN;

      redraw = true;
      break;

    case ALLEGRO_EVENT_KEY_DOWN:
      key[event.keyboard.keycode] = KEY_SEEN | KEY_RELEASED;
      break;
    case ALLEGRO_EVENT_KEY_UP:
      key[event.keyboard.keycode] &= KEY_RELEASED;
      break;

    case ALLEGRO_EVENT_DISPLAY_CLOSE:
      done = true;
      break;
    }

    if(done)
      break;

    if (context.state == GameState::intro_screen) {
      al_stop_samples();
      al_play_sample(intro_music, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);

      al_clear_to_color(al_map_rgb(0, 0, 0));
      al_draw_text(font,
                   al_map_rgb(255, 255, 255),
                   resolution.x/2,
                   resolution.y/2,
                   ALLEGRO_ALIGN_CENTRE,
                   "Get a job");
      al_flip_display();
      std::chrono::milliseconds timespan(3000);
      std::this_thread::sleep_for(timespan);
      context.state = GameState::normal;
    }

    if (context.state == GameState::interlude && redraw && al_is_event_queue_empty(queue)) {
      al_clear_to_color(al_map_rgb(0, 0, 0));
      // ***********************************************************************
      // source: https://stackoverflow.com/questions/4713131
      auto b_itr = bullets.begin();
      while(b_itr != bullets.end()) {
        if((*b_itr)->marked_for_removal) {
          delete (*b_itr);
          b_itr = bullets.erase(b_itr);
        }
        else {
          if ((*b_itr)->x > resume.x && (*b_itr)->x < resume.x + resume.width
              && (*b_itr)->y > resume.y && (*b_itr)->y < resume.y + resume.height) {
            resume.fail = true;
            break;
          }
          b_itr++;
        }
      }
      // ***********************************************************************
      if (resume.fail) {
        context.state = GameState::interlude_fail;
        resume.interlude = false;
        resume.win = true;
        resume.fail = false;
      }
      else if (!resume.fail && context.time_remaining <= 0) {
        context.state = GameState::interlude_win;
        resume.interlude = false;
        resume.win = true;
      }

      std::vector<Entity*>::iterator itr = entities.begin();
      for (itr = entities.begin(); itr < entities.end(); itr++) {
        (*itr)->update();
        (*itr)->draw();
      }

      std::vector<Interviewer*>::iterator int_itr;
      for (int_itr = interviewers.begin(); int_itr < interviewers.end(); int_itr++) {
        (*int_itr)->update();
        (*int_itr)->draw();
      }

      for (b_itr = bullets.begin(); b_itr < bullets.end(); b_itr++) {
        (*b_itr)->update();
        (*b_itr)->draw();
      }
      al_draw_text(font,
                   al_map_rgb(255, 255, 255),
                   resolution.x/2,
                   line_height,
                   ALLEGRO_ALIGN_CENTRE,
                   "INTERVIEW");
      int text_width = al_get_text_width(font, "INTERVIEW");
      std::stringstream ss;
      ss << "Time remaining: " << std::setprecision(4) << context.time_remaining;
      std::string time_str = ss.str();
      al_draw_text(font,
                  al_map_rgb(255, 255, 255),
                  resolution.x/2 - text_width/2,
                  line_height*2,
                  ALLEGRO_ALIGN_LEFT,
                  time_str.c_str());
      context.time_remaining -= context.tick_rate; // has problems with tick rate consistency
      al_flip_display();
      redraw = false;
    }

    if (context.state == GameState::normal && redraw && al_is_event_queue_empty(queue)) {
      al_clear_to_color(al_map_rgb(0, 0, 0));

      double time_dilation_factor = 1;

      std::vector<Entity*>::iterator itr;
      for (itr = entities.begin(); itr < entities.end(); itr++) {
        (*itr)->update();
        (*itr)->draw();
      }

      std::vector<BlackHole*>::iterator bh_itr;
      for (bh_itr = black_holes.begin(); bh_itr < black_holes.end(); bh_itr++) {
        (*bh_itr)->update();
        (*bh_itr)->draw();
        double dx = (*bh_itr)->x - resume.x;
        double dy = (*bh_itr)->y - resume.y;
        double r = sqrt(pow(dx,2) + pow(dy,2));
        time_dilation_factor += (1 - (1 / sqrt(1 - (2*BlackHole::G*(*bh_itr)->m*(2e14/current_level)/(((*bh_itr)->radius + r)*c_squared)))));
      }

      time_dilation_factor += (1 - (1 / sqrt(1 - resume.get_scalar_velocity_squared()/c_squared)));
      context.time_remaining -= context.tick_rate / time_dilation_factor; // has problems with tick rate consistency
      context.reference_time_remaining -= context.tick_rate; // has problems with tick rate consistency

      if (context.time_remaining <= 0 || context.reference_time_remaining <= 0) {
        context.state = GameState::failure;
      }

      al_draw_text(font,
                   al_map_rgb(255, 255, 255),
                   resolution.x/2,
                   line_height,
                   ALLEGRO_ALIGN_CENTRE,
                   level_string.c_str());

      int text_width = al_get_text_width(font,level_string.c_str());
      std::stringstream ss;
      ss << "Time remaining: " << std::setprecision(4) << context.time_remaining;
      std::string time_str = ss.str();
      al_draw_text(font,
                  al_map_rgb(255, 255, 255),
                  resolution.x/2 - text_width/2,
                  line_height*2,
                  ALLEGRO_ALIGN_LEFT,
                  time_str.c_str());
      ss.str("");
      ss << "Reference time: " << std::setprecision(4) << context.reference_time_remaining;
      time_str = ss.str();
      al_draw_text(font,
                  al_map_rgb(255, 255, 255),
                  resolution.x/2 - text_width/2,
                  line_height*3,
                  ALLEGRO_ALIGN_LEFT,
                  time_str.c_str());

      al_flip_display();
      redraw = false;
    }

    if (resume.win) {
      context.reset_time();

      if ((current_level%5) == 0 && current_level != 35) {
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_draw_text(font,
                     al_map_rgb(255, 255, 255),
                     resolution.x/2,
                     resolution.y/2,
                     ALLEGRO_ALIGN_CENTRE,
                     "\"Hi! We'd like to bring you in for an interview to tell you about");
        al_draw_text(font,
                    al_map_rgb(255, 255, 255),
                    resolution.x/2,
                    resolution.y/2 + line_height,
                    ALLEGRO_ALIGN_CENTRE,
                    "the company and learn a little more about your background.\"");
        al_flip_display();
        std::chrono::milliseconds timespan(6000);
        std::this_thread::sleep_for(timespan);

        context.state = GameState::interlude;
        context.time_remaining = 30;
        resume.interlude = true;
        resume.reset();
        entities.clear();
        entities.push_back(&resume);
        for (int i = 0; i < current_level / 5; i++) {
          std::random_device rd;
          std::mt19937 gen(rd());
          std::uniform_int_distribution<int> x_pos_distr(200, resolution.x - 200);
          std::normal_distribution<float> y_pos_distr(resolution.y * 0.2, 20);
          std::normal_distribution<float> fire_rate(current_level%5+4, 2);
          int x = ((unsigned int)x_pos_distr(gen))%(resolution.x);
          int y = ((unsigned int)y_pos_distr(gen))%(resolution.y);// + (radius/2);
          interviewers.push_back(std::move(new Interviewer(x, y, abs(fire_rate(gen)), &bullets)));
        }
      }

      if (current_level%35 == 0) {
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_draw_text(font,
                     al_map_rgb(255, 255, 255),
                     resolution.x/2,
                     resolution.y/2,
                     ALLEGRO_ALIGN_CENTRE,
                     "\"Thanks so much for your interest in this company! You made it to the final round of our candidate selection process.\"");
        al_draw_text(font,
                     al_map_rgb(255, 255, 255),
                     resolution.x/2,
                     resolution.y/2 + line_height,
                     ALLEGRO_ALIGN_CENTRE,
                     "\"Unfortunately, we decided to go with another candidate.\"");
        al_draw_text(font,
                    al_map_rgb(255, 255, 255),
                    resolution.x/2,
                    resolution.y/2 + line_height*2,
                    ALLEGRO_ALIGN_CENTRE,
                    "Congratulations on making it this far.");
        al_draw_text(font,
                    al_map_rgb(255, 255, 255),
                    resolution.x/2,
                    resolution.y/2 + line_height*3,
                    ALLEGRO_ALIGN_CENTRE,
                    "The job hunt continues...");
        al_flip_display();
        std::chrono::milliseconds timespan(6000);
        std::this_thread::sleep_for(timespan);
        current_level = 1;
        context.state = GameState::normal;
        level_string = "Level ";
        level_string += std::to_string(current_level);
        clear_entities<Bullet>(bullets);
        clear_entities<Interviewer>(interviewers);
        clear_entities<BlackHole>(black_holes);
        entities.clear();
        entities.push_back(&manager);
        entities.push_back(&resume);
        resume.reset();
        for (int i = 0; i < current_level; i++) {
          black_holes.push_back(new BlackHole());
        }
        resume.interlude = false;
        continue;
      }

      if (context.state == GameState::interlude_win) {
        context.state = GameState::normal;
        // init_level(entities, &resume, &manager);
        clear_entities<Bullet>(bullets);
        clear_entities<Interviewer>(interviewers);
        entities.clear();
        entities.push_back(&manager);
        entities.push_back(&resume);
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_draw_text(font,
                     al_map_rgb(255, 255, 255),
                     resolution.x/2,
                     resolution.y/2,
                     ALLEGRO_ALIGN_CENTRE,
                     "\"Thanks so much for your interest in this company! Unfortunately, we will not be moving forward at this time.\"");
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<unsigned int> powerup_dist(0, powerups.size());
        unsigned int new_powerup = powerup_dist(gen) % powerups.size();
        //unsigned int new_powerup = powerups[rand()%(powerups.size())];
        switch (new_powerup) {
        case Powerup::rocket_boost:
          resume.rocket_boost_enable();
          al_draw_text(font,
                       al_map_rgb(255, 255, 255),
                       resolution.x/2,
                       resolution.y/2 + line_height,
                       ALLEGRO_ALIGN_CENTRE,
                       "Your persistence has rewarded you with a new ability. You gain ROCKET BOOST.");
          break;
        case Powerup::gravity_reduction:
          BlackHole::G *= 0.90;
          al_draw_text(font,
                       al_map_rgb(255, 255, 255),
                       resolution.x/2,
                       resolution.y/2 + line_height,
                       ALLEGRO_ALIGN_CENTRE,
                       "Your persistence has rewarded you with a new ability. You gain GRAVITY REDUCTION.");
          al_draw_text(font,
                      al_map_rgb(255, 255, 255),
                      resolution.x/2,
                      resolution.y/2 + line_height*2,
                      ALLEGRO_ALIGN_CENTRE,
                      "Gravity is now 10% less strong.");
          break;
        case Powerup::gravity_increase:
          resume.v_init *= 1.15;
          resume.v_max *= 1.10;
          BlackHole::G *= 1.05;
          al_draw_text(font,
                      al_map_rgb(255, 255, 255),
                      resolution.x/2,
                      resolution.y/2 + line_height*2,
                      ALLEGRO_ALIGN_CENTRE,
                      "Your velocity has increased, but gravity is now stronger.");
          break;
        case Powerup::max_speed_reduction:
          resume.v_max *= 0.95;
          resume.v_init *= 1.10;
          al_draw_text(font,
                      al_map_rgb(255, 255, 255),
                      resolution.x/2,
                      resolution.y/2 + line_height*2,
                      ALLEGRO_ALIGN_CENTRE,
                      "Your launch velocity has increased, but your max velocity has decreased.");
          break;
        case Powerup::max_speed_increase:
          resume.v_max *= 1.15;
          al_draw_text(font,
                      al_map_rgb(255, 255, 255),
                      resolution.x/2,
                      resolution.y/2 + line_height*2,
                      ALLEGRO_ALIGN_CENTRE,
                      "Your max velocity has increased.");
          break;
        case Powerup::init_speed_reduction:
          resume.v_init *= 0.90;
          resume.v_max *= 1.05;
          al_draw_text(font,
                      al_map_rgb(255, 255, 255),
                      resolution.x/2,
                      resolution.y/2 + line_height*2,
                      ALLEGRO_ALIGN_CENTRE,
                      "Your max velocity has increased, but your launch velocity has decreased.");
          break;
        case Powerup::init_speed_increase:
          resume.v_init *= 1.15;
          al_draw_text(font,
                      al_map_rgb(255, 255, 255),
                      resolution.x/2,
                      resolution.y/2 + line_height*2,
                      ALLEGRO_ALIGN_CENTRE,
                      "Your launch velocity has increased.");
          break;
        }

        current_level--;
        al_flip_display();
        std::chrono::milliseconds timespan(6000);
        std::this_thread::sleep_for(timespan);

        al_stop_samples();
        al_play_sample(level_music, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
      }

      if (context.state == GameState::interlude_fail) {
        context.state = GameState::normal;
        // std::vector<Bullet*>::iterator itr = bullets.begin();
        // for (itr = bullets.begin(); itr < bullets.end(); itr++) {
        //   delete (*itr);
        // }
        // bullets.clear();
        // init_level(entities, &resume, &manager);
        clear_entities<Bullet>(bullets);
        clear_entities<Interviewer>(interviewers);
        entities.clear();
        entities.push_back(&manager);
        entities.push_back(&resume);
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_draw_text(font,
                     al_map_rgb(255, 255, 255),
                     resolution.x/2,
                     resolution.y/2,
                     ALLEGRO_ALIGN_CENTRE,
                     "\"Thanks so much for your interest in this company! Unfortunately, we will not be moving forward at this time.\"");
        al_draw_text(font,
                    al_map_rgb(255, 255, 255),
                    resolution.x/2,
                    resolution.y/2 + line_height,
                    ALLEGRO_ALIGN_CENTRE,
                    "You do not gain any new abilities.");

        current_level--;
        al_flip_display();
        std::chrono::milliseconds timespan(6000);
        std::this_thread::sleep_for(timespan);
        al_stop_samples();
        al_play_sample(level_music, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
      }

      current_level++;
      level_string = "Level ";
      level_string += std::to_string(current_level);
      manager.respawn();
      clear_entities<BlackHole>(black_holes);
      for (int i = 0; i < current_level; i++) {
        black_holes.push_back(new BlackHole());
      }
      resume.reset();
    }

    if (context.state == GameState::failure) {
      al_clear_to_color(al_map_rgb(0, 0, 0));
      current_level = 1;
      level_string = "Level ";
      level_string += std::to_string(current_level);
      clear_entities<BlackHole>(black_holes);
      entities.clear();
      entities.push_back(&manager);
      entities.push_back(&resume);
      black_holes.push_back(new BlackHole());
      resume.reset();
      context.state = GameState::intro_screen;
      context.reset_time();
      al_flip_display();
    }
  }

  al_destroy_font(font);
  al_destroy_display(disp);
  al_destroy_timer(timer);
  al_destroy_event_queue(queue);
  al_stop_samples();
  al_destroy_sample(intro_music);
  al_destroy_sample(level_music);

  clear_entities<BlackHole>(black_holes);

  return 0;
}
