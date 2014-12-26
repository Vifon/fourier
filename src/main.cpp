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


const int SCREEN_W = 1280;
const int SCREEN_H =  700;
ALLEGRO_COLOR FUNCTION_COLOR;


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
    for (int x = begin; x < steps; ++x) {
        combined_values[i++] = fun(x*frequency_multiplier * (range / steps));
    }

    return std::move(combined_values);
}

void redraw_functions(const std::vector<unsigned int>& frequencies,
                      const size_t selected,
                      const std::function<double(double)> fun)
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
    unsigned int x = 0;
    // Draw the combined function.
    for (auto y : combined_values) {
        // Function negated because the coordinates are inverted.
        y = scale_function(-y, FUNCTION_HEIGHT, combined_min, combined_max);

        al_put_pixel(x + XMARGIN, y + YMARGIN, FUNCTION_COLOR);
        ++x;
    }

    // Draw the individual functions.
    for (int i = 0; i < frequencies.size(); ++i) {
        std::vector<double> function_values =
            std::move(calculate_function(fun,
                                         0, 2*M_PI,
                                         SUBFUNCTION_WIDTH,
                                         frequencies[i]));
        size_t x = 0;
        for (auto y : function_values) {
            y = scale_function(-y, SUBFUNCTION_HEIGHT, -1, 1) + SUBFUNCTION_YMARGIN;

            al_put_pixel(x + XMARGIN + i*(SUBFUNCTION_WIDTH + SUBFUNCTION_XMARGIN),
                         y,
                         FUNCTION_COLOR);
            ++x;
        }
    }
}

void redraw_borders(const size_t function_count, const size_t selected)
{
    // Combined function's border.
    al_draw_rectangle(XMARGIN,
                      YMARGIN,
                      XMARGIN + FUNCTION_WIDTH + 2,
                      YMARGIN + FUNCTION_HEIGHT + 2,
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
                          + i*(SUBFUNCTION_WIDTH + SUBFUNCTION_XMARGIN),

                          SUBFUNCTION_YMARGIN,

                          XMARGIN
                          + i*(SUBFUNCTION_WIDTH + SUBFUNCTION_XMARGIN)
                          + SUBFUNCTION_WIDTH + 2,

                          SUBFUNCTION_YMARGIN + SUBFUNCTION_HEIGHT + 2,
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
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_display_event_source(display));

    std::vector<unsigned int> frequencies = {1,0,0,0};
    size_t selected = 0;

    while (doexit == false) {
        // Redrawing
        switch (doredraw) {
        case REDRAW_ALL:
            al_clear_to_color(background_color);
            redraw_functions(frequencies, selected, sin);
        case REDRAW_BORDERS:
            redraw_borders(frequencies.size(), selected);
            al_flip_display();
            doredraw = REDRAW_NONE;
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

            if (key >= ALLEGRO_KEY_0
                && key <= ALLEGRO_KEY_9) {

                doredraw = REDRAW_ALL;
                frequencies[selected] = key - ALLEGRO_KEY_0;
            } else if (key == ALLEGRO_KEY_LEFT) {
                if (selected > 0) {
                    doredraw = REDRAW_BORDERS;
                    --selected;
                }
            } else if (key == ALLEGRO_KEY_RIGHT) {
                if (selected < frequencies.size()-1) {
                    doredraw = REDRAW_BORDERS;
                    ++selected;
                }
            } else if (key == ALLEGRO_KEY_Q) {
                doexit = true;
            }
        }
    }

    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    al_shutdown_primitives_addon();

    return 0;
}
