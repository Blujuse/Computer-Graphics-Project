#pragma once
#include <vector>

struct Color {
	uint8_t r, g, b, a;
};


void setPixel(std::vector<uint8_t>& image, int x, int y, int width, int height, const Color& color)
{
	int pixelIdx = x + y * width;
	image[pixelIdx * 4 + 0] = color.r;
	image[pixelIdx * 4 + 1] = color.g;
	image[pixelIdx * 4 + 2] = color.b;
	image[pixelIdx * 4 + 3] = color.a;
}

void drawCircle(std::vector<uint8_t>& image, int width, int height, int r, int c, int radius, const Color& color) {
	if (r < 0 || r > height || c < 0 || c > height) return;
	int minC = std::max(c - radius, 0);
	int minR = std::max(r - radius, 0);
	int maxC = std::min(c + radius, width);
	int maxR = std::min(r + radius, height);

	int radSq = radius * radius;

	for(int ir = minR; ir < maxR; ++ir)
		for (int ic = minC; ic < maxC; ++ic) {
			int distSq = (ic-c)*(ic-c) + (ir-r)*(ir-r);
			if (distSq <= radSq) {
				setPixel(image, ic, ir, width, height, color);
			}
		}
}

