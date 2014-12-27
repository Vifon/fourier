/////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2014 Wojciech Siewierski <wojciech dot siewierski at onet dot pl> //
//                                                                                 //
// Author: Wojciech Siewierski <wojciech dot siewierski at onet dot pl>            //
//                                                                                 //
// This program is free software; you can redistribute it and/or                   //
// modify it under the terms of the GNU General Public License                     //
// as published by the Free Software Foundation; either version 3                  //
// of the License, or (at your option) any later version.                          //
//                                                                                 //
// This program is distributed in the hope that it will be useful,                 //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                  //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   //
// GNU General Public License for more details.                                    //
//                                                                                 //
// You should have received a copy of the GNU General Public License               //
// along with this program. If not, see <http://www.gnu.org/licenses/>.            //
/////////////////////////////////////////////////////////////////////////////////////

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <algorithm>
#include <functional>
#include <vector>

#include <cmath>
#include <cstdlib>
#include <ctime>


const int SCREEN_W = 1280;
const int SCREEN_H =  700;

ALLEGRO_COLOR FUNCTION_COLOR;
ALLEGRO_COLOR SUBFUNCTION_COLOR;
ALLEGRO_COLOR RANDOM_FUNCTION_COLOR;


const int XMARGIN = 60;
const int YMARGIN = 60;
const int FUNCTION_WIDTH = 1170;
const int FUNCTION_HEIGHT = 360;

const int SUBFUNCTION_XMARGIN = 30;
const int SUBFUNCTION_YMARGIN = YMARGIN + FUNCTION_HEIGHT + 30;
const int SUBFUNCTION_WIDTH = 270;
const int SUBFUNCTION_HEIGHT = 180;

// Scale function to the given height.
double scale_function(double y, int height, double min_value, double max_value)
{
    y -= min_value;               // Treat minimum as 0.
    max_value -= min_value;       // Adjust the maximum accordingly.
    y = y * (height / max_value); // Scale the function.
    return y;
}

// Calculate the function values for x between begin and end with the
// specified quantity of steps in-between.
std::vector<double> calculate_function(const std::function<double(double)> fun,
                                       const double begin,
                                       const double end,
                                       const size_t steps,
                                       const double frequency_multiplier)
{
    std::vector<double> combined_values(steps);

    double range = end - begin;

    int i = 0;
    for (size_t x = begin; x < steps; ++x) {
        combined_values[i++] = fun(x*frequency_multiplier * (range / steps));
    }

    return std::move(combined_values);
}


// Workaround for the Allegro limitations.
//
// Note: tested only with `points_stride == 2*sizeof(point)` i.e. just
// the points array, without additional data within.
void draw_fuckin_long_ribbon(const float* points, int points_stride,
                             ALLEGRO_COLOR color, float thickness,
                             int num_segments)
{
    while (num_segments*2 >= ALLEGRO_VERTEX_CACHE_SIZE) {
        al_draw_ribbon(points, points_stride, color, thickness, ALLEGRO_VERTEX_CACHE_SIZE/2);
        points += ALLEGRO_VERTEX_CACHE_SIZE - 2;
        num_segments -= ALLEGRO_VERTEX_CACHE_SIZE/2 - 1;
    }
    al_draw_ribbon(points, points_stride, color, thickness, num_segments);
}

std::vector<float> values_to_points(const std::vector<double>& values,
                                    std::function<float(double)> x_transform,
                                    std::function<float(double)> y_transform)
{
    std::vector<float> points;
    points.reserve(values.size()*2);

    unsigned int x = 0;

    for (auto y : values) {
        points.push_back(x_transform(x));
        points.push_back(y_transform(y));

        ++x;
    }

    return points;
}

void redraw_combined_function(const std::vector<unsigned int>& frequencies,
                              const std::function<double(double)> fun,
                              const ALLEGRO_COLOR color,
                              const float thickness)
{
    // Vector of vectors with functions' values.
    std::vector<std::vector<double>> functions(frequencies.size());
    for (size_t i = 0; i < functions.size(); ++i) {
        functions[i] = std::move(
            calculate_function(fun,
                               0, 2*M_PI,
                               FUNCTION_WIDTH,
                               frequencies[i]));
    }
    // Values of functions from the previous vectors added together
    // (like the zip higher order function).
    std::vector<double> combined_values(FUNCTION_WIDTH, 0);
    for (auto& function_values : functions) {
        for (size_t i = 0; i < function_values.size(); ++i) {
            combined_values[i] += function_values[i];
        }
    }

    double combined_max = *std::max_element(combined_values.begin(),
                                            combined_values.end());
    double combined_min = *std::min_element(combined_values.begin(),
                                            combined_values.end());

    // Draw the combined function.
    draw_fuckin_long_ribbon(
        std::move(values_to_points(
                      combined_values,
                      [=](double x){
                          return x + XMARGIN;
                      },
                      [=](double y){
                          // Value with a minus because the coordinates are inverted.
                          return scale_function(-y, FUNCTION_HEIGHT,
                                                combined_min, combined_max) + YMARGIN;
                      }))
        .data(),
        sizeof(float)*2, color, thickness, combined_values.size());
}

void redraw_subfunctions(const std::vector<unsigned int>& frequencies,
                         const std::function<double(double)> fun,
                         const ALLEGRO_COLOR color,
                         const float thickness)
{
    // Draw the individual functions.
    for (size_t i = 0; i < frequencies.size(); ++i) {
        std::vector<double> function_values =
            std::move(calculate_function(fun,
                                         0, 2*M_PI,
                                         SUBFUNCTION_WIDTH,
                                         frequencies[i]));

        draw_fuckin_long_ribbon(
            std::move(values_to_points(
                          function_values,
                          [=](double x){
                              return x + XMARGIN + i*(SUBFUNCTION_WIDTH + SUBFUNCTION_XMARGIN);
                          },
                          [=](double y){
                              // Value with a minus because the coordinates are inverted.
                              return scale_function(-y, SUBFUNCTION_HEIGHT,
                                                    -1, 1) + SUBFUNCTION_YMARGIN;
                          }))
            .data(),
            sizeof(float)*2, color, thickness, function_values.size());
    }
}

void redraw_functions(const std::vector<unsigned int>& frequencies,
                      const std::vector<unsigned int>& random_frequencies,
                      const std::function<double(double)> fun)
{
    if (!random_frequencies.empty()) {
        redraw_combined_function(random_frequencies, fun, RANDOM_FUNCTION_COLOR, 2);
    }
    redraw_combined_function(frequencies, fun, FUNCTION_COLOR, 3);
    redraw_subfunctions(frequencies, fun, SUBFUNCTION_COLOR, 1);
}

void redraw_borders(const size_t function_count, const size_t selected)
{
    // Combined function's border.
    al_draw_rectangle(XMARGIN - 1,
                      YMARGIN - 1,
                      XMARGIN + FUNCTION_WIDTH + 1,
                      YMARGIN + FUNCTION_HEIGHT + 1,
                      al_map_rgb(0x00, 0x00, 0xAA), 1);

    // Individual functions' borders.
    for (size_t i = 0; i < function_count; ++i) {
        ALLEGRO_COLOR border_color;
        if (i == selected) {
            border_color = al_map_rgb(0x00, 0xAA, 0x00);
        } else {
            border_color = al_map_rgb(0x00, 0x00, 0xAA);
        }

        al_draw_rectangle(XMARGIN
                          + i*(SUBFUNCTION_WIDTH + SUBFUNCTION_XMARGIN) - 1,

                          SUBFUNCTION_YMARGIN - 1,

                          XMARGIN
                          + i*(SUBFUNCTION_WIDTH + SUBFUNCTION_XMARGIN)
                          + SUBFUNCTION_WIDTH + 1,

                          SUBFUNCTION_YMARGIN + SUBFUNCTION_HEIGHT + 1,

                          border_color, 1);
    }
}

enum redraw_t
{
    REDRAW_NONE = 0,
    REDRAW_BORDERS,
    REDRAW_ALL
};

int main(int argc, char *argv[])
{
    srand(time(0));

    ALLEGRO_DISPLAY* display;
    ALLEGRO_EVENT_QUEUE* event_queue;

    bool doexit = false;
    redraw_t doredraw = REDRAW_ALL;

    al_init();
    al_init_primitives_addon();
    al_install_keyboard();
    al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
    display = al_create_display(SCREEN_W, SCREEN_H);
    event_queue = al_create_event_queue();
    ALLEGRO_COLOR background_color = al_map_rgb(0x11, 0x11, 0x11);
    FUNCTION_COLOR = al_map_rgb(0xFF, 0x00, 0x00);
    SUBFUNCTION_COLOR = al_map_rgb(0xFF, 0x00, 0x00);
    RANDOM_FUNCTION_COLOR = al_map_rgb(0x00, 0xAA, 0xAA);
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_display_event_source(display));

    std::vector<unsigned int> frequencies = {1,0,0,0};
    size_t selected = 0;
    std::vector<unsigned int> random_frequencies;

    while (doexit == false) {
        // Redrawing
        switch (doredraw) {
        case REDRAW_ALL:
            al_clear_to_color(background_color);
            redraw_functions(frequencies, random_frequencies, sin);
        case REDRAW_BORDERS:
            redraw_borders(frequencies.size(), selected);
            al_flip_display();
            doredraw = REDRAW_NONE;
            break;
        case REDRAW_NONE:
            break;
        }

        ALLEGRO_EVENT ev;
        al_wait_for_event(event_queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            doexit = true;
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_EXPOSE ||
                   ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
            doredraw = REDRAW_ALL;
        } else if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
            int key = ev.keyboard.keycode;

            // set frequency
            if (key >= ALLEGRO_KEY_0
                && key <= ALLEGRO_KEY_9) {

                doredraw = REDRAW_ALL;
                frequencies[selected] = key - ALLEGRO_KEY_0;
            }
            // select the function to modify
            else if (key == ALLEGRO_KEY_LEFT) {
                if (selected > 0) {
                    doredraw = REDRAW_BORDERS;
                    --selected;
                }
            } else if (key == ALLEGRO_KEY_RIGHT) {
                if (selected < frequencies.size()-1) {
                    doredraw = REDRAW_BORDERS;
                    ++selected;
                }
            }
            else if (key == ALLEGRO_KEY_R) {
                doredraw = REDRAW_ALL;
                // disable the random function
                if (ev.keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) {
                    random_frequencies.clear();
                } else if (ev.keyboard.modifiers & ALLEGRO_KEYMOD_CTRL) {
                    frequencies = {1,0,0,0};
                }
                // generate a new random function
                else {
                    random_frequencies.clear();
                    for (int i = 0; i < frequencies.size(); ++i) {
                        random_frequencies.push_back(rand() % 9 + 1);
                    }
                }
            }
            // quit
            else if (key == ALLEGRO_KEY_Q) {
                doexit = true;
            }
        }
    }

    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    al_shutdown_primitives_addon();

    return 0;
}
