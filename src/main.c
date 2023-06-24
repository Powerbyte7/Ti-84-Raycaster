#include <sys/util.h>
#include <graphx.h>
#include <keypadc.h>
#include <math.h>


/* Include the sprite data */
#include "gfx/gfx.h"

float px,py,pdx,pdy,pa; // Player position

#define START_X ((GFX_LCD_WIDTH - oiram_width) / 2)
#define START_Y ((GFX_LCD_HEIGHT - oiram_height) / 2)

/* Create a buffer to store the background behind the sprite */
gfx_UninitedSprite(background, oiram_width, oiram_height);

void DrawSprite(int x, int y);

#define DEGREE 0.0174533
#define RENDER_DISTANCE 4

/* View raycasts in 2D instead of 3D */
//#define DEBUG_VIEW

#define MAP_X 8
#define MAP_Y 8
int map[] =
{
    1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,1,1,0,1,0,1,
    1,0,1,1,0,1,0,1,
    1,1,1,1,1,1,1,1
};

#ifdef DEBUG_VIEW
#include <ti/real.h>

char result[10];

void float2str(float value, char *str)
{
    real_t tmp_real = os_FloatToReal(value);
    os_RealToStr(str, &tmp_real, 8, 1, 2);
}
#endif

float dist(float ax, float ay, float bx, float by) {
    return sqrtf((bx-ax)*(bx-ax)+(by-ay)*(by-ay));
}

#include "table.h"

// Faster calculation of ntan with lookup table
// Uses expression f(x)=-tan(x)
// Usable in range 0 to 6.28
float ntanLookup(float value) {
    // Place in range 0 < x < 3.14
    if (value > M_PI) {
        value -= M_PI;
    }

    int index;

    // The function is symetric
    // When value is bigger than PI/2 it flips
    if (value > M_PI_2) {
        value = value - M_PI_2;
        index = (int) 162.3380419 * value;

        return tan_table[256 - index];
    }

    index = (int) 162.3380419 * value;

    return -tan_table[index];
}

// Faster calculation of atan with lookup table
// Uses expression f(x)=-1/tan(x)
// Usable in range 0 to 6.28
float atanLookup(float value) {
    // Place in range 0 < x < 3.14
    if (value > M_PI) {
        value -= M_PI;
    }

    int index;

    // The function is symetric
    // When value is bigger than PI/2 it flips
    if (value > M_PI_2) {
        value = value - M_PI_2;
        index = (int) 162.3380419 * value;

        return tan_table[index];
    }

    index = (int) 162.3380419 * value;

    return -tan_table[256 - index];
}



void raycast() {
    int ray, dof, map_index, map_x, map_y;
    float ray_x, ray_y, ray_angle, x_offset, y_offset, distance;
    float ra = pa-0.05*20;

    if (ra > 2*M_PI) {
        ra -= 2*M_PI;
    }          
    if (ra < 0) {
        ra += 2*M_PI;
    }

    for (int ray = 0; ray <20; ray++) {
        dof = 0;

        //float atan = -1/tanf(ra);
        float atan = atanLookup(ra);
        float distance_h = 1000000, hx=px,hy=py;
        if (ra > M_PI) {
            ray_y = (((int) py>>6)<<6)-0.0001;
            ray_x = (py-ray_y)*atan+px;
            y_offset = -64;
            x_offset = -y_offset*atan;
        } else if (ra < M_PI) {
            ray_y = (((int) py>>6)<<6)+64;
            ray_x = (py-ray_y)*atan+px;
            y_offset = 64;
            x_offset = -y_offset*atan;
        } else if (ra == 0 || ra == M_PI) {
            ray_x = px;
            ray_y = py;
            dof = RENDER_DISTANCE;
        }

        while (dof < RENDER_DISTANCE) {
            map_x = (int) (ray_x) >> 6;
            map_y = (int) (ray_y) >> 6;
            map_index = map_y * MAP_X + map_x;
            if (map_index > 0 && map_index < MAP_X * MAP_Y && map[map_index] == 1) {
                hx = ray_x;
                hy = ray_y;
                distance_h = dist(px,py,hx,hy);
                dof = RENDER_DISTANCE;
            } else {
                ray_x += x_offset;
                ray_y += y_offset;
                dof += 1;
            }
        }


        dof = 0;
        // float ntan = -tan(ra);
        float ntan = ntanLookup(ra);
        float distance_v = 1000000,vx=px,vy=py;
        if (ra > M_PI_2 && ra < M_PI_2*3) {
            ray_x = (((int) px>>6)<<6)-0.0001;
            ray_y = (px-ray_x)*ntan+py;
            x_offset = -64;
            y_offset = -x_offset*ntan;
        } else if (ra < M_PI_2 || ra > M_PI_2*3) {
            ray_x = (((int) px>>6)<<6)+64;
            ray_y = (px-ray_x)*ntan+py;
            x_offset = 64;
            y_offset = -x_offset*ntan;
        } else if (ra == 0 || ra == M_PI) {
            ray_y = py;
            ray_x = px;
            dof = RENDER_DISTANCE;
        }

        while (dof < RENDER_DISTANCE) {
            map_x = (int) (ray_x) >> 6;
            map_y = (int) (ray_y) >> 6;
            map_index = map_y * MAP_X + map_x;
            if (map_index > 0 && map_index < MAP_X * MAP_Y && map[map_index] == 1) {
                vx = ray_x;
                vy = ray_y;
                distance_v = dist(px,py,vx,vy);
                dof = RENDER_DISTANCE;
            } else {
                ray_x += x_offset;
                ray_y += y_offset;
                dof += 1;
            }
        }

        if (distance_v<distance_h) {
            ray_x=vx;
            ray_y=vy;
            distance = distance_v;

            gfx_SetColor(4);
        }
        if (distance_v>distance_h) {
            ray_x=hx;
            ray_y=hy;
            distance = distance_h;
            gfx_SetColor(10);
        }

        ra += 0.1;
        if (ra > 2*M_PI) {
        ra -= 2*M_PI;
        }          
        if (ra < 0) {
            ra += 2*M_PI;
        }

        #ifdef DEBUG_VIEW
        gfx_Line(px,py,ray_x,ray_y);
        #endif

        float line_height = (12*GFX_LCD_HEIGHT)/distance;

        float lower_y = GFX_LCD_HEIGHT/2 - line_height;
        // float upper_y = GFX_LCD_HEIGHT/2 + line_height; 

        gfx_FillRectangle(ray*16, lower_y, 16, line_height*2);
    }
    
    #ifdef DEBUG_VIEW
    gfx_FillRectangle(px-5,py-5,10,10);
    float2str(ntanLookup(ra), result);
    gfx_PrintStringXY(result, 20, 20);
    #endif
}



int main(void)
{
    int i;

    background->width = oiram_width;
    background->height = oiram_height;

    /* Coordinates used for the sprite */
    int x = START_X;
    int y = START_Y;
    int x_dir = START_X;
    int y_dir = START_Y;

    px = 120;
    py = 120;
    pa = 5.0;

    /* Initialize the graphics */
    gfx_Begin();

    /* Set the palette for the sprites */
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_FillScreen(1);
    gfx_SetTransparentColor(0);
    gfx_SetColor(0);

    /* Draw to the offscreen buffer */
    gfx_SetDrawBuffer();

    /* Copy the buffer to the screen */
    /* Same as gfx_Blit(gfx_buffer) */
    gfx_BlitBuffer();
    

    do
    {
        kb_key_t arrows;

        /* Scan the keypad to update kb_Data */
        kb_Scan();

        /* Get the arrow key statuses */
        arrows = kb_Data[7];

        x_dir = START_X;
        y_dir = START_Y;

        /* Check if any arrows are pressed */
        if (arrows)
        {
            if (arrows & kb_Right) {
                pa += 0.2;

                if (pa > 2*M_PI) {
                    pa -= 2*M_PI;
                }
            }
            if (arrows & kb_Left) {
                pa -= 0.2;

                if (pa < 0) {
                    pa += 2*M_PI;
                }
            }

            /* Calculate move delta */ 
            pdx = cos(pa)*5;
            pdy = sin(pa)*5;

            
            if (arrows & kb_Up) {
                px += pdx;
                py += pdy;
            }
            
            if (arrows & kb_Down) {
                px -= pdx;
                py -= pdy;
            }
            
            /* Draw scene */
            gfx_FillScreen(1);
            raycast();
        }

        /* Copy the buffer to the screen */
        /* Same as gfx_Blit(gfx_buffer) */
        gfx_BlitBuffer();

    } while (kb_Data[6] != kb_Clear);

    /* End graphics drawing */
    gfx_End();

    return 0;
}

/* Function for drawing the main sprite */
void DrawSprite(int x, int y)
{
    static int oldX = START_X;
    static int oldY = START_Y;

    /* Render the original background */
    gfx_Sprite(background, oldX, oldY);

    /* Get the background behind the sprite */
    gfx_GetSprite(background, x, y);

    /* Render the sprite */
    gfx_TransparentSprite(oiram, x, y);

    oldX = x;
    oldY = y;
}
