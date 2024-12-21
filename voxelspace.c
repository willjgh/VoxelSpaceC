#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "dos/dos.h"

// constants
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCALE_FACTOR 150.0

// heightmap and colourmap arrays
uint8_t* heightmap = NULL;
uint8_t* colourmap = NULL;

// camera type defintion
typedef struct {
    float x; // x position on map
    float y; // y position on map
    float height; // height in space
    float angle; // yaw angle of camera
    float zfar; // far plane distance
} camera_t;

// initialize a camera
camera_t camera = {
    .x = 512,
    .y = 512,
    .height = 150.0,
    .angle = 0.0,
    .zfar = 400
};

// keyboard input
void processinput(void) {
    if (keystate(KEY_UP)) {
        // relative to camera angle
        camera.x -= sin(camera.angle);
        camera.y -= cos(camera.angle);
    }
    if (keystate(KEY_DOWN)) {
        // relative to camera angle
        camera.x += sin(camera.angle);
        camera.y += cos(camera.angle);
    }
    if (keystate(KEY_LEFT)) {
        camera.angle+= 0.02;
    }
    if (keystate(KEY_RIGHT)) {
        camera.angle-= 0.02;
    }
    if (keystate(KEY_SHIFT)) {
        camera.height--;
    }
    if (keystate(KEY_SPACE)) {
        camera.height++;
    }
}

// main function
int main(void) {

    // dos setup
    setvideomode(videomode_320x200);

    // colour palette
    uint8_t palette[256 * 3];
    int map_width, map_height, pal_count;

    // load maps
    colourmap = loadgif("maps/colourmap.gif", &map_width, &map_height, &pal_count, palette);
    heightmap = loadgif("maps/heightmap.gif", &map_width, &map_height, NULL, NULL); // don't use heightmap for colour information

    // set dos palette
    for (int i = 0; i < pal_count; i++) {
        setpal(i, palette[3 * i + 0], palette[3 * i + 1], palette[3 * i + 2]);
    }
    setpal(0, 36, 36, 56);

    // double buffers: draw and view
    setdoublebuffer(1);
    uint8_t* framebuffer = screenbuffer();

    // drawing loop
    while (!shuttingdown()) {
        waitvbl();
        clearscreen();

        // keyboard input
        processinput();

        // yaw angle
        float sinangle = sin(camera.angle);
        float cosangle = cos(camera.angle);

        // far plane extent points (relative to camera + rotation)
        float plx = (-cosangle - sinangle) * camera.zfar; // -camera.zfar;
        float ply = ( sinangle - cosangle) * camera.zfar; // camera.zfar;
        float prx = ( cosangle - sinangle) * camera.zfar; // camera.zfar;
        float pry = (-sinangle - cosangle) * camera.zfar; // camera.zfar;

        // loop over screen columns
        for (int i = 0; i < SCREEN_WIDTH; i++) {

            // ray direction increments (compute using relative far plane)
            float dx = (plx + (((prx - plx) / SCREEN_WIDTH) * i)) / camera.zfar;
            float dy = (ply + (((pry - ply) / SCREEN_WIDTH) * i)) / camera.zfar;
            
            // initial ray position (camera position)
            float rx = camera.x;
            float ry = camera.y;

            // maximum height drawn so far (start at bottom of screen, y decreases with height)
            float max_height = SCREEN_HEIGHT;

            // march ray
            for (int z = 1; z < camera.zfar; z++) {
                
                // increment ray
                rx += dx;
                ry += dy;

                // find position on map (module map size)
                int mapoffset = ((1024 * ((int)(ry) & 1023)) + ((int)(rx) & 1023));

                // get height on screen: heightmap value + perspective correction + relative to camera
                int heightonscreen = (int)(((camera.height - heightmap[mapoffset]) / z) * SCALE_FACTOR);

                // clamp to display limits
                if (heightonscreen < 0) {
                    heightonscreen = 0;
                }
                if (heightonscreen > SCREEN_HEIGHT - 1) {
                    heightonscreen = SCREEN_HEIGHT - 1;
                }

                // higher than current max height drawn (inverted y)
                if (heightonscreen < max_height) {

                    // draw from new height down to prev max
                    for (int y = heightonscreen; y < max_height; y++) {

                        // get colour info
                        framebuffer[(SCREEN_WIDTH * y) + i] = (uint8_t)colourmap[mapoffset];
                    }

                    // update max height
                    max_height = heightonscreen;
                }
            }
        }

        // swap draw and view buffers
        framebuffer = swapbuffers();

        // ESC exit
        if (keystate(KEY_ESCAPE)) {
            break;
        }
    }
    return EXIT_SUCCESS;
}