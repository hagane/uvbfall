#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h> 
#include <fftw3.h>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

uint16_t *raw;
fftw_complex *in,
			 *out;
fftw_plan plan;

struct pixel
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct pixel* tex_ptr;

int len, res_x, res_y;

// LOL, state machine :3
enum {
	NEED_NOTHING,
	NEED_DRAW,
	NEED_PROCESS
} need;

void draw() {
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,
			len/2,len/2,0,
			GL_RGB,
			GL_UNSIGNED_BYTE, tex_ptr);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0,0.0); glVertex2f(0.0,0.0);
		glTexCoord2f(1.0,0.0); glVertex2f(res_x,0.0);
		glTexCoord2f(1.0,1.0); glVertex2f(res_x,res_y);
		glTexCoord2f(0.0,1.0); glVertex2f(0.0,res_y);
	glEnd();
	SDL_GL_SwapBuffers();

	need = NEED_NOTHING;
}

uint8_t init_opengl(int res_x, int res_y)
{
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != -1)	{
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		
		if(SDL_SetVideoMode(res_x, res_y, 32, SDL_ANYFORMAT|SDL_OPENGL))
		{
			glEnable(GL_TEXTURE_2D);
			glViewport(0,0,res_x,res_y);
			gluOrtho2D(0,res_x,0,res_y);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			return 1;
		}
	}
	return 0;
}

void process() {
	float val;

	if (!fread(raw, sizeof(uint16_t), len, stdin))
		exit(1);
	for (int i = 0; i < len; i++)
		in[i] = ((float)raw[i] / 65535.0) - 1;

	fftw_execute(plan);

	memmove(tex_ptr+res_x, tex_ptr, res_x*(res_y-1)*sizeof(struct pixel));
	tex_ptr[0].r = 0;
	tex_ptr[0].g = 0;
	tex_ptr[0].b = 0;
	for (int i = 1; i < len/2; i++) {
		val = cabs(out[i] + out[len-i]) / (float) len * 8;
		if (val < 0.25) {
			tex_ptr[i].r = val * 4 * 255;
			tex_ptr[i].g = tex_ptr[i].b = 0;
		}
		else if (val < .5) {
			tex_ptr[i].r = 255;
			tex_ptr[i].g = (val-0.25) * 4 * 255;
			tex_ptr[i].b = 0;
		}
		else {
			tex_ptr[i].r = tex_ptr[i].g = 255;
			tex_ptr[i].b = (val - 0.5) * 4 * 255;
		}
	}

	need = NEED_DRAW;
}

uint32_t timer_func(uint32_t interval, void *param) {
	need = NEED_PROCESS;
	return interval;
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
			printf("Usage: %s <DFT length> <bitrate> [-xNNN -yNNN -bNNN]\n", argv[0]);
			return -1;
	}
	len = atoi(argv[1]);
	for(int a = 1; a < argc; a++)
	{
		if(((char*)argv[a])[0] == '-')
		{
			char dim;
			int res;
			sscanf((char*)argv[a],"-%c%u",&dim,&res);
			switch(dim)
			{
				case 'x': res_x = res; break;
				case 'y': res_y = res; break;
				default: printf("Unknown parameter: %c", dim);
						 return -2;
			}
		}
	}

	if((res_x == 0) && (res_y == 0))
	{
		res_x = res_y = len/2;
	}

	if((res_x == 0) || (res_y == 0))
	{
		puts("Only one dimension specified.");
		return -3;
	}

	raw = calloc(len, sizeof(uint16_t));
	in = fftw_malloc(sizeof(fftw_complex) * len);
	out = fftw_malloc(sizeof(fftw_complex) * len);
	plan = fftw_plan_dft_1d(len, in, out, FFTW_FORWARD, FFTW_MEASURE);
	tex_ptr = calloc(len * len /2, sizeof(struct pixel));

	if(!init_opengl(res_x,res_y))
	{
		puts("OpenGL init failed.");
		return -4;
	}

	int delay = len/atof(argv[2]) * 1000;
	printf("Delay %i\n", delay);
	SDL_AddTimer(delay, timer_func, NULL);
	SDL_Event ev;
	while(1) {
		if(SDL_PollEvent(&ev) != 0)	{
			if (ev.type == SDL_QUIT) {
				puts("SDL_QUIT received. Shutting down.");
				break;
			}
		}
		switch (need) {
			case NEED_PROCESS: process(); break;
			case NEED_DRAW: draw(); break;
			default: ;
		}
		// let other processeses do stuff
		SDL_Delay(1);
	}

	fftw_destroy_plan(plan);
	fftw_free(in);
	fftw_free(out);
	free(raw);
	free(tex_ptr);
}

