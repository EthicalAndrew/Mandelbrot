#include <iostream>     
#include <thread>         
#include <mutex> 
#include <chrono>
#include <complex>
#include <fstream> //this does something
#include <condition_variable>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::complex;
using std::ofstream;

//Define the alias "the_clock" for the clock type to use.
typedef std::chrono::steady_clock the_clock;
//The size of the image to generate.
const int WIDTH = 1920;
const int HEIGHT = 1200;
//The number of times to iterate before a point isn't in the Mandelbrot set.
const int MAX_ITERATIONS = 500;
//The image data.
//Shared between threads **
uint32_t image[HEIGHT][WIDTH];
//Check if threads are ready to pass over

//Write the image to a TGA file
void write_tga() {
	std::string filename = "output.tga";
	ofstream outfile(filename, ofstream::binary);

	uint8_t header[18] = {
		0, //no image ID
		0, //no colour map
		2, //uncompressed 24-bit image
		0, 0, 0, 0, 0, //empty colour map specification
		0, 0, //X origin
		0, 0, //Y origin
		WIDTH & 0xFF, (WIDTH >> 8) & 0xFF, //width
		HEIGHT & 0xFF, (HEIGHT >> 8) & 0xFF, //height
		24, //bits per pixel
		0, //image descriptor
	};

	outfile.write((const char*)header, 18);

	for (int y = 0; y < HEIGHT; ++y) {
		for (int x = 0; x < WIDTH; ++x) {
			uint8_t pixel[3] = {
				image[y][x] & 0xFF, //blue channel
				(image[y][x] >> 8) & 0xFF, //green channel
				(image[y][x] >> 16) & 0xFF, //red channel
			};
			outfile.write((const char*)pixel, 3);
		}
	}

	outfile.close();
	if (!outfile) {
		//An error has occurred at some point.
		std::cout << "Error writing to " << filename << std::endl;
		exit(1);
	}
}

void Calc(int yStart, int yEnd) {
	double left = -2.0;
	double right = 1.0;
	double top = 1.125;
	double bottom = -1.125;

	for (int y = yStart; y < yEnd; ++y) {
		for (int x = 0; x < WIDTH; ++x) {

			// Work out the point in the complex plane that corresponds to this pixel in the output image.
			complex<double> c(left + (x * (right - left) / WIDTH), top + (y * (bottom - top) / HEIGHT));

			//Start off z at (0, 0).
			complex<double> z(0.0, 0.0);

			//Iterate z = z^2 + c until z moves more than 2 units away from (0, 0), or iterated too many times.
			int iterations = 0;
			while (abs(z) < 2.0 && iterations < MAX_ITERATIONS) {
				z = (z * z) + c;

				++iterations;
			}

			ptr->lock();

			if (iterations == MAX_ITERATIONS) {
				//z didn't escape from the circle.
				//This point is in the Mandelbrot set.
				image[y][x] = 0x2f77d0; // Blue
			}

			else {
				//z escaped within less than MAX_ITERATIONS
				//iterations. This point isn't in the set.
				image[y][x] = 0x000000; // black/
			}

			ptr->unlock();
		}
	}
}

int main() {
	std::cout << "Computing..." << std::endl;

	the_clock::time_point start = the_clock::now();
	{
		std::thread a(Calc, 0, 201);
		std::thread b(Calc, 201, 401);
		std::thread c(Calc, 401, 601);
		std::thread d(Calc, 601, 801);
		std::thread e(Calc, 801, 1001);
		std::thread f(Calc, 1001, 1200);
		a.join();
		b.join();
		c.join();
		d.join();
		e.join();
		f.join();
	}
	//Synchronize threads: Meaning they all finish then send results

	//Collects data and combines it.

	the_clock::time_point end = the_clock::now();

	//Compute the difference between the two times in milliseconds
	auto time_taken = duration_cast<milliseconds>(end - start).count();
	std::cout << "Computing the Mandelbrot set took " << time_taken << " ms." << std::endl;

	std::thread a(write_tga);
	a.join();

	return 0;
}