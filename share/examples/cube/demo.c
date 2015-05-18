#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "cube.h"

#define AXIS_X 1
#define AXIS_Y 2
#define AXIS_Z 3

unsigned char cube[8][8];

// Delay loop.
// This is not calibrated to milliseconds,
// but we had allready made to many effects using this
// calibration when we figured it might be a good idea
// to calibrate it.
void delay_ms(unsigned duration)
{
    struct timeval t0, now;
    int z, msec;

    gettimeofday(&t0, 0);
    z = 0;
    for (;;) {
        /* Send layer data. Latch is active,
         * so previous layer is still displayed. */
        gpio_plane(cube[z]);

        /* Disable output, activate latch,
         * switch to next layer. */
        gpio_oe(0);
        gpio_le(0);
        gpio_le(1);
        gpio_layer(z);
        gpio_oe(1);

        /* Next layer. */
        z++;
        if (z >= CUBE_SIZE) {
            z = 0;

            /* Check time. */
            gettimeofday(&now, 0);
            msec = (now.tv_sec - t0.tv_sec) * 1000;
            msec += (now.tv_usec - t0.tv_usec) / 1000;
            if (msec >= duration)
                break;
        }
    }
}

// ==========================================================================================
//   Draw functions
// ==========================================================================================

// Set a single voxel to ON
void setvoxel(int x, int y, int z)
{
    if (inrange(x, y, z))
        cube[z][y] |= (1 << x);
}

// Set a single voxel to ON
void clrvoxel(int x, int y, int z)
{
    if (inrange(x, y, z))
        cube[z][y] &= ~(1 << x);
}

// This function validates that we are drawing inside the cube.
int inrange(int x, int y, int z)
{
    return x >= 0 && x < 8 && y >= 0 && y < 8 && z >= 0 && z < 8;
}

// Get the current status of a voxel
int getvoxel(int x, int y, int z)
{
    if (! inrange(x, y, z))
        return 0;

    return (cube[z][y] >> x) & 1;
}

// In some effect we want to just take bool and write it to a voxel
// this function calls the apropriate voxel manipulation function.
void altervoxel(int x, int y, int z, int state)
{
    if (state == 1) {
        setvoxel(x, y, z);
    } else {
        clrvoxel(x, y, z);
    }
}

// Flip the state of a voxel.
// If the voxel is 1, its turned into a 0, and vice versa.
void flpvoxel(int x, int y, int z)
{
    if (inrange(x, y, z))
        cube[z][y] ^= (1 << x);
}

// Makes sure x1 is alwas smaller than x2
// This is usefull for functions that uses for loops,
// to avoid infinite loops
void argorder(int ix1, int ix2, int *ox1, int *ox2)
{
	if (ix1>ix2)
	{
		int tmp;
		tmp = ix1;
		ix1= ix2;
		ix2 = tmp;
	}
	*ox1 = ix1;
	*ox2 = ix2;
}

// Sets all voxels along a X/Y plane at a given point
// on axis Z
void setplane_z(int z)
{
	int i;

	if (z>=0 && z<8)
	{
		for (i=0;i<8;i++)
			cube[z][i] = 0xff;
	}
}

// Clears voxels in the same manner as above
void clrplane_z (int z)
{
	int i;

	if (z>=0 && z<8)
	{
		for (i=0;i<8;i++)
			cube[z][i] = 0x00;
	}
}

void setplane_x(int x)
{
	int z, y;

	if (x>=0 && x<8)
	{
		for (z=0;z<8;z++)
		{
			for (y=0;y<8;y++)
			{
				cube[z][y] |= (1 << x);
			}
		}
	}
}

void clrplane_x(int x)
{
	int z;
	int y;
	if (x>=0 && x<8)
	{
		for (z=0;z<8;z++)
		{
			for (y=0;y<8;y++)
			{
				cube[z][y] &= ~(1 << x);
			}
		}
	}
}

void setplane_y(int y)
{
	int z;
	if (y>=0 && y<8)
	{
		for (z=0;z<8;z++)
			cube[z][y] = 0xff;
	}
}

void clrplane_y(int y)
{
	int z;
	if (y>=0 && y<8)
	{
		for (z=0;z<8;z++)
			cube[z][y] = 0x00;
	}
}

void setplane(char axis, unsigned char i)
{
    switch (axis)
    {
        case AXIS_X:
            setplane_x(i);
            break;

       case AXIS_Y:
            setplane_y(i);
            break;

       case AXIS_Z:
            setplane_z(i);
            break;
    }
}

void clrplane(char axis, unsigned char i)
{
    switch (axis)
    {
        case AXIS_X:
            clrplane_x(i);
            break;

       case AXIS_Y:
            clrplane_y(i);
            break;

       case AXIS_Z:
            clrplane_z(i);
            break;
    }
}

// Fill a value into all 64 byts of the cube buffer
// Mostly used for clearing. fill(0x00)
// or setting all on. fill(0xff)
void fill(unsigned char pattern)
{
	int z;
	int y;
	for (z=0;z<8;z++)
	{
		for (y=0;y<8;y++)
		{
			cube[z][y] = pattern;
		}
	}
}

// Returns a byte with a row of 1's drawn in it.
// byteline(2,5) gives 0b00111100
char byteline(int start, int end)
{
	return (0xff << start) & ~(0xff << (end+1));
}

// Draw a box with all walls drawn and all voxels inside set
void box_filled(int x1, int y1, int z1, int x2, int y2, int z2)
{
	int iy;
	int iz;

	argorder(x1, x2, &x1, &x2);
	argorder(y1, y2, &y1, &y2);
	argorder(z1, z2, &z1, &z2);

	for (iz=z1;iz<=z2;iz++)
	{
		for (iy=y1;iy<=y2;iy++)
		{
			cube[iz][iy] |= byteline(x1, x2);
		}
	}

}

// Darw a hollow box with side walls.
void box_walls(int x1, int y1, int z1, int x2, int y2, int z2)
{
	int iy;
	int iz;

	argorder(x1, x2, &x1, &x2);
	argorder(y1, y2, &y1, &y2);
	argorder(z1, z2, &z1, &z2);

	for (iz=z1;iz<=z2;iz++)
	{
		for (iy=y1;iy<=y2;iy++)
		{
			if (iy == y1 || iy == y2 || iz == z1 || iz == z2)
			{
				cube[iz][iy] = byteline(x1, x2);
			} else
			{
				cube[iz][iy] |= ((0x01 << x1) | (0x01 << x2));
			}
		}
	}

}

// Draw a wireframe box. This only draws the corners and edges,
// no walls.
void box_wireframe(int x1, int y1, int z1, int x2, int y2, int z2)
{
	int iy;
	int iz;

	argorder(x1, x2, &x1, &x2);
	argorder(y1, y2, &y1, &y2);
	argorder(z1, z2, &z1, &z2);

	// Lines along X axis
	cube[z1][y1] = byteline(x1, x2);
	cube[z1][y2] = byteline(x1, x2);
	cube[z2][y1] = byteline(x1, x2);
	cube[z2][y2] = byteline(x1, x2);

	// Lines along Y axis
	for (iy=y1;iy<=y2;iy++)
	{
		setvoxel(x1, iy, z1);
		setvoxel(x1, iy, z2);
		setvoxel(x2, iy, z1);
		setvoxel(x2, iy, z2);
	}

	// Lines along Z axis
	for (iz=z1;iz<=z2;iz++)
	{
		setvoxel(x1, y1, iz);
		setvoxel(x1, y2, iz);
		setvoxel(x2, y1, iz);
		setvoxel(x2, y2, iz);
	}

}

// Draw a line between any coordinates in 3d space.
// Uses integer values for input, so dont expect smooth animations.
void line(int x1, int y1, int z1, int x2, int y2, int z2)
{
    int x, y, z, dx, dy, dz;
    int lasty, lastz;

    // We always want to draw the line from x=0 to x=7.
    // If x1 is bigget than x2, we need to flip all the values.
    if (x1 > x2) {
        int tmp;
        tmp = x2; x2 = x1; x1 = tmp;
        tmp = y2; y2 = y1; y1 = tmp;
        tmp = z2; z2 = z1; z1 = tmp;
    }
    dx = x2 - x1;

    if (y1 > y2) {
        dy = y1 - y2;
        lasty = y2;
    } else {
        dy = y2 - y1;
        lasty = y1;
    }

    if (z1 > z2) {
        dz = z1 - z2;
        lastz = z2;
    } else {
        dz = z2 - z1;
        lastz = z1;
    }

    // For each step of x, y increments by:
    for (x = x1; x <= x2; x++) {
        y = (dy * (x-x1) / dx) + y1;
        z = (dz * (x-x1) / dx) + z1;
        setvoxel(x, y, z);
    }
}

// Shift the entire contents of the cube along an axis
// This is great for effects where you want to draw something
// on one side of the cube and have it flow towards the other
// side. Like rain flowing down the Z axiz.
void shift(char axis, int direction)
{
	int i, x, y;
	int ii, iii;
	int state;

	for (i = 0; i < 8; i++)
	{
		if (direction == -1)
		{
			ii = i;
		} else
		{
			ii = (7-i);
		}


		for (x = 0; x < 8; x++)
		{
			for (y = 0; y < 8; y++)
			{
				if (direction == -1)
				{
					iii = ii+1;
				} else
				{
					iii = ii-1;
				}

				if (axis == AXIS_Z)
				{
					state = getvoxel(x, y, iii);
					altervoxel(x, y, ii, state);
				}

				if (axis == AXIS_Y)
				{
					state = getvoxel(x, iii, y);
					altervoxel(x, ii, y, state);
				}

				if (axis == AXIS_X)
				{
					state = getvoxel(iii, y, x);
					altervoxel(ii, y, x, state);
				}
			}
		}
	}

	if (direction == -1)
	{
		i = 7;
	} else
	{
		i = 0;
	}

	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			if (axis == AXIS_Z)
				clrvoxel(x, y, i);

			if (axis == AXIS_Y)
				clrvoxel(x, i, y);

			if (axis == AXIS_X)
				clrvoxel(i, y, x);
		}
	}
}

// ==========================================================================================
//   Effect functions
// ==========================================================================================

void fireworks(int iterations, int delay)
{
#define NFIREWORKS 30
    int i, f, e;
    int origin_x = 3;
    int origin_y = 3;
    int origin_z = 3;
    int rand_y, rand_x, rand_z;
    int slowrate, gravity;

    // Particles and their position, x,y,z and their movement, dx,dy,dz.
    // Scaled by 100 for integer arith.
    int particles[NFIREWORKS][6];

    fill(0x00);
    for (i=0; i<iterations; i++) {

        origin_x = rand() % 4;
        origin_y = rand() % 4;
        origin_z = rand() % 2;
        origin_z += 5;
        origin_x += 2;
        origin_y += 2;

        // shoot a particle up in the air
        for (e=0; e<origin_z; e++) {
            setvoxel(origin_x, origin_y, e);
            delay_ms(300+250*e);
            fill(0x00);
        }

        // Fill particle array
        for (f=0; f<NFIREWORKS; f++) {
            // Position
            particles[f][0] = origin_x * 100;
            particles[f][1] = origin_y * 100;
            particles[f][2] = origin_z * 100;

            rand_x = rand() % 200;
            rand_y = rand() % 200;
            rand_z = rand() % 200;

            // Movement
            particles[f][3] = 100 - rand_x; // dx
            particles[f][4] = 100 - rand_y; // dy
            particles[f][5] = 100 - rand_z; // dz
        }

        // explode
        for (e=0; e<25; e++) {
            // Coefficient = tan((e+0.1)/20) * 1000.
            static const int coeff[25] = {
                5, 55, 105, 156, 207, 260, 314, 370, 428, 489,
                552, 620, 691, 768, 850, 940, 1039, 1149, 1273, 1413,
                1574, 1763, 1989, 2264, 2610,
            };

            slowrate = 100 + coeff[e];
            gravity = coeff[e] / 20;

            for (f=0; f<NFIREWORKS; f++) {
                particles[f][0] += particles[f][3] * 100 / slowrate;
                particles[f][1] += particles[f][4] * 100 / slowrate;
                particles[f][2] += particles[f][5] * 100 / slowrate;
                particles[f][2] -= gravity;

                setvoxel(particles[f][0] / 100,
                         particles[f][1] / 100,
                         particles[f][2] / 100);
            }
            delay_ms(delay);
            fill(0x00);
        }
    }
}

const unsigned char LUT[65] = {
    0, 8, 17, 26, 35, 43, 52, 60, 69, 77,
    85, 93, 100, 107, 114, 121, 127, 134, 139, 145,
    150, 155, 159, 163, 167, 170, 173, 175, 177, 179,
    180, 180, 181, 180, 180, 179, 177, 175, 173, 170,
    167, 163, 159, 155, 150, 145, 139, 134, 127, 121,
    114, 107, 100, 93, 85, 77, 69, 60, 52, 43,
    35, 26, 17, 8, 0,
};

int totty_sin(int sin_of)
{
    unsigned char inv = 0;
    if (sin_of < 0) {
        sin_of = -sin_of;
        inv = 1;
    }
    sin_of &= 0x7f; //127
    if (sin_of > 64) {
        sin_of -= 64;
        inv = 1-inv;
    }
    if (inv)
        return -LUT[sin_of];
    else
        return LUT[sin_of];
}

int totty_cos(int cos_of)
{
    unsigned char inv = 0;

    cos_of += 32;   // Simply rotate by 90 degrees for COS
    cos_of &= 0x7f; // 127
    if (cos_of > 64) {
        cos_of -= 64;
        inv = 1;
    }
    if (inv)
        return -LUT[cos_of];
    else
        return LUT[cos_of];
}

void quad_ripples(int iterations, int delay)
{
    // 16 values for square root of a^2+b^2.  index a*4+b = 10*sqrt
    // This gives the distance to 3.5,3.5 from the point
    unsigned char sqrt_LUT[] = {
        49, 43, 38, 35, 43, 35, 29, 26, 38, 29, 21, 16, 35, 25, 16, 7
    };
    unsigned char x, y, height, distance;
    int i;

    for (i=0; i<iterations*4; i+=4) {
        fill(0x00);
        for (x=0; x<4; x++) {
            for (y=0; y<4; y++) {
                // x+y*4 gives no. from 0-15 for sqrt_LUT
                distance = sqrt_LUT[x + y*4]; // distance is 0-50 roughly

                // height is sin of distance + iteration*4
                //height = 4 + totty_sin(distance+i) / 52;
                height = (196 + totty_sin(distance+i)) / 49;

                // Use 4-way mirroring to save on calculations
                setvoxel(x,   y,        height);
                setvoxel(7-x, y,        height);
                setvoxel(x,   7-y,      height);
                setvoxel(7-x, 7-y,      height);
                setvoxel(x,   y,        7-height);
                setvoxel(7-x, y,        7-height);
                setvoxel(x,   7-y,      7-height);
                setvoxel(7-x, 7-y,      7-height);
                setvoxel(x,   height,   y);
                setvoxel(7-x, height,   y);
                setvoxel(x,   height,   7-y);
                setvoxel(7-x, height,   7-y);
                setvoxel(x,   7-height, y);
                setvoxel(7-x, 7-height, y);
                setvoxel(x,   7-height, 7-y);
                setvoxel(7-x, 7-height, 7-y);
            }
        }
        delay_ms(delay);
    }
}

// **********************************************************

void effect_random_sparkle_flash(int iterations, int voxels, int delay)
{
	int i;
	int v;
	for (i = 0; i < iterations; i++)
	{
		for (v=0;v<=voxels;v++)
			setvoxel(rand()%8, rand()%8, rand()%8);

        delay_ms(delay);
		fill(0x00);
	}
}

// blink 1 random voxel, blink 2 random voxels..... blink 20 random voxels
// and back to 1 again.
void effect_random_sparkle(void)
{
	int i;

	for (i=1;i<20;i++)
	{
        effect_random_sparkle_flash(5, i, 100);
	}

	for (i=20;i>=1;i--)
	{
        effect_random_sparkle_flash(5, i, 100);
	}

}

int effect_telcstairs_do(int x, int val, int delay)
{
	int y, z;

	for(y = 0, z = x; y <= z; y++, x--)
	{
		if(x < CUBE_SIZE && y < CUBE_SIZE)
		{
			cube[x][y] = val;
		}
	}
	delay_ms(delay);
	return z;
}

void effect_telcstairs(int invert, int delay, int val)
{
	int x;

	if(invert)
	{
		for(x = CUBE_SIZE*2; x >= 0; x--)
		{
			x = effect_telcstairs_do(x, val, delay);
		}
	}
	else
	{
		for(x = 0; x < CUBE_SIZE*2; x++)
		{
			x = effect_telcstairs_do(x, val, delay);
		}
	}
}

void draw_positions_axis(char axis, unsigned char positions[64], int invert)
{
	int x, y, p;

	fill(0x00);

	for (x=0; x<8; x++)
	{
		for (y=0; y<8; y++)
		{
			if (invert)
			{
				p = (7-positions[(x*8)+y]);
			} else
			{
				p = positions[(x*8)+y];
			}

			if (axis == AXIS_Z)
				setvoxel(x, y, p);

			if (axis == AXIS_Y)
				setvoxel(x, p, y);

			if (axis == AXIS_X)
				setvoxel(p, y, x);
		}
	}

}

void effect_wormsqueeze(int size, int axis, int direction, int iterations,
    int delay)
{
	int x, y, i, j, k, dx, dy;
	int cube_size;
	int origin = 0;

	if (direction == -1)
		origin = 7;

	cube_size = 8-(size-1);

	x = rand()%cube_size;
	y = rand()%cube_size;

	for (i=0; i<iterations; i++)
	{
		dx = ((rand()%3)-1);
		dy = ((rand()%3)-1);

		if ((x+dx) > 0 && (x+dx) < cube_size)
			x += dx;

		if ((y+dy) > 0 && (y+dy) < cube_size)
			y += dy;

		shift(axis, direction);


		for (j=0; j<size;j++)
		{
			for (k=0; k<size;k++)
			{
				if (axis == AXIS_Z)
					setvoxel(x+j, y+k, origin);

				if (axis == AXIS_Y)
					setvoxel(x+j, origin, y+k);

				if (axis == AXIS_X)
					setvoxel(origin, y+j, x+k);
			}
		}

		delay_ms(delay);
	}
}

void line_3d(int x1, int y1, int z1, int x2, int y2, int z2)
{
    int i, dx, dy, dz, l, m, n, x_inc, y_inc, z_inc,
        err_1, err_2, dx2, dy2, dz2;
    int pixel[3];

    pixel[0] = x1;
    pixel[1] = y1;
    pixel[2] = z1;
    dx = x2 - x1;
    dy = y2 - y1;
    dz = z2 - z1;
    if (dx >= 0) {
        x_inc = 1;
        l = dx;
    } else {
        x_inc = -1;
        l = -dx;
    }
    if (dy >= 0) {
        y_inc = 1;
        m = dy;
    } else {
        y_inc = -1;
        m = -dy;
    }
    if (dz >= 0) {
        z_inc = 1;
        n = dz;
    } else {
        z_inc = -1;
        n = -dz;
    }
    dx2 = l << 1;
    dy2 = m << 1;
    dz2 = n << 1;
    if ((l >= m) && (l >= n)) {
        err_1 = dy2 - l;
        err_2 = dz2 - l;
        for (i = 0; i < l; i++) {
            //PUT_PIXEL(pixel);
            setvoxel(pixel[0], pixel[1], pixel[2]);
            //printf("Setting %i %i %i \n", pixel[0], pixel[1], pixel[2]);
            if (err_1 > 0) {
                pixel[1] += y_inc;
                err_1 -= dx2;
            }
            if (err_2 > 0) {
                pixel[2] += z_inc;
                err_2 -= dx2;
            }
            err_1 += dy2;
            err_2 += dz2;
            pixel[0] += x_inc;
        }
    } else if ((m >= l) && (m >= n)) {
        err_1 = dx2 - m;
        err_2 = dz2 - m;
        for (i = 0; i < m; i++) {
            //PUT_PIXEL(pixel);
            setvoxel(pixel[0], pixel[1], pixel[2]);
            //printf("Setting %i %i %i \n", pixel[0], pixel[1], pixel[2]);
            if (err_1 > 0) {
                pixel[0] += x_inc;
                err_1 -= dy2;
            }
            if (err_2 > 0) {
                pixel[2] += z_inc;
                err_2 -= dy2;
            }
            err_1 += dx2;
            err_2 += dz2;
            pixel[1] += y_inc;
        }
    } else {
        err_1 = dy2 - n;
        err_2 = dx2 - n;
        for (i = 0; i < n; i++) {
            setvoxel(pixel[0], pixel[1], pixel[2]);
            //printf("Setting %i %i %i \n", pixel[0], pixel[1], pixel[2]);
            //PUT_PIXEL(pixel);
            if (err_1 > 0) {
                pixel[1] += y_inc;
                err_1 -= dz2;
            }
            if (err_2 > 0) {
                pixel[0] += x_inc;
                err_2 -= dz2;
            }
            err_1 += dy2;
            err_2 += dx2;
            pixel[2] += z_inc;
        }
    }
    setvoxel(pixel[0], pixel[1], pixel[2]);
    //printf("Setting %i %i %i \n", pixel[0], pixel[1], pixel[2]);
    //PUT_PIXEL(pixel);
}

#if 0
void sinelines(int iterations, int delay)
{
    int i, x;

    float left, right, sine_base, x_dividor, ripple_height;

    for (i=0; i<iterations; i++) {
        for (x=0; x<8; x++) {
            x_dividor = 2 + sin((float)i / 100) + 1;
            ripple_height = 3 + (sin((float)i / 200) + 1) * 6;

            sine_base = (float) i/40 + (float) x/x_dividor;

            left = 4 + sin(sine_base) * ripple_height;
            right = 4 + cos(sine_base) * ripple_height;
            right = 7-left;

            //printf("%i %i \n", (int) left, (int) right);

            line_3d(0-3, x, (int) left, 7+3, x, (int) right);
            //line_3d((int) right, 7, x);
        }

        // delay_ms(delay);
        fill(0x00);
    }
}
#endif

void effect_boxside_randsend_parallel(char axis, int origin, int delay,
    int mode)
{
	int i;
	int done;
	unsigned char cubepos[64];
	unsigned char pos[64];
	int notdone = 1;
	int notdone2 = 1;
	int sent = 0;

	for (i=0;i<64;i++)
	{
		pos[i] = 0;
	}

	while (notdone)
	{
		if (mode == 1)
		{
			notdone2 = 1;
			while (notdone2 && sent<64)
			{
				i = rand()%64;
				if (pos[i] == 0)
				{
					sent++;
					pos[i] += 1;
					notdone2 = 0;
				}
			}
		} else if (mode == 2)
		{
			if (sent<64)
			{
				pos[sent] += 1;
				sent++;
			}
		}

		done = 0;
		for (i=0;i<64;i++)
		{
			if (pos[i] > 0 && pos[i] <7)
			{
				pos[i] += 1;
			}

			if (pos[i] == 7)
				done++;
		}

		if (done == 64)
			notdone = 0;

		for (i=0;i<64;i++)
		{
			if (origin == 0)
			{
				cubepos[i] = pos[i];
			} else
			{
				cubepos[i] = (7-pos[i]);
			}
		}

		delay_ms(delay);
		draw_positions_axis(axis, cubepos, 0);
	}

}

void effect_rain(int iterations)
{
	int i, ii;
	int rnd_x;
	int rnd_y;
	int rnd_num;

	for (ii=0;ii<iterations;ii++)
	{
		rnd_num = rand()%4;

		for (i=0; i < rnd_num;i++)
		{
			rnd_x = rand()%8;
			rnd_y = rand()%8;
			setvoxel(rnd_x, rnd_y, 7);
		}

        delay_ms(500);
		shift(AXIS_Z, -1);
	}
}

// Set or clear exactly 512 voxels in a random order.
void effect_random_filler(int delay, int state)
{
	int x, y, z;
	int loop = 0;

    if (state == 1) {
		fill(0x00);
    } else {
		fill(0xff);
	}

    while (loop < 511)
	{
		x = rand()%8;
		y = rand()%8;
		z = rand()%8;

        if ((state == 0 && getvoxel(x, y, z) == 1) ||
            (state == 1 && getvoxel(x, y, z) == 0))
		{
			altervoxel(x, y, z, state);
			delay_ms(delay);
			loop++;
		}
	}
}

void effect_axis_updown_randsuspend(char axis, int delay, int sleep, int invert)
{
	unsigned char positions[64];
	unsigned char destinations[64];

	int i, px;

    // Set 64 random positions
	for (i=0; i<64; i++)
	{
		positions[i] = 0; // Set all starting positions to 0
		destinations[i] = rand()%8;
	}

    // Loop 8 times to allow destination 7 to reach all the way
	for (i=0; i<8; i++)
	{
        // For every iteration, move all position one step closer to their destination
		for (px=0; px<64; px++)
		{
			if (positions[px]<destinations[px])
			{
				positions[px]++;
			}
		}
        // Draw the positions and take a nap
		draw_positions_axis(axis, positions, invert);
		delay_ms(delay);
	}

    // Set all destinations to 7 (opposite from the side they started out)
	for (i=0; i<64; i++)
	{
		destinations[i] = 7;
	}

    // Suspend the positions in mid-air for a while
	delay_ms(sleep);

    // Then do the same thing one more time
	for (i=0; i<8; i++)
	{
		for (px=0; px<64; px++)
		{
			if (positions[px]<destinations[px])
			{
				positions[px]++;
			}
			if (positions[px]>destinations[px])
			{
				positions[px]--;
			}
		}
		draw_positions_axis(axis, positions, invert);
		delay_ms(delay);
	}
}

void effect_blinky2()
{
	int i, r;
	fill(0x00);

	for (r=0;r<2;r++)
	{
        i = 350;
		while (i>0)
		{
			fill(0x00);
			delay_ms(i);

			fill(0xff);
			delay_ms(100);

            i = i - (7 + (500 / (i/10)));
		}

        delay_ms(500);

        i = 350;
		while (i>0)
		{
			fill(0x00);
            delay_ms(351-i);

			fill(0xff);
			delay_ms(100);

            i = i - (7 + (500 / (i/10)));
		}
	}
}

// Draw a plane on one axis and send it back and forth once.
void effect_planboing(int plane, int speed)
{
	int i;
	for (i=0;i<8;i++)
	{
		fill(0x00);
                setplane(plane, i);
		delay_ms(speed);
	}

	for (i=7;i>=0;i--)
	{
		fill(0x00);
                setplane(plane, i);
		delay_ms(speed);
	}
}


//********************************************************

// ********************************************************
const char font_data[128][8] =
{
  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00 },    // 0 :
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |

  { 0x00,  0x3E,  0x41,  0x55,  0x41,  0x55,  0x49,  0x3E },    // 1 : 
                //              |          |
                //              |   *****  |
                //              |  *     * |
                //              |  * * * * |
                //              |  *     * |
                //              |  * * * * |
                //              |  *  *  * |
                //              |   *****  |

  { 0x00,  0x3E,  0x7F,  0x6B,  0x7F,  0x6B,  0x77,  0x3E },    // 2 : 
                //              |          |
                //              |   *****  |
                //              |  ******* |
                //              |  ** * ** |
                //              |  ******* |
                //              |  ** * ** |
                //              |  *** *** |
                //              |   *****  |

  { 0x00,  0x22,  0x77,  0x7F,  0x7F,  0x3E,  0x1C,  0x08 },    // 3 : 
                //              |          |
                //              |   *   *  |
                //              |  *** *** |
                //              |  ******* |
                //              |  ******* |
                //              |   *****  |
                //              |    ***   |
                //              |     *    |

  { 0x00,  0x08,  0x1C,  0x3E,  0x7F,  0x3E,  0x1C,  0x08 },    // 4 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   *****  |
                //              |  ******* |
                //              |   *****  |
                //              |    ***   |
                //              |     *    |

  { 0x00,  0x08,  0x1C,  0x2A,  0x7F,  0x2A,  0x08,  0x1C },    // 5 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   * * *  |
                //              |  ******* |
                //              |   * * *  |
                //              |     *    |
                //              |    ***   |

  { 0x00,  0x08,  0x1C,  0x3E,  0x7F,  0x3E,  0x08,  0x1C },    // 6 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   *****  |
                //              |  ******* |
                //              |   *****  |
                //              |     *    |
                //              |    ***   |

  { 0x00,  0x00,  0x1C,  0x3E,  0x3E,  0x3E,  0x1C,  0x00 },    // 7 :
                //              |          |
                //              |          |
                //              |    ***   |
                //              |   *****  |
                //              |   *****  |
                //              |   *****  |
                //              |    ***   |
                //              |          |

  { 0xFF,  0xFF,  0xE3,  0xC1,  0xC1,  0xC1,  0xE3,  0xFF },    // 8 : 
                //              | ******** |
                //              | ******** |
                //              | ***   ** |
                //              | **     * |
                //              | **     * |
                //              | **     * |
                //              | ***   ** |
                //              | ******** |

  { 0x00,  0x00,  0x1C,  0x22,  0x22,  0x22,  0x1C,  0x00 },    // 9 :
                //              |          |
                //              |          |
                //              |    ***   |
                //              |   *   *  |
                //              |   *   *  |
                //              |   *   *  |
                //              |    ***   |
                //              |          |

  { 0xFF,  0xFF,  0xE3,  0xDD,  0xDD,  0xDD,  0xE3,  0xFF },    // 10 :

                //              | ******** |
                //              | ******** |
                //              | ***   ** |
                //              | ** *** * |
                //              | ** *** * |
                //              | ** *** * |
                //              | ***   ** |
                //              | ******** |

  { 0x00,  0x0F,  0x03,  0x05,  0x39,  0x48,  0x48,  0x30 },    // 11 :

                //              |          |
                //              |     **** |
                //              |       ** |
                //              |      * * |
                //              |   ***  * |
                //              |  *  *    |
                //              |  *  *    |
                //              |   **     |

  { 0x00,  0x08,  0x3E,  0x08,  0x1C,  0x22,  0x22,  0x1C },    // 12 : 
                //              |          |
                //              |     *    |
                //              |   *****  |
                //              |     *    |
                //              |    ***   |
                //              |   *   *  |
                //              |   *   *  |
                //              |    ***   |

  { 0x00,  0x18,  0x14,  0x10,  0x10,  0x30,  0x70,  0x60 },    // 13 :

                //              |          |
                //              |    **    |
                //              |    * *   |
                //              |    *     |
                //              |    *     |
                //              |   **     |
                //              |  ***     |
                //              |  **      |

  { 0x00,  0x0F,  0x19,  0x11,  0x13,  0x37,  0x76,  0x60 },    // 14 : 
                //              |          |
                //              |     **** |
                //              |    **  * |
                //              |    *   * |
                //              |    *  ** |
                //              |   ** *** |
                //              |  *** **  |
                //              |  **      |

  { 0x00,  0x08,  0x2A,  0x1C,  0x77,  0x1C,  0x2A,  0x08 },    // 15 : 
                //              |          |
                //              |     *    |
                //              |   * * *  |
                //              |    ***   |
                //              |  *** *** |
                //              |    ***   |
                //              |   * * *  |
                //              |     *    |

  { 0x00,  0x60,  0x78,  0x7E,  0x7F,  0x7E,  0x78,  0x60 },    // 16 : 
                //              |          |
                //              |  **      |
                //              |  ****    |
                //              |  ******  |
                //              |  ******* |
                //              |  ******  |
                //              |  ****    |
                //              |  **      |

  { 0x00,  0x03,  0x0F,  0x3F,  0x7F,  0x3F,  0x0F,  0x03 },    // 17 : 
                //              |          |
                //              |       ** |
                //              |     **** |
                //              |   ****** |
                //              |  ******* |
                //              |   ****** |
                //              |     **** |
                //              |       ** |

  { 0x00,  0x08,  0x1C,  0x2A,  0x08,  0x2A,  0x1C,  0x08 },    // 18 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   * * *  |
                //              |     *    |
                //              |   * * *  |
                //              |    ***   |
                //              |     *    |

  { 0x00,  0x66,  0x66,  0x66,  0x66,  0x00,  0x66,  0x66 },    // 19 : 
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |          |
                //              |  **  **  |
                //              |  **  **  |

  { 0x00,  0x3F,  0x65,  0x65,  0x3D,  0x05,  0x05,  0x05 },    // 20 : 
                //              |          |
                //              |   ****** |
                //              |  **  * * |
                //              |  **  * * |
                //              |   **** * |
                //              |      * * |
                //              |      * * |
                //              |      * * |

  { 0x00,  0x0C,  0x32,  0x48,  0x24,  0x12,  0x4C,  0x30 },    // 21 : 
                //              |          |
                //              |     **   |
                //              |   **  *  |
                //              |  *  *    |
                //              |   *  *   |
                //              |    *  *  |
                //              |  *  **   |
                //              |   **     |

  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x7F,  0x7F,  0x7F },    // 22 : 
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |  ******* |
                //              |  ******* |
                //              |  ******* |

  { 0x00,  0x08,  0x1C,  0x2A,  0x08,  0x2A,  0x1C,  0x3E },    // 23 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   * * *  |
                //              |     *    |
                //              |   * * *  |
                //              |    ***   |
                //              |   *****  |

  { 0x00,  0x08,  0x1C,  0x3E,  0x7F,  0x1C,  0x1C,  0x1C },    // 24 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   *****  |
                //              |  ******* |
                //              |    ***   |
                //              |    ***   |
                //              |    ***   |

  { 0x00,  0x1C,  0x1C,  0x1C,  0x7F,  0x3E,  0x1C,  0x08 },    // 25 : 
                //              |          |
                //              |    ***   |
                //              |    ***   |
                //              |    ***   |
                //              |  ******* |
                //              |   *****  |
                //              |    ***   |
                //              |     *    |

  { 0x00,  0x08,  0x0C,  0x7E,  0x7F,  0x7E,  0x0C,  0x08 },    // 26 : 
                //              |          |
                //              |     *    |
                //              |     **   |
                //              |  ******  |
                //              |  ******* |
                //              |  ******  |
                //              |     **   |
                //              |     *    |

  { 0x00,  0x08,  0x18,  0x3F,  0x7F,  0x3F,  0x18,  0x08 },    // 27 : 
                //              |          |
                //              |     *    |
                //              |    **    |
                //              |   ****** |
                //              |  ******* |
                //              |   ****** |
                //              |    **    |
                //              |     *    |

  { 0x00,  0x00,  0x00,  0x70,  0x70,  0x70,  0x7F,  0x7F },    // 28 : 
                //              |          |
                //              |          |
                //              |          |
                //              |  ***     |
                //              |  ***     |
                //              |  ***     |
                //              |  ******* |
                //              |  ******* |

  { 0x00,  0x00,  0x14,  0x22,  0x7F,  0x22,  0x14,  0x00 },    // 29 : 
                //              |          |
                //              |          |
                //              |    * *   |
                //              |   *   *  |
                //              |  ******* |
                //              |   *   *  |
                //              |    * *   |
                //              |          |

  { 0x00,  0x08,  0x1C,  0x1C,  0x3E,  0x3E,  0x7F,  0x7F },    // 30 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |    ***   |
                //              |   *****  |
                //              |   *****  |
                //              |  ******* |
                //              |  ******* |

  { 0x00,  0x7F,  0x7F,  0x3E,  0x3E,  0x1C,  0x1C,  0x08 },    // 31 : 
                //              |          |
                //              |  ******* |
                //              |  ******* |
                //              |   *****  |
                //              |   *****  |
                //              |    ***   |
                //              |    ***   |
                //              |     *    |

  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00 },    // 32 :
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |

  { 0x00,  0x18,  0x18,  0x18,  0x18,  0x18,  0x00,  0x18 },    // 33 : !
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |          |
                //              |    **    |

  { 0x00,  0x36,  0x36,  0x14,  0x00,  0x00,  0x00,  0x00 },    // 34 : "
                //              |          |
                //              |   ** **  |
                //              |   ** **  |
                //              |    * *   |
                //              |          |
                //              |          |
                //              |          |
                //              |          |

  { 0x00,  0x36,  0x36,  0x7F,  0x36,  0x7F,  0x36,  0x36 },    // 35 : #
                //              |          |
                //              |   ** **  |
                //              |   ** **  |
                //              |  ******* |
                //              |   ** **  |
                //              |  ******* |
                //              |   ** **  |
                //              |   ** **  |

  { 0x00,  0x08,  0x1E,  0x20,  0x1C,  0x02,  0x3C,  0x08 },    // 36 : $
                //              |          |
                //              |     *    |
                //              |    ****  |
                //              |   *      |
                //              |    ***   |
                //              |       *  |
                //              |   ****   |
                //              |     *    |

  { 0x00,  0x60,  0x66,  0x0C,  0x18,  0x30,  0x66,  0x06 },    // 37 : %
                //              |          |
                //              |  **      |
                //              |  **  **  |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |  **  **  |
                //              |      **  |

  { 0x00,  0x3C,  0x66,  0x3C,  0x28,  0x65,  0x66,  0x3F },    // 38 : &
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |   ****   |
                //              |   * *    |
                //              |  **  * * |
                //              |  **  **  |
                //              |   ****** |

  { 0x00,  0x18,  0x18,  0x18,  0x30,  0x00,  0x00,  0x00 },    // 39 : '
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |   **     |
                //              |          |
                //              |          |
                //              |          |

  { 0x00,  0x60,  0x30,  0x18,  0x18,  0x18,  0x30,  0x60 },    // 40 : (
                //              |          |
                //              |  **      |
                //              |   **     |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |   **     |
                //              |  **      |

  { 0x00,  0x06,  0x0C,  0x18,  0x18,  0x18,  0x0C,  0x06 },    // 41 : )
                //              |          |
                //              |      **  |
                //              |     **   |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |     **   |
                //              |      **  |

  { 0x00,  0x00,  0x36,  0x1C,  0x7F,  0x1C,  0x36,  0x00 },    // 42 : *
                //              |          |
                //              |          |
                //              |   ** **  |
                //              |    ***   |
                //              |  ******* |
                //              |    ***   |
                //              |   ** **  |
                //              |          |

  { 0x00,  0x00,  0x08,  0x08,  0x3E,  0x08,  0x08,  0x00 },    // 43 : +
                //              |          |
                //              |          |
                //              |     *    |
                //              |     *    |
                //              |   *****  |
                //              |     *    |
                //              |     *    |
                //              |          |

  { 0x00,  0x00,  0x00,  0x00,  0x30,  0x30,  0x30,  0x60 },    // 44 : ,
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |   **     |
                //              |   **     |
                //              |   **     |
                //              |  **      |

  { 0x00,  0x00,  0x00,  0x00,  0x3C,  0x00,  0x00,  0x00 },    // 45 : -
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |          |
                //              |          |
                //              |          |

  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x60,  0x60 },    // 46 : .
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |  **      |
                //              |  **      |

  { 0x00,  0x00,  0x06,  0x0C,  0x18,  0x30,  0x60,  0x00 },    // 47 : /
                //              |          |
                //              |          |
                //              |      **  |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |  **      |
                //              |          |

  { 0x00,  0x3C,  0x66,  0x6E,  0x76,  0x66,  0x66,  0x3C },    // 48 : 0
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  ** ***  |
                //              |  *** **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x18,  0x18,  0x38,  0x18,  0x18,  0x18,  0x7E },    // 49 : 1
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |   ***    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |  ******  |

  { 0x00,  0x3C,  0x66,  0x06,  0x0C,  0x30,  0x60,  0x7E },    // 50 : 2
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |      **  |
                //              |     **   |
                //              |   **     |
                //              |  **      |
                //              |  ******  |

  { 0x00,  0x3C,  0x66,  0x06,  0x1C,  0x06,  0x66,  0x3C },    // 51 : 3
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |      **  |
                //              |    ***   |
                //              |      **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x0C,  0x1C,  0x2C,  0x4C,  0x7E,  0x0C,  0x0C },    // 52 : 4
                //              |          |
                //              |     **   |
                //              |    ***   |
                //              |   * **   |
                //              |  *  **   |
                //              |  ******  |
                //              |     **   |
                //              |     **   |

  { 0x00,  0x7E,  0x60,  0x7C,  0x06,  0x06,  0x66,  0x3C },    // 53 : 5
                //              |          |
                //              |  ******  |
                //              |  **      |
                //              |  *****   |
                //              |      **  |
                //              |      **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x3C,  0x66,  0x60,  0x7C,  0x66,  0x66,  0x3C },    // 54 : 6
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x7E,  0x66,  0x0C,  0x0C,  0x18,  0x18,  0x18 },    // 55 : 7
                //              |          |
                //              |  ******  |
                //              |  **  **  |
                //              |     **   |
                //              |     **   |
                //              |    **    |
                //              |    **    |
                //              |    **    |

  { 0x00,  0x3C,  0x66,  0x66,  0x3C,  0x66,  0x66,  0x3C },    // 56 : 8
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x3C,  0x66,  0x66,  0x3E,  0x06,  0x66,  0x3C },    // 57 : 9
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
                //              |      **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x00,  0x18,  0x18,  0x00,  0x18,  0x18,  0x00 },    // 58 : :
                //              |          |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |          |

  { 0x00,  0x00,  0x18,  0x18,  0x00,  0x18,  0x18,  0x30 },    // 59 : ;
                //              |          |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |   **     |

  { 0x00,  0x06,  0x0C,  0x18,  0x30,  0x18,  0x0C,  0x06 },    // 60 : <
                //              |          |
                //              |      **  |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |    **    |
                //              |     **   |
                //              |      **  |

  { 0x00,  0x00,  0x00,  0x3C,  0x00,  0x3C,  0x00,  0x00 },    // 61 : =
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |          |
                //              |   ****   |
                //              |          |
                //              |          |

  { 0x00,  0x60,  0x30,  0x18,  0x0C,  0x18,  0x30,  0x60 },    // 62 : >
                //              |          |
                //              |  **      |
                //              |   **     |
                //              |    **    |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |  **      |

  { 0x00,  0x3C,  0x66,  0x06,  0x1C,  0x18,  0x00,  0x18 },    // 63 : ?
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |      **  |
                //              |    ***   |
                //              |    **    |
                //              |          |
                //              |    **    |

  { 0x00,  0x38,  0x44,  0x5C,  0x58,  0x42,  0x3C,  0x00 },    // 64 : @
                //              |          |
                //              |   ***    |
                //              |  *   *   |
                //              |  * ***   |
                //              |  * **    |
                //              |  *    *  |
                //              |   ****   |
                //              |          |

  { 0x00,  0x3C,  0x66,  0x66,  0x7E,  0x66,  0x66,  0x66 },    // 65 : A
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  ******  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |

  { 0x00,  0x7C,  0x66,  0x66,  0x7C,  0x66,  0x66,  0x7C },    // 66 : B
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |

  { 0x00,  0x3C,  0x66,  0x60,  0x60,  0x60,  0x66,  0x3C },    // 67 : C
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x7C,  0x66,  0x66,  0x66,  0x66,  0x66,  0x7C },    // 68 : D
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |

  { 0x00,  0x7E,  0x60,  0x60,  0x7C,  0x60,  0x60,  0x7E },    // 69 : E
                //              |          |
                //              |  ******  |
                //              |  **      |
                //              |  **      |
                //              |  *****   |
                //              |  **      |
                //              |  **      |
                //              |  ******  |

  { 0x00,  0x7E,  0x60,  0x60,  0x7C,  0x60,  0x60,  0x60 },    // 70 : F
                //              |          |
                //              |  ******  |
                //              |  **      |
                //              |  **      |
                //              |  *****   |
                //              |  **      |
                //              |  **      |
                //              |  **      |

  { 0x00,  0x3C,  0x66,  0x60,  0x60,  0x6E,  0x66,  0x3C },    // 71 : G
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |  **      |
                //              |  ** ***  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x66,  0x66,  0x66,  0x7E,  0x66,  0x66,  0x66 },    // 72 : H
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  ******  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |

  { 0x00,  0x3C,  0x18,  0x18,  0x18,  0x18,  0x18,  0x3C },    // 73 : I
                //              |          |
                //              |   ****   |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |   ****   |

  { 0x00,  0x1E,  0x0C,  0x0C,  0x0C,  0x6C,  0x6C,  0x38 },    // 74 : J
                //              |          |
                //              |    ****  |
                //              |     **   |
                //              |     **   |
                //              |     **   |
                //              |  ** **   |
                //              |  ** **   |
                //              |   ***    |

  { 0x00,  0x66,  0x6C,  0x78,  0x70,  0x78,  0x6C,  0x66 },    // 75 : K
                //              |          |
                //              |  **  **  |
                //              |  ** **   |
                //              |  ****    |
                //              |  ***     |
                //              |  ****    |
                //              |  ** **   |
                //              |  **  **  |

  { 0x00,  0x60,  0x60,  0x60,  0x60,  0x60,  0x60,  0x7E },    // 76 : L
                //              |          |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  ******  |

  { 0x00,  0x63,  0x77,  0x7F,  0x6B,  0x63,  0x63,  0x63 },    // 77 : M
                //              |          |
                //              |  **   ** |
                //              |  *** *** |
                //              |  ******* |
                //              |  ** * ** |
                //              |  **   ** |
                //              |  **   ** |
                //              |  **   ** |

  { 0x00,  0x63,  0x73,  0x7B,  0x6F,  0x67,  0x63,  0x63 },    // 78 : N
                //              |          |
                //              |  **   ** |
                //              |  ***  ** |
                //              |  **** ** |
                //              |  ** **** |
                //              |  **  *** |
                //              |  **   ** |
                //              |  **   ** |

  { 0x00,  0x3C,  0x66,  0x66,  0x66,  0x66,  0x66,  0x3C },    // 79 : O
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x7C,  0x66,  0x66,  0x66,  0x7C,  0x60,  0x60 },    // 80 : P
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
                //              |  **      |
                //              |  **      |

  { 0x00,  0x3C,  0x66,  0x66,  0x66,  0x6E,  0x3C,  0x06 },    // 81 : Q
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  ** ***  |
                //              |   ****   |
                //              |      **  |

  { 0x00,  0x7C,  0x66,  0x66,  0x7C,  0x78,  0x6C,  0x66 },    // 82 : R
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
                //              |  ****    |
                //              |  ** **   |
                //              |  **  **  |

  { 0x00,  0x3C,  0x66,  0x60,  0x3C,  0x06,  0x66,  0x3C },    // 83 : S
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |   ****   |
                //              |      **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x7E,  0x5A,  0x18,  0x18,  0x18,  0x18,  0x18 },    // 84 : T
                //              |          |
                //              |  ******  |
                //              |  * ** *  |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |

  { 0x00,  0x66,  0x66,  0x66,  0x66,  0x66,  0x66,  0x3E },    // 85 : U
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |

  { 0x00,  0x66,  0x66,  0x66,  0x66,  0x66,  0x3C,  0x18 },    // 86 : V
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
                //              |    **    |

  { 0x00,  0x63,  0x63,  0x63,  0x6B,  0x7F,  0x77,  0x63 },    // 87 : W
                //              |          |
                //              |  **   ** |
                //              |  **   ** |
                //              |  **   ** |
                //              |  ** * ** |
                //              |  ******* |
                //              |  *** *** |
                //              |  **   ** |

  { 0x00,  0x63,  0x63,  0x36,  0x1C,  0x36,  0x63,  0x63 },    // 88 : X
                //              |          |
                //              |  **   ** |
                //              |  **   ** |
                //              |   ** **  |
                //              |    ***   |
                //              |   ** **  |
                //              |  **   ** |
                //              |  **   ** |

  { 0x00,  0x66,  0x66,  0x66,  0x3C,  0x18,  0x18,  0x18 },    // 89 : Y
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
                //              |    **    |
                //              |    **    |
                //              |    **    |

  { 0x00,  0x7E,  0x06,  0x0C,  0x18,  0x30,  0x60,  0x7E },    // 90 : Z
                //              |          |
                //              |  ******  |
                //              |      **  |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |  **      |
                //              |  ******  |

  { 0x00,  0x1E,  0x18,  0x18,  0x18,  0x18,  0x18,  0x1E },    // 91 : [
                //              |          |
                //              |    ****  |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    ****  |

  { 0x00,  0x00,  0x60,  0x30,  0x18,  0x0C,  0x06,  0x00 },    // 92 : \
                //              |          |
                //              |          |
                //              |  **      |
                //              |   **     |
                //              |    **    |
                //              |     **   |
                //              |      **  |
                //              |          |

  { 0x00,  0x78,  0x18,  0x18,  0x18,  0x18,  0x18,  0x78 },    // 93 : ]
                //              |          |
                //              |  ****    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |  ****    |

  { 0x00,  0x08,  0x14,  0x22,  0x41,  0x00,  0x00,  0x00 },    // 94 : ^
                //              |          |
                //              |     *    |
                //              |    * *   |
                //              |   *   *  |
                //              |  *     * |
                //              |          |
                //              |          |
                //              |          |

  { 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x7F },    // 95 : _
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |  ******* |

  { 0x00,  0x0C,  0x0C,  0x06,  0x00,  0x00,  0x00,  0x00 },    // 96 : `
                //              |          |
                //              |     **   |
                //              |     **   |
                //              |      **  |
                //              |          |
                //              |          |
                //              |          |
                //              |          |

  { 0x00,  0x00,  0x00,  0x3C,  0x06,  0x3E,  0x66,  0x3E },    // 97 : a
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |      **  |
                //              |   *****  |
                //              |  **  **  |
                //              |   *****  |

  { 0x00,  0x60,  0x60,  0x60,  0x7C,  0x66,  0x66,  0x7C },    // 98 : b
                //              |          |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |

  { 0x00,  0x00,  0x00,  0x3C,  0x66,  0x60,  0x66,  0x3C },    // 99 : c
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **      |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x06,  0x06,  0x06,  0x3E,  0x66,  0x66,  0x3E },    // 100 : d
                //              |          |
                //              |      **  |
                //              |      **  |
                //              |      **  |
                //              |   *****  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |

  { 0x00,  0x00,  0x00,  0x3C,  0x66,  0x7E,  0x60,  0x3C },    // 101 : e
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  ******  |
                //              |  **      |
                //              |   ****   |

  { 0x00,  0x1C,  0x36,  0x30,  0x30,  0x7C,  0x30,  0x30 },    // 102 : f
                //              |          |
                //              |    ***   |
                //              |   ** **  |
                //              |   **     |
                //              |   **     |
                //              |  *****   |
                //              |   **     |
                //              |   **     |

  { 0x00,  0x00,  0x3E,  0x66,  0x66,  0x3E,  0x06,  0x3C },    // 103 : g
                //              |          |
                //              |          |
                //              |   *****  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
                //              |      **  |
                //              |   ****   |

  { 0x00,  0x60,  0x60,  0x60,  0x7C,  0x66,  0x66,  0x66 },    // 104 : h
                //              |          |
                //              |  **      |
                //              |  **      |
                //              |  **      |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |

  { 0x00,  0x00,  0x18,  0x00,  0x18,  0x18,  0x18,  0x3C },    // 105 : i
                //              |          |
                //              |          |
                //              |    **    |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |   ****   |

  { 0x00,  0x0C,  0x00,  0x0C,  0x0C,  0x6C,  0x6C,  0x38 },    // 106 : j
                //              |          |
                //              |     **   |
                //              |          |
                //              |     **   |
                //              |     **   |
                //              |  ** **   |
                //              |  ** **   |
                //              |   ***    |

  { 0x00,  0x60,  0x60,  0x66,  0x6C,  0x78,  0x6C,  0x66 },    // 107 : k
                //              |          |
                //              |  **      |
                //              |  **      |
                //              |  **  **  |
                //              |  ** **   |
                //              |  ****    |
                //              |  ** **   |
                //              |  **  **  |

  { 0x00,  0x18,  0x18,  0x18,  0x18,  0x18,  0x18,  0x18 },    // 108 : l
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |    **    |

  { 0x00,  0x00,  0x00,  0x63,  0x77,  0x7F,  0x6B,  0x6B },    // 109 : m
                //              |          |
                //              |          |
                //              |          |
                //              |  **   ** |
                //              |  *** *** |
                //              |  ******* |
                //              |  ** * ** |
                //              |  ** * ** |

  { 0x00,  0x00,  0x00,  0x7C,  0x7E,  0x66,  0x66,  0x66 },    // 110 : n
                //              |          |
                //              |          |
                //              |          |
                //              |  *****   |
                //              |  ******  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |

  { 0x00,  0x00,  0x00,  0x3C,  0x66,  0x66,  0x66,  0x3C },    // 111 : o
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |

  { 0x00,  0x00,  0x7C,  0x66,  0x66,  0x7C,  0x60,  0x60 },    // 112 : p
                //              |          |
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  *****   |
                //              |  **      |
                //              |  **      |

  { 0x00,  0x00,  0x3C,  0x6C,  0x6C,  0x3C,  0x0D,  0x0F },    // 113 : q
                //              |          |
                //              |          |
                //              |   ****   |
                //              |  ** **   |
                //              |  ** **   |
                //              |   ****   |
                //              |     ** * |
                //              |     **** |

  { 0x00,  0x00,  0x00,  0x7C,  0x66,  0x66,  0x60,  0x60 },    // 114 : r
                //              |          |
                //              |          |
                //              |          |
                //              |  *****   |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **      |
                //              |  **      |

  { 0x00,  0x00,  0x00,  0x3E,  0x40,  0x3C,  0x02,  0x7C },    // 115 : s
                //              |          |
                //              |          |
                //              |          |
                //              |   *****  |
                //              |  *       |
                //              |   ****   |
                //              |       *  |
                //              |  *****   |

  { 0x00,  0x00,  0x18,  0x18,  0x7E,  0x18,  0x18,  0x18 },    // 116 : t
                //              |          |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |  ******  |
                //              |    **    |
                //              |    **    |
                //              |    **    |

  { 0x00,  0x00,  0x00,  0x66,  0x66,  0x66,  0x66,  0x3E },    // 117 : u
                //              |          |
                //              |          |
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |

  { 0x00,  0x00,  0x00,  0x00,  0x66,  0x66,  0x3C,  0x18 },    // 118 : v
                //              |          |
                //              |          |
                //              |          |
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |   ****   |
                //              |    **    |

  { 0x00,  0x00,  0x00,  0x63,  0x6B,  0x6B,  0x6B,  0x3E },    // 119 : w
                //              |          |
                //              |          |
                //              |          |
                //              |  **   ** |
                //              |  ** * ** |
                //              |  ** * ** |
                //              |  ** * ** |
                //              |   *****  |

  { 0x00,  0x00,  0x00,  0x66,  0x3C,  0x18,  0x3C,  0x66 },    // 120 : x
                //              |          |
                //              |          |
                //              |          |
                //              |  **  **  |
                //              |   ****   |
                //              |    **    |
                //              |   ****   |
                //              |  **  **  |

  { 0x00,  0x00,  0x00,  0x66,  0x66,  0x3E,  0x06,  0x3C },    // 121 : y
                //              |          |
                //              |          |
                //              |          |
                //              |  **  **  |
                //              |  **  **  |
                //              |   *****  |
                //              |      **  |
                //              |   ****   |

  { 0x00,  0x00,  0x00,  0x3C,  0x0C,  0x18,  0x30,  0x3C },    // 122 : z
                //              |          |
                //              |          |
                //              |          |
                //              |   ****   |
                //              |     **   |
                //              |    **    |
                //              |   **     |
                //              |   ****   |

  { 0x00,  0x0E,  0x18,  0x18,  0x30,  0x18,  0x18,  0x0E },    // 123 : {
                //              |          |
                //              |     ***  |
                //              |    **    |
                //              |    **    |
                //              |   **     |
                //              |    **    |
                //              |    **    |
                //              |     ***  |

  { 0x00,  0x18,  0x18,  0x18,  0x00,  0x18,  0x18,  0x18 },    // 124 : |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |
                //              |          |
                //              |    **    |
                //              |    **    |
                //              |    **    |

  { 0x00,  0x70,  0x18,  0x18,  0x0C,  0x18,  0x18,  0x70 },    // 125 : }
                //              |          |
                //              |  ***     |
                //              |    **    |
                //              |    **    |
                //              |     **   |
                //              |    **    |
                //              |    **    |
                //              |  ***     |

  { 0x00,  0x00,  0x00,  0x3A,  0x6C,  0x00,  0x00,  0x00 },    // 126 : ~
                //              |          |
                //              |          |
                //              |          |
                //              |   *** *  |
                //              |  ** **   |
                //              |          |
                //              |          |
                //              |          |

  { 0x00,  0x08,  0x1C,  0x36,  0x63,  0x41,  0x41,  0x7F }    // 127 : 
                //              |          |
                //              |     *    |
                //              |    ***   |
                //              |   ** **  |
                //              |  **   ** |
                //              |  *     * |
                //              |  *     * |
                //              |  ******* |


};

void effect_text(char *string, int delayt)
{
    int ltr, dist, rw;

    fill(0x00);
    for (ltr=0; string[ltr]; ltr++) {       // For each letter in string array
        for (dist = 0; dist < 8; dist++) {  // bring letter forward
            int rev = 0;

            fill(0x00);                     // blank cube
            for (rw = 7; rw >= 0; rw--) {   // copy rows
#if 0
                // put this in for normal cube
                cube[rev][dist] = bitswap(font_data[string[ltr]][rw]);
#else
                cube[rev][dist] = font_data[string[ltr]][rw];
                // use above line for backward ass cubes
#endif
                rev++;
            }
            delay_ms(delayt);
        }
    }
}

void effect_text_up(char *string, int delayt)
{
    int ltr, dist, rw;

    fill(0x00);
    for (ltr=0; string[ltr]; ltr++) {       // For each letter in string array
        for (dist = 0; dist < 8; dist++) {  // bring letter forward
            int rev = 0;

            fill(0x00);                     // blank cube
            for (rw = 7; rw >= 0; rw--) {   // copy rows
#if 0
                // put this in for miswired backwards cube
                cube[rev][dist] = bitswap(font_data[string[ltr]][rw]);
#else
                cube[dist][rev] = font_data[string[ltr]][rw];
                // use above line for proper cubes
#endif
                rev++;
            }
            delay_ms(delayt);
        }
    }
}

void int_ripples(int iterations, int delay)
{
	// 16 values for square root of a^2+b^2.  index a*4+b = 10*sqrt
	// This gives the distance to 3.5,3.5 from the point
	unsigned char sqrt_LUT[]={
            49, 43, 38, 35, 43, 35, 29, 26, 38, 29, 21, 16, 35, 25, 16, 7
        };
	unsigned char x, y, height, distance;
	int i;

	for (i=0;i<iterations*4;i+=4)
	{
		fill(0x00);
		for (x=0;x<4;x++)
			for (y=0;y<4;y++)
			{
				// x+y*4 gives no. from 0-15 for sqrt_LUT
				distance=sqrt_LUT[x+y*4];// distance is 0-50 roughly
				// height is sin of distance + iteration*4
				//height = 4 + totty_sin(distance+i) / 52;
				height = (196 + totty_sin(distance+i)) / 49;
				// Use 4-way mirroring to save on calculations
				setvoxel(x,   y,   height);
				setvoxel(7-x, y,   height);
				setvoxel(x,   7-y, height);
				setvoxel(7-x, 7-y, height);
			}
		delay_ms(delay);
	}
}

void side_ripples(int iterations, int delay)
{
	// 16 values for square root of a^2+b^2.  index a*4+b = 10*sqrt
	// This gives the distance to 3.5,3.5 from the point
	unsigned char sqrt_LUT[]={
            49, 43, 38, 35, 43, 35, 29, 26, 38, 29, 21, 16, 35, 25, 16, 7
        };
	unsigned char x, y, height, distance;
	int i;

	for (i=0;i<iterations*4;i+=4)
	{
		fill(0x00);
		for (x=0;x<4;x++)
			for (y=0;y<4;y++)
			{
				// x+y*4 gives no. from 0-15 for sqrt_LUT
				distance=sqrt_LUT[x+y*4];// distance is 0-50 roughly
				// height is sin of distance + iteration*4
				//height = 4 + totty_sin(distance+i) / 52;
				height = (196 + totty_sin(distance+i)) / 49;
				// Use 4-way mirroring to save on calculations
				setvoxel(x,   height,   y);
				setvoxel(7-x, height,   y);
				setvoxel(x,   height,   7-y);
				setvoxel(7-x, height,   7-y);
				setvoxel(x,   7-height, y);
				setvoxel(7-x, 7-height, y);
				setvoxel(x,   7-height, 7-y);
				setvoxel(7-x, 7-height, 7-y);
			}
		delay_ms(delay);
	}
}

void mirror_ripples(int iterations, int delay)
{
	// 16 values for square root of a^2+b^2.  index a*4+b = 10*sqrt
	// This gives the distance to 3.5,3.5 from the point
	unsigned char sqrt_LUT[]={
            49, 43, 38, 35, 43, 35, 29, 26, 38, 29, 21, 16, 35, 25, 16, 7
        };
	unsigned char x, y, height, distance;
	int i;

	for (i=0;i<iterations*4;i+=4)
	{
		fill(0x00);
		for (x=0;x<4;x++)
			for (y=0;y<4;y++)
			{
				// x+y*4 gives no. from 0-15 for sqrt_LUT
				distance=sqrt_LUT[x+y*4];// distance is 0-50 roughly
				// height is sin of distance + iteration*4
				//height = 4 + totty_sin(distance+i) / 52;
				height = (196 + totty_sin(distance+i)) / 49;
				// Use 4-way mirroring to save on calculations
				setvoxel(x,   y,   height);
				setvoxel(7-x, y,   height);
				setvoxel(x,   7-y, height);
				setvoxel(7-x, 7-y, height);
				setvoxel(x,   y,   7-height);
				setvoxel(7-x, y,   7-height);
				setvoxel(x,   7-y, 7-height);
				setvoxel(7-x, 7-y, 7-height);
			}
		delay_ms(delay);
	}
}

void zoom_pyramid_clear()
{
  //1

  box_walls(0, 0, 0, 7, 0, 7);
  delay_ms(250);

  //2

  //Pyramid
    box_wireframe(0, 0, 0, 7, 0, 1);

   clrplane_y(0);
  delay_ms(250);

  //3

  //Pyramid
     clrplane_y(1);
  box_walls(0, 2, 0, 7, 2, 7);
  delay_ms(250);

  //4

  //Pyramid
     clrplane_y(2);
  box_walls(0, 3, 0, 7, 3, 7);
  delay_ms(250);

  //5

  //Pyramid
     clrplane_y(3);
  box_walls(0, 4, 0, 7, 4, 7);
  delay_ms(250);

  //5

  //Pyramid

     clrplane_y(4);
  box_walls(0, 5, 0, 7, 5, 7);
  delay_ms(250);
  //6


  //Pyramid

     clrplane_y(5);
      box_walls(0, 6, 0, 7, 6, 7);
  delay_ms(250);
  //7

  //Pyramid

  clrplane_y(6);
  box_walls(0, 7, 0, 7, 7, 7);
  delay_ms(250);

  clrplane_y(7);
  delay_ms(5000);
}

void zoom_pyramid()
{
    int i, j, k, time;

  //1
  fill(0x00);

  box_walls(0, 0, 0, 7, 0, 7);
  delay_ms(250);

  //2
  fill(0x00);
  //Pyramid
    box_wireframe(0, 0, 0, 7, 0, 1);

   box_walls(0, 1, 0, 7, 1, 7);
  delay_ms(250);

  //3
  fill(0x00);
  //Pyramid
    box_wireframe(0, 0, 0, 7, 1, 1);
    box_wireframe(1, 1, 2, 6, 1, 3);

  box_walls(0, 2, 0, 7, 2, 7);
  delay_ms(250);

  //4
  fill(0x00);
  //Pyramid
    box_wireframe(0, 0, 0, 7, 2, 1);
    box_wireframe(1, 1, 2, 6, 2, 3);
    box_wireframe(2, 2, 4, 5, 2, 5);

  box_walls(0, 3, 0, 7, 3, 7);
  delay_ms(250);

  //5
  fill(0x00);
  //Pyramid
    box_wireframe(0, 0, 0, 7, 3, 1);
    box_wireframe(1, 1, 2, 6, 3, 3);
    box_wireframe(2, 2, 4, 5, 3, 5);
    box_wireframe(3, 3, 6, 4, 3, 7);

  box_walls(0, 4, 0, 7, 4, 7);
  delay_ms(250);

  //5
  fill(0x00);
  //Pyramid
    box_wireframe(0, 0, 0, 7, 4, 1);
    box_wireframe(1, 1, 2, 6, 4, 3);
    box_wireframe(2, 2, 4, 5, 4, 5);
    box_wireframe(3, 3, 6, 4, 4, 7);

  box_walls(0, 5, 0, 7, 5, 7);
  delay_ms(250);
  //6

  fill(0x00);
  //Pyramid
    box_wireframe(0, 0, 0, 7, 5, 1);
    box_wireframe(1, 1, 2, 6, 5, 3);
    box_wireframe(2, 2, 4, 5, 5, 5);
    box_wireframe(3, 3, 6, 4, 4, 7);

      box_walls(0, 6, 0, 7, 6, 7);
  delay_ms(250);
  //7
  fill(0x00);
  //Pyramid
    box_wireframe(0, 0, 0, 7, 6, 1);
    box_wireframe(1, 1, 2, 6, 6, 3);
    box_wireframe(2, 2, 4, 5, 5, 5);
    box_wireframe(3, 3, 6, 4, 4, 7);

  box_walls(0, 7, 0, 7, 7, 7);
  delay_ms(250);

  fill(0x00);
  box_wireframe(0, 0, 0, 7, 7, 1);
  box_wireframe(1, 1, 2, 6, 6, 3);
  box_wireframe(2, 2, 4, 5, 5, 5);
  box_wireframe(3, 3, 6, 4, 4, 7);

  delay_ms(5000);
}

void effect_intro()
{
    int cnt, cnt_2, time;

    // Bottom To Top

    for (cnt=0; cnt<=7; cnt++) {
        box_wireframe(0, 0, 0, 7, 7, cnt);
        delay_ms(300);
    }
    for (cnt=0; cnt<7; cnt++) {
        clrplane_z(cnt);
        delay_ms(300);
    }

    // Shift Things Right
    // 1
    shift(AXIS_Y, -1);
    for (cnt=0; cnt<=7; cnt++) {
        setvoxel(cnt, 0, 6);
    }
    delay_ms(300);
    // 2
    shift(AXIS_Y, -1);
    for (cnt=0; cnt<=7; cnt++) {
        setvoxel(cnt, 0, 5);
    }
    setvoxel(0, 0, 6);
    setvoxel(7, 0, 6);
    delay_ms(300);
    // 3
    shift(AXIS_Y, -1);
    for (cnt=0; cnt<=7; cnt++) {
        setvoxel(cnt, 0, 4);
    }
    setvoxel(0, 0, 5);
    setvoxel(7, 0, 5);
    setvoxel(0, 0, 6);
    setvoxel(7, 0, 6);
    delay_ms(300);

    // 4
    shift(AXIS_Y, -1);
    for (cnt=0; cnt<=7; cnt++) {
        setvoxel(cnt, 0, 3);
    }
    setvoxel(0, 0 ,4);
    setvoxel(7, 0, 4);
    setvoxel(0, 0, 5);
    setvoxel(7, 0, 5);
    setvoxel(0, 0, 6);
    setvoxel(7, 0, 6);
    delay_ms(300);

    // 5
    shift(AXIS_Y, -1);
    for(cnt=0; cnt<=7; cnt++) {
        setvoxel(cnt, 0, 2);
    }
    setvoxel(0, 0, 3);
    setvoxel(7, 0, 3);
    setvoxel(0, 0, 4);
    setvoxel(7, 0, 4);
    setvoxel(0, 0, 5);
    setvoxel(7, 0, 5);
    setvoxel(0, 0, 6);
    setvoxel(7, 0, 6);
    delay_ms(300);

    // 6
    shift(AXIS_Y, -1);
    for(cnt=0; cnt<=7; cnt++) {
        setvoxel(cnt, 0, 1);
    }
    setvoxel(0, 0, 2);
    setvoxel(7, 0, 2);
    setvoxel(0, 0, 3);
    setvoxel(7, 0, 3);
    setvoxel(0, 0, 4);
    setvoxel(7, 0, 4);
    setvoxel(0, 0, 5);
    setvoxel(7, 0, 5);
    delay_ms(300);

    // 7
    shift(AXIS_Y, -1);
    for(cnt=0;cnt<=7;cnt++){
        setvoxel(cnt, 0, 0);
    }
    setvoxel(0, 0, 1);
    setvoxel(7, 0, 1);
    setvoxel(0, 0, 2);
    setvoxel(7, 0, 2);
    setvoxel(0, 0, 3);
    setvoxel(7, 0, 3);
    setvoxel(0, 0, 4);
    setvoxel(7, 0, 4);
    setvoxel(0, 0, 5);
    setvoxel(7, 0, 5);
    delay_ms(300);

    // Right To Left
    for (cnt=0; cnt<=7; cnt++) {
        box_wireframe(0, 0, 0, 7, cnt, 7);
        delay_ms(300);
    }
    for (cnt=0; cnt<7; cnt++) {
        clrplane_y(cnt);
        delay_ms(300);
    }

    // Shift to the bottom
    for (cnt_2=6; cnt_2>=0; cnt_2--) {
        shift(AXIS_Z, -1);
        for (cnt=0; cnt<=7; cnt++) {
            setvoxel(cnt, cnt_2, 0);
        }
        for (cnt=6; cnt>cnt_2; cnt--) {
            setvoxel(0, cnt, 0);
            setvoxel(7, cnt, 0);
        }
        delay_ms(300);
    }

    // Make All Wall Box

    for (cnt=0; cnt<=6; cnt++) {
        fill(0x00);
        box_walls(0, 0, 0, 7, 7, cnt);
        delay_ms(300);
    }

    time = 500;
    for (cnt_2=0; cnt_2<5; cnt_2++) {
        time -= 75;
        // Make Box Smaller
        for (cnt=0; cnt<=3; cnt++) {
            fill(0x00);
            box_walls(cnt, cnt, cnt, 7-cnt, 7-cnt, 7-cnt);
            delay_ms(time);
        }

        // Make Box Bigger
        for (cnt=0; cnt<=3; cnt++) {
            fill(0x00);
            box_walls(3-cnt, 3-cnt, 3-cnt, 4+cnt, 4+cnt, 4+cnt);
            delay_ms(time);
        }
    }
    for (cnt_2=0; cnt_2<5; cnt_2++) {
        time += 75;
        // Make Box Smaller
        for (cnt=0; cnt<=3; cnt++) {
            fill(0x00);
            box_walls(cnt, cnt, cnt, 7-cnt, 7-cnt, 7-cnt);
            delay_ms(time);
        }

        // Make Box Bigger
        for (cnt=0; cnt<=3; cnt++) {
            fill(0x00);
            box_walls(3-cnt, 3-cnt, 3-cnt, 4+cnt, 4+cnt, 4+cnt);
            delay_ms(time);
        }
    }
    delay_ms(500);
}

// ******************************************
// 3D addins ********************************
// ******************************************
#if 0
void linespin(int iterations, int delay)
{
	float top_x, top_y, top_z, bot_x, bot_y, bot_z, sin_base;
	float center_x, center_y;

	center_x = 4;
	center_y = 4;

	int i, z;
	for (i=0;i<iterations;i++)
	{

		//printf("Sin base %f \n", sin_base);

		for (z = 0; z < 8; z++)
		{

		sin_base = (float)i/50 + (float)z/(10 + (7*sin((float)i / 200)));

		top_x = center_x + sin(sin_base) * 5;
		top_y = center_x + cos(sin_base) * 5;
		//top_z = center_x + cos(sin_base/100) * 2.5;

		bot_x = center_x + sin(sin_base+3.14) * 10;
		bot_y = center_x + cos(sin_base+3.14) * 10;
		//bot_z = 7-top_z;

		bot_z = z;
		top_z = z;

		// setvoxel((int) top_x, (int) top_y, 7);
		// setvoxel((int) bot_x, (int) bot_y, 0);

		//printf("P1: %i %i %i P2: %i %i %i \n", (int) top_x, (int) top_y, 7, (int) bot_x, (int) bot_y, 0);

		//line_3d((int) top_x, (int) top_y, (int) top_z, (int) bot_x, (int) bot_y, (int) bot_z);
		line_3d((int) top_z, (int) top_x, (int) top_y, (int) bot_z, (int) bot_x, (int) bot_y);
		}

		// delay_ms(delay);
		fill(0x00);
	}

}
#endif

// circle, len 16, offset 28
const unsigned char paths[44] = {
    0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,
    0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x71,
    0x72,0x73,0x74,0x75,0x76,0x77,0x67,0x57,
    0x47,0x37,0x27,0x17,0x04,0x03,0x12,0x21,
    0x30,0x40,0x51,0x62,0x73,0x74,0x65,0x56,
    0x47,0x37,0x26,0x15
};

void font_getpath(unsigned char path, unsigned char *destination, int length)
{
	int i;
	int offset = 0;

	if (path == 1)
		offset = 28;

	for (i = 0; i < length; i++)
		destination[i] = paths[i+offset];
}

// ************************************************************

void effect_pathmove(unsigned char *path, int length)
{
	int i, z;
	unsigned char state;

	for (i=(length-1);i>=1;i--)
	{
		for (z=0;z<8;z++)
		{

			state = getvoxel(((path[(i-1)]>>4) & 0x0f), (path[(i-1)] & 0x0f), z);
			altervoxel(((path[i]>>4) & 0x0f), (path[i] & 0x0f), z, state);
		}
	}
	for (i=0;i<8;i++)
		clrvoxel(((path[0]>>4) & 0x0f), (path[0] & 0x0f), i);
}

void effect_rand_patharound(int iterations, int delay)
{
	int z, dz, i;
	z = 4;
	unsigned char path[28];

	font_getpath(0, path, 28);

	for (i = 0; i < iterations; i++)
	{
		dz = ((rand()%3)-1);
		z += dz;

		if (z>7)
			z = 7;

		if (z<0)
			z = 0;

		effect_pathmove(path, 28);
		setvoxel(0, 7, z);
		delay_ms(delay);
	}
}

void main()
{
    int i, x, y, z, m, n;

    gpio_init();
    gpio_ext(1);

    for (y=0; y<8; y++) {
        for (x=0; x<8; x++) {
            for (z=0; z<8; z++) {
                setvoxel(x, y, z);
            }

            delay_ms(20);

            for (z=0; z<8; z++) {
                clrvoxel(x, y, z);
            }
        }
    }
    for (z=0; z<8; z++) {
        for (y=0; y<8; y++) {
            for (x=0; x<8; x++) {
                setvoxel(x, y, z);
                delay_ms(5);
            }
        }
    }
    fill(0);

    for (;;) {
        printf("intro\n");
        effect_intro();

        printf("wormsqueeze\n");
        effect_wormsqueeze(2, AXIS_Z, -1, 100, 500);

        printf("pyramid\n");
        zoom_pyramid();
        zoom_pyramid_clear();

        printf("text\n");
        effect_text("MIPS RETROBSD", 600);
        //sinelines(4000, 10);
        //linespin(1500, 10);

        printf("planboing\n");
        effect_planboing(AXIS_Z, 450);
        effect_planboing(AXIS_Y, 450);
        effect_planboing(AXIS_X, 450);

        printf("blinky2\n");
        effect_blinky2();

        printf("mirror ripples\n");
        mirror_ripples(600, 200);

        printf("axis_updown_randsuspend\n");
        effect_axis_updown_randsuspend(AXIS_Z, 250, 2500, 0);
        effect_axis_updown_randsuspend(AXIS_Z, 250, 2500, 1);
        effect_axis_updown_randsuspend(AXIS_Z, 250, 2500, 0);
        effect_axis_updown_randsuspend(AXIS_Z, 250, 2500, 1);
        effect_axis_updown_randsuspend(AXIS_X, 250, 2500, 0);
        effect_axis_updown_randsuspend(AXIS_X, 250, 2500, 1);
        effect_axis_updown_randsuspend(AXIS_Y, 250, 2500, 0);
        effect_axis_updown_randsuspend(AXIS_Y, 250, 2500, 1);

        printf("rand_patharound\n");
        effect_rand_patharound(200, 250);

        printf("fireworks\n");
        fireworks(10, 250);

        //printf("random_filler\n");
        //effect_random_filler(35, 1);
        //effect_random_filler(35, 0);

        printf("rain\n");
        effect_rain(100);

        printf("side ripples\n");
        side_ripples(300, 250);

        printf("text up\n");
        effect_text_up("MIPS RETROBSD", 600);

        printf("random sparkle\n");
        effect_random_sparkle();

        printf("quad ripples\n");
        quad_ripples(600, 150);

        printf("boxside_randsend_parallel\n");
        effect_boxside_randsend_parallel(AXIS_X, 0, 70, 1);
        effect_boxside_randsend_parallel(AXIS_X, 1, 70, 1);
        effect_boxside_randsend_parallel(AXIS_Y, 0, 70, 1);
        effect_boxside_randsend_parallel(AXIS_Y, 1, 70, 1);
        effect_boxside_randsend_parallel(AXIS_Z, 0, 70, 1);
        effect_boxside_randsend_parallel(AXIS_Z, 1, 70, 1);
    }
}
