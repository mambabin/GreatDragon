#include <IL/il.h>
#include <string>
#include <cassert>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;


static ILuint GenSingleImage(){
	ILuint image;
	ilGenImages(1, &image);
	return image;
}

static void DelSingleImage(ILuint image){
	ilDeleteImages(1, &image);
}

static void SwapPixel(unsigned char *begin, int size){
	unsigned char pixel[4];
	for(int i = 0; i < size; i++)
		pixel[i] = *(begin + i);
	for(int i = 0; i < size; i++)
		begin[i] = pixel[size - 1 - i];
}

static void Resize(const string &src, int newWidth, int newHeight, const string &out){
	ILuint image = GenSingleImage();
	ilBindImage(image);

	if(ilLoadImage(src.c_str()) != IL_TRUE){
		printf("Failed to load src.\n");
		DelSingleImage(image);
		return;
	}

	ILint bpp = ilGetInteger(IL_IMAGE_BPP);
	if(bpp != 3){
		printf("Only support 3 channels.\n");
		DelSingleImage(image);
		return;
	}

	ILint width = ilGetInteger(IL_IMAGE_WIDTH);
	ILint height = ilGetInteger(IL_IMAGE_HEIGHT);

	int len = width * height * bpp;
	unsigned char *buffer = (unsigned char *)malloc(len);
	if(buffer == NULL){
		printf("Out of memory.\n");
		DelSingleImage(image);
		return;
	}

	ILubyte *data = ilGetData();
	memcpy(buffer, data, len);

	DelSingleImage(image);

	image = GenSingleImage();
	ilBindImage(image);

	int newLen = newWidth * newHeight * bpp;
	unsigned char *newBuffer = (unsigned char *)malloc(newLen);
	if(buffer == NULL){
		printf("Out of memory.\n");
		DelSingleImage(image);
		free(buffer);
		return;
	}

	float widthFactor = (float)width / (float)newWidth;
	float heightFactor = (float)height / (float)newHeight;
	int pos = 0;
	int totalHeight = 0;
	for(float i = 0.0f; i < height; i += heightFactor){
		if(totalHeight >= newHeight)
			break;

		for(float j = 0.0f; j < width; j += widthFactor){
			int k;
			for(k = 0; k < bpp; k++){
				if(pos >= newLen)
					break;
				newBuffer[pos++] = buffer[(int)i * width * bpp + (int)j * bpp + k];
			}
			if(k >= bpp)
				SwapPixel(newBuffer + pos - 3, 3);
		}

		totalHeight++;
		int total = newWidth * bpp * totalHeight;
		if(pos < total){
			while(pos < total)
				newBuffer[pos++] = 0;
		}
		else if(pos > total){
			pos = total;
		}
	}
	if(pos < newLen){
		while(pos < newLen)
			newBuffer[pos++] = 0;
	}

	ilTexImage(newWidth, newHeight, 1, bpp, IL_RGB, IL_UNSIGNED_BYTE, newBuffer);

	ilEnable(IL_FILE_OVERWRITE);
	ilSaveImage(out.c_str());

	DelSingleImage(image);
	free(buffer);
	free(newBuffer);
}

static void GenNav(const string &src, const string &out){
	ILuint image = GenSingleImage();
	ilBindImage(image);

	if(ilLoadImage(src.c_str()) != IL_TRUE){
		printf("Failed to load src.\n");
		DelSingleImage(image);
		return;
	}

	ILint bpp = ilGetInteger(IL_IMAGE_BPP);
	if(bpp != 3){
		printf("Only support 3 channels.\n");
		DelSingleImage(image);
		return;
	}

	ILint width = ilGetInteger(IL_IMAGE_WIDTH);
	ILint height = ilGetInteger(IL_IMAGE_HEIGHT);

	int len = width * height * bpp;
	unsigned char *buffer = (unsigned char *)malloc(len);
	if(buffer == NULL){
		printf("Out of memory.\n");
		DelSingleImage(image);
		return;
	}

	ILubyte *data = ilGetData();
	memcpy(buffer, data, len);

	DelSingleImage(image);

	image = GenSingleImage();
	ilBindImage(image);

	int newLen = width * height * 4;
	unsigned char *newBuffer = (unsigned char *)malloc(newLen);
	if(buffer == NULL){
		printf("Out of memory.\n");
		DelSingleImage(image);
		free(buffer);
		return;
	}

	int pos = 0;
	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			newBuffer[pos++] = 0;
			for(int k = 0; k < bpp; k++)
				newBuffer[pos++] = buffer[(int)i * width * bpp + (int)j * bpp + k];
			if(newBuffer[pos - 1] == 255){
				newBuffer[pos - 1] = 224;
				newBuffer[pos - 2] = 149;
				newBuffer[pos - 3] = 3;
				newBuffer[pos - 4] = 255;
			}
			SwapPixel(newBuffer + pos - 4, 4);
		}
	}

	ilTexImage(width, height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, newBuffer);

	ilEnable(IL_FILE_OVERWRITE);
	ilSaveImage(out.c_str());

	DelSingleImage(image);
	free(buffer);
	free(newBuffer);
}

static void ExtraName(const string &full, string *name){
	size_t pos = full.find_last_of(".");
	if(pos == string::npos)
		*name = full;
	else
		*name = full.substr(0, pos);
}

static void ShowHelp(const char *me){
	printf("Usage: %s src width height\n", me);
}

int main(int argc, char *argv[]){
	if(argc < 4){
		ShowHelp(argv[0]);
		return 0;
	}

	const char *src = argv[1];
	int width = atoi(argv[2]);
	int height = atoi(argv[3]);
	if(width <= 0 || height <= 0){
		ShowHelp(argv[0]);
		return 0;
	}

	ilInit();

	string resizeName = "Resize-" + string(src);
	Resize(src, width, height, resizeName);

	string name;
	ExtraName(resizeName, &name);
	GenNav(resizeName, name + ".png");

	return 0;
}
