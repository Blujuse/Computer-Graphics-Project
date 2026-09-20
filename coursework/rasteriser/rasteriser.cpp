// This define is necessary to get the M_PI constant.
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>
#include "Shading.hpp"
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"
#include "Mesh.hpp"

enum ShadingMode 
{
	PHONG,
	BLINN_PHONG
};

struct Triangle {
	std::array<Eigen::Vector3f, 3> screen; // Coordinates of the triangle in screen space.
	std::array<Eigen::Vector3f, 3> verts; // Vertices of the triangle in world space.
	std::array<Eigen::Vector3f, 3> cam; // Vertices of the triangle in camera space.
	std::array<Eigen::Vector3f, 3> norms; // Normals of the triangle corners in world space.
	std::array<Eigen::Vector2f, 3> texs; // Texture coordinates of the triangle corners.
};


Eigen::Matrix4f projectionMatrix(int height, int width, float horzFov = 70.f * M_PI / 180.f, float zFar = 10.f, float zNear = 0.1f)
{
	// Make a projection matrix following the formulation in the lecture slides, and using the provided parameters.
	// First, work out vertical FoV based on the horizontal FoV:
	float aspectRatio = float(width) / height;
	double vertFov = horzFov / aspectRatio;
	// Now construct the matrix.
	Eigen::Matrix4f projection;
	double tanV = 1 / tan(vertFov / 2.0f);
	double tanH = tanV / aspectRatio;
	double perspOne = zFar / (zFar - zNear);
	double perspTwo = -zFar * zNear / (zFar - zNear);

	projection <<
		tanH, 0, 0, 0,
		0, tanV, 0, 0,
		0, 0, perspOne, perspTwo,
		0, 0, 1, 0;

	return projection;
}

void findScreenBoundingBox(const Triangle& t, int width, int height, int& minX, int& minY, int& maxX, int& maxY)
{
	// Find a bounding box around the triangle
	minX = std::min(std::min(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	minY = std::min(std::min(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());
	maxX = std::max(std::max(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	maxY = std::max(std::max(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());

	// Constrain it to lie within the image.
	minX = std::min(std::max(minX, 0), width - 1);
	maxX = std::min(std::max(maxX, 0), width - 1);
	minY = std::min(std::max(minY, 0), height - 1);
	maxY = std::min(std::max(maxY, 0), height - 1);
}


void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	std::vector<float>& zBuffer,
	const Triangle& t,
	const std::vector<std::unique_ptr<Light>>& lights,
	const std::vector<uint8_t>& albedoTexture, int texWidth, int texHeight,
	const Eigen::Vector3f& specularColor,
	float specularExponent,
	ShadingMode shadingMode,
	const Eigen::Vector3f& camWorldPos)
{
	int minX, minY, maxX, maxY;
	findScreenBoundingBox(t, width, height, minX, minY, maxX, maxY);

	Eigen::Vector2f edge1 = v2(t.screen[2] - t.screen[0]);
	Eigen::Vector2f edge2 = v2(t.screen[1] - t.screen[0]);
	float triangleArea = 0.5f * vec2Cross(edge2, edge1);
	if (triangleArea < 0) {
		// Triangle is backfacing
		// Exit and quit drawing!
		return;
	}

	for (int x = minX; x <= maxX; ++x)
		for (int y = minY; y <= maxY; ++y) {
			Eigen::Vector2f p(x, y);

			// Find sub-triangle areas
			float a0 = 0.5f * fabsf(vec2Cross(v2(t.screen[1]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a1 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a2 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[1]), p - v2(t.screen[1])));

			// find barycentrics
			float b0 = a0 / triangleArea;
			float b1 = a1 / triangleArea;
			float b2 = a2 / triangleArea;

			// If outside triangle, exit early
			float sum = b0 + b1 + b2;
			if (sum > 1.0001) {
				continue;
			}

			// Get the depths from the camera-space position of the 3 corners.
			float depth0 = t.cam[0].z(), depth1 = t.cam[1].z(), depth2 = t.cam[2].z();

			// Work out the depth at the point P
			float depthP = 1.0f / (b0 / depth0 + b1 / depth1 + b2 / depth2);

			// Interpolate to find the world-space position of this pixel (correct this version to be 
			// perspective-correct).
			// Don't forget to multiply by depthP!
			Eigen::Vector3f worldP = depthP * (((b0 * t.verts[0] / depth0) + (b1 * t.verts[1] / depth1) + (b2 * t.verts[2] / depth2)));

			// Interpolate to find the normal of this pixel (correct this version to be 
			// perspective-correct).
			// Tip: you don't need to worry about multiplying by depthP - you'll normalise this anyway!
			Eigen::Vector3f normP = depthP * (((b0 * t.norms[0] / depth0) + (b1 * t.norms[1] / depth1) + (b2 * t.norms[2] / depth2)));
			normP.normalize();

			// Interpolate to find the correct clip-space depth (correct this version to be perspective-correct)
			// This won't make too much of a difference in this case, but technically this version does use slightly
			// incorrect depths.
			float depth = (t.screen[0].z() * b0) + (t.screen[1].z() * b1) + (t.screen[2].z() * b2);

			int depthIdx = static_cast<int>(p.x()) + static_cast<int>(p.y()) * width;
			if (depth > zBuffer[depthIdx]) 
			{
				continue;
			}
			else
			{
				zBuffer[depthIdx] = depth;
			}

			// Add code to calculate the texture coordinates corresponding to P, texP.
			// Use barycentric interpolation!
			Eigen::Vector2f texP = (t.texs[0] * b0) + (t.texs[1] * b1) + (t.texs[2] * b2);

			// Convert this coordinate to a point in texture space
			// To do so, multiply by the texWidth and texHeight to get to the correct range.
			// Don't forget to flip the y coordinates! 
			int texR = texHeight - (texP.y() * texHeight);
			int texC = texP.x() * texWidth;
			// Handle the case where texR or texC end up outside the image!
			// There are different ways you could do this - for example using 
			// the modulo (%) operator to wrap around, or clamping to the edges.
			// Write your own code below to do this - once you're done you should be sure 
			// that 0 <= texC < texWidth and 0 <= texR < texHeight.

			if (!(0 <= texC && texC < texWidth))
			{
				texC = std::max(0, texC);
				if (texC != 0)
				{
					texC = std::min(texC, texWidth - 1);
				}
			}

			if (!(0 <= texR && texR < texHeight))
			{
				texR = std::max(0, texR);
				if (texR != 0)
				{
					texR = std::min(texR, texWidth - 1);
				}
			}

			// Get the value from the texture (hint: use the getPixel function on the albedoTexture).
			Color texColor{ 255,255,255,255 };
			texColor = getPixel(albedoTexture, texC, texR, texWidth, texHeight);

			// Convert it into an Eigen::Vector3f as an albedo
			// (Optional bonus task, if you checked out the slides on gamma correction:
			// gamma correct this colour, so the texture doesn't appear overly bright.
			// should you raise to the power 1/2.2, or 2.2?)
			Eigen::Vector3f albedo{ pow(texColor.r / 255.0f, 2.2f), pow(texColor.g / 255.0f, 2.2f), pow(texColor.b / 255.0f, 2.2f) };

			// ----- Lighting code ------
			// Work out colour at this position.
			Eigen::Vector3f color = Eigen::Vector3f::Zero();

			Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();

			// Iterate over lights, and sum to find colour.
			for (auto& light : lights) {

				// Work out the contribution from this light source, and add it to the color variable.

				// Work out the intensity of this light source, at the point worldP.
				Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

				// We only need to do the following if the light isn't an ambient light.
				if (light->getType() != Light::Type::AMBIENT) {
					Eigen::Vector3f incomingLightDir = light->getDirection(worldP);

					float specularTerm;
					if (shadingMode == ShadingMode::PHONG) {
						specularTerm = phongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);
					}
					else {
						specularTerm = blinnPhongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);
					}

					Eigen::Vector3f specularOut = specularColor * specularTerm;
					specularOut = coeffWiseMultiply(specularOut, lightIntensity);

					// Take the dot product of the normal with the light direction.
					float dotProd = normP.dot(-incomingLightDir);

					// We don't want negative light - if dot product less than 0, set it to 0.
					dotProd = std::max(dotProd, 0.0f);

					// Multiply the light intensity by the dot product.
					Eigen::Vector3f diffuseOut = lightIntensity * dotProd;
					diffuseOut = coeffWiseMultiply(diffuseOut, albedo);

					color += specularOut;
					//color += diffuseOut;
					//color = (incomingLightDir + Eigen::Vector3f::Ones()) / 2;
				}
				else 
				{
					// Light is ambient - just multiply light intensity with albedo.
					color += coeffWiseMultiply(lightIntensity, albedo);
				}
			}

			Color c;
			// Gamma-correcting colours.
			c.r = std::min(powf(color.x(), 1 / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1 / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1 / 2.2f), 1.0f) * 255;

			c.a = 255;

			setPixel(image, x, y, width, height, c);
		}
}



void drawMesh(std::vector<unsigned char>& image,
	std::vector<float>& zBuffer,
	const Mesh& mesh,
	const std::vector<uint8_t>& albedoTexture, int texWidth, int texHeight,
	const Eigen::Vector3f& specularColor,
	float specularExponent,
	ShadingMode shadingMode,
	const Eigen::Vector3f& camWorldPos,
	const Eigen::Matrix4f& modelToWorld,
	const Eigen::Matrix4f& worldToCam,
	const Eigen::Matrix4f& camToClip,
	const std::vector<std::unique_ptr<Light>>& lights,
	int width, int height)
{
	for (int i = 0; i < mesh.vFaces.size(); ++i) 
	{
		Eigen::Vector3f
			v0 = mesh.verts[mesh.vFaces[i][0]],
			v1 = mesh.verts[mesh.vFaces[i][1]],
			v2 = mesh.verts[mesh.vFaces[i][2]];
		Eigen::Vector3f
			n0 = mesh.norms[mesh.nFaces[i][0]],
			n1 = mesh.norms[mesh.nFaces[i][1]],
			n2 = mesh.norms[mesh.nFaces[i][2]];

		Triangle t;
		t.verts[0] = (modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.verts[1] = (modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.verts[2] = (modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

		t.cam[0] = (worldToCam * modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.cam[1] = (worldToCam * modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.cam[2] = (worldToCam * modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

		// Work out the clip space coordinates, by multiplying by worldToClip and doing the 
		// perspective divide.
		Eigen::Vector4f vClip0 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v0);
		vClip0 /= vClip0.w();
		Eigen::Vector4f vClip1 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v1);
		vClip1 /= vClip1.w();
		Eigen::Vector4f vClip2 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v2);
		vClip2 /= vClip2.w();

		// Check that all 3 vertices are in the clip box (-1 to 1 in x, y and z) and if not,
		// skip drawing this triangle.
		// Hint: I've made a function outsideClipBox in LinAlg.hpp to help with this!
		if (outsideClipBox(vClip0) && outsideClipBox(vClip1) && outsideClipBox(vClip2))
		{
			continue;
		}

		// Work out the screen space coordinates based on the image height and width.
		t.screen[0] = Eigen::Vector3f((vClip0.x() + 1.0f) * width / 2, (-vClip0.y() + 1.0f) * height / 2, vClip0.z());
		t.screen[1] = Eigen::Vector3f((vClip1.x() + 1.0f) * width / 2, (-vClip1.y() + 1.0f) * height / 2, vClip1.z());
		t.screen[2] = Eigen::Vector3f((vClip2.x() + 1.0f) * width / 2, (-vClip2.y() + 1.0f) * height / 2, vClip2.z());

		// transform the normals (using the inverse transpose of the upper 3x3 block)
		t.norms[0] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n0).normalized();
		t.norms[1] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n1).normalized();
		t.norms[2] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n2).normalized();

		t.texs[0] = mesh.texs[mesh.tFaces[i][0]];
		t.texs[1] = mesh.texs[mesh.tFaces[i][1]];
		t.texs[2] = mesh.texs[mesh.tFaces[i][2]];

		drawTriangle(image, width, height, zBuffer, t, lights, albedoTexture, texWidth, texHeight, specularColor, specularExponent, shadingMode, camWorldPos);
	}
}

void superSample(std::vector<uint8_t>& imageBuffer, std::vector<uint8_t>& imageBufferOutput, int realHeight, int realWidth, int sampleRate, int width)
{
	// Go through each pixel for for the output image
	for (int r = 0; r < realHeight; ++r)
	{
		for (int c = 0; c < realWidth; ++c)
		{
			// avg used to store colour average for pixels
			Eigen::Vector4f avg(0, 0, 0, 0);

			// Go through each pixel in the large image
			for (int i = 0; i < sampleRate; ++i)
			{
				for (int j = 0; j < sampleRate; ++j)
				{
					// Gets pixel in larger image
					int superX = c * sampleRate + j;
					int superY = r * sampleRate + i;
					int index = (superY * width + superX) * 4;

					// Get the pixel colour
					for (int k = 0; k < 4; ++k)
					{
						// Add the pixel colour to the average
						avg[k] += imageBuffer[index + k];
					}
				}
			}

			// Divide the average by the number of samples
			avg /= (sampleRate * sampleRate);

			// Set the pixel in the output image
			int outIndex = (r * realWidth + c) * 4;

			// Set the pixel colour in the output image
			for (int k = 0; k < 4; ++k)
			{
				// Set the pixel colour in the output image
				imageBufferOutput[outIndex + k] = static_cast<uint8_t>(avg[k]);
			}
		}
	}
}

int drawScene(const std::string& outputFilename, ShadingMode mode, float specularExponent)
{
	// Setting up image size and sampling
	const int realWidth = 1920, realHeight = 1080;
	const int sampleRate = 2;

	// Upscaling the image size by the sample rate
	const int width = realWidth * sampleRate, height = realHeight * sampleRate;
	const int nChannels = 4;

	// Setting up an image buffer
	// This std::vector has one 8-bit value for each pixel in each row and column of the image, and
	// for each of the 4 channels (red, green, blue and alpha).
	// Remember 8-bit unsigned values can range from 0 to 255.
	std::vector<uint8_t> imageBuffer(height * width * nChannels);
	std::vector<uint8_t> imageBufferOutput(realHeight * realWidth * nChannels);
	std::vector<float> zBuffer(height * width);

	// This line sets the image to black initially.
	Color black{ 0,0,0,255 };

	for (int r = 0; r < height; ++r)
	{
		for (int c = 0; c < width; ++c)
		{
			setPixel(imageBuffer, c, r, width, height, black);
			zBuffer[r * width + c] = 1.0f;
		}
	}

	// ==========: Camera Matrices ========

	// This makes the projection matrix, using the function you implemented. Once the code is working,
	// try changing the FoV!
	Eigen::Matrix4f projection = projectionMatrix(height, width, 1.221f, 1000.0f);

	// This matrix rotates the camera, tilting it down, then translates it up to make it look down on the scene.
	// Once your code is working, try changing this to move the camera around!
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(48.25f, -18.0f, 75.0f)) * rotateYMatrix(-10.0f);
	cameraToWorld *= rotateXMatrix(-12.0f);

	//Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.0f, 10.0f, -70.0f));

	Eigen::Vector3f camWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);

	// The main important task = set up the worldToCamera and worldToClip matrices here!
	// Set up worldToCamera, based on cameraToWorld above
	Eigen::Matrix4f worldToCamera = cameraToWorld.inverse();
	// Set up worldToClip, using the projection and worldToCamera matrices
	Eigen::Matrix4f worldToClip = projection * worldToCamera;

	std::vector<std::unique_ptr<Light>> lights;
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.4f, 0.4f, 0.4f)));

	//lights.emplace_back(new PointLight(Eigen::Vector3f(75.0f, 75.0f, 75.0f), Eigen::Vector3f(0.0f, 0.0f, 220.0f)));
	//lights.emplace_back(new DirectionalLight(Eigen::Vector3f(0.4f, 0.4f, 0.4f), Eigen::Vector3f(1.f, 0.f, 0.0f)));
	lights.emplace_back(new SpotLight(Eigen::Vector3f(1.2f, 1.2f, 1.2f), Eigen::Vector3f(-0.225f, 0.55f, 220.0f), Eigen::Vector3f(0.0f, 0.0f, 0.0f), 10.0f));
	lights.emplace_back(new SpotLight(Eigen::Vector3f(1.2f, 1.2f, 1.2f), Eigen::Vector3f(-0.225f, 0.55f, 225.0f), Eigen::Vector3f(0.0f, 0.0f, 0.0f), 10.0f));
	lights.emplace_back(new SpotLight(Eigen::Vector3f(1.2f, 1.2f, 1.2f), Eigen::Vector3f(-0.225f, 0.55f, 230.0f), Eigen::Vector3f(0.0f, 0.0f, 0.0f), 10.0f));
	lights.emplace_back(new SpotLight(Eigen::Vector3f(1.2f, 1.2f, 1.2f), Eigen::Vector3f(0.425f, 0.55f, 220.0f), Eigen::Vector3f(0.0f, 0.0f, 0.0f), 10.0f));
	lights.emplace_back(new SpotLight(Eigen::Vector3f(1.2f, 1.2f, 1.2f), Eigen::Vector3f(0.425f, 0.55f, 225.0f), Eigen::Vector3f(0.0f, 0.0f, 0.0f), 10.0f));
	lights.emplace_back(new SpotLight(Eigen::Vector3f(1.2f, 1.2f, 1.2f), Eigen::Vector3f(0.425f, 0.55f, 230.0f), Eigen::Vector3f(0.0f, 0.0f, 0.0f), 10.0f));


	struct TextureData
	{
		std::vector<uint8_t> data;
		unsigned int texWidth, texHeight;
	};

	
	#pragma region Model Loading

	Mesh snakeModel = loadMeshFile("../models/Snake.obj");
	TextureData snakeTexture;
	lodepng::decode(snakeTexture.data, snakeTexture.texWidth, snakeTexture.texHeight, "../models/SnakeAtlas.png");

	Eigen::Matrix4f snakeTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	snakeTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, snakeModel, snakeTexture.data, snakeTexture.texWidth, snakeTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, snakeTransform, worldToCamera, projection, lights, width, height);


	Mesh GRUOneModel = loadMeshFile("../models/GRU.obj");
	TextureData GRUTexture;
	lodepng::decode(GRUTexture.data, GRUTexture.texWidth, GRUTexture.texHeight, "../models/AtlasGRU.png");

	Eigen::Matrix4f GRUOneTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	GRUOneTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, GRUOneModel, GRUTexture.data, GRUTexture.texWidth, GRUTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, GRUOneTransform, worldToCamera, projection, lights, width, height);


	Mesh GRUTwoModel = loadMeshFile("../models/GRU.obj");

	Eigen::Matrix4f GRUTwoTransform = translationMatrix(Eigen::Vector3f(5.0f, -20.0f, 180.0f));
	GRUTwoTransform *= rotateYMatrix(90.0f);
	drawMesh(imageBuffer, zBuffer, GRUTwoModel, GRUTexture.data, GRUTexture.texWidth, GRUTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, GRUTwoTransform, worldToCamera, projection, lights, width, height);


	Mesh socomModel = loadMeshFile("../models/Socom.obj");
	TextureData socomTexture;
	lodepng::decode(socomTexture.data, socomTexture.texWidth, socomTexture.texHeight, "../models/Socom.png");

	Eigen::Matrix4f socomTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	socomTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, socomModel, socomTexture.data, socomTexture.texWidth, socomTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, socomTransform, worldToCamera, projection, lights, width, height);


	Mesh famasModel = loadMeshFile("../models/Famas.obj");
	TextureData famasTexture;
	lodepng::decode(famasTexture.data, famasTexture.texWidth, famasTexture.texHeight, "../models/Famas.png");

	Eigen::Matrix4f famasOneTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	famasOneTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, famasModel, famasTexture.data, famasTexture.texWidth, famasTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, famasOneTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f famasTwoTransform = translationMatrix(Eigen::Vector3f(5.0f, -20.0f, 180.0f));
	famasTwoTransform *= rotateYMatrix(90.0f);
	drawMesh(imageBuffer, zBuffer, famasModel, famasTexture.data, famasTexture.texWidth, famasTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, famasTwoTransform, worldToCamera, projection, lights, width, height);


	Mesh wallOneModel = loadMeshFile("../models/WallOne.obj");
	TextureData wallOneTexture;
	lodepng::decode(wallOneTexture.data, wallOneTexture.texWidth, wallOneTexture.texHeight, "../models/WallOne.png");

	Eigen::Matrix4f wallOneTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	wallOneTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, wallOneModel, wallOneTexture.data, wallOneTexture.texWidth, wallOneTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, wallOneTransform, worldToCamera, projection, lights, width, height);


	Mesh wallTwoModel = loadMeshFile("../models/WallTwo.obj");
	TextureData wallTwoTexture;
	lodepng::decode(wallTwoTexture.data, wallTwoTexture.texWidth, wallTwoTexture.texHeight, "../models/WallTwo.png");

	Eigen::Matrix4f wallTwoTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	wallTwoTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, wallTwoModel, wallTwoTexture.data, wallTwoTexture.texWidth, wallTwoTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, wallTwoTransform, worldToCamera, projection, lights, width, height);


	Mesh wallThreeModel = loadMeshFile("../models/WallThree.obj");
	TextureData wallThreeTexture;
	lodepng::decode(wallThreeTexture.data, wallThreeTexture.texWidth, wallThreeTexture.texHeight, "../models/WallThree.png");

	Eigen::Matrix4f wallThreeTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	wallThreeTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, wallThreeModel, wallThreeTexture.data, wallThreeTexture.texWidth, wallThreeTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, wallThreeTransform, worldToCamera, projection, lights, width, height);


	Mesh wallFourModel = loadMeshFile("../models/WallFour.obj");
	TextureData wallFourTexture;
	lodepng::decode(wallFourTexture.data, wallFourTexture.texWidth, wallFourTexture.texHeight, "../models/WallThree.png");

	Eigen::Matrix4f wallFourTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	wallFourTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, wallFourModel, wallFourTexture.data, wallFourTexture.texWidth, wallFourTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, wallFourTransform, worldToCamera, projection, lights, width, height);


	Mesh farWallDetail = loadMeshFile("../models/FarWallDetail.obj");
	TextureData farWallDetailTexture;
	lodepng::decode(farWallDetailTexture.data, farWallDetailTexture.texWidth, farWallDetailTexture.texHeight, "../models/FarWallDetail.png");

	Eigen::Matrix4f farWallDetailTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	farWallDetailTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, farWallDetail, farWallDetailTexture.data, farWallDetailTexture.texWidth, farWallDetailTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, farWallDetailTransform, worldToCamera, projection, lights, width, height);


	Mesh secondFloorOneModel = loadMeshFile("../models/SecondFloorOne.obj");
	TextureData secondFloorOneTexture;
	lodepng::decode(secondFloorOneTexture.data, secondFloorOneTexture.texWidth, secondFloorOneTexture.texHeight, "../models/SecondFloorOne.png");

	Eigen::Matrix4f secondFloorOneTransform = translationMatrix(Eigen::Vector3f(40.0f, -21.5f, 139.0f));
	secondFloorOneTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, secondFloorOneModel, secondFloorOneTexture.data, secondFloorOneTexture.texWidth, secondFloorOneTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, secondFloorOneTransform, worldToCamera, projection, lights, width, height);


	Mesh secondFloorTwoModel = loadMeshFile("../models/SecondFloorTwo.obj");
	TextureData secondFloorTwoTexture;
	lodepng::decode(secondFloorTwoTexture.data, secondFloorTwoTexture.texWidth, secondFloorTwoTexture.texHeight, "../models/SecondFloorTwo.png");

	Eigen::Matrix4f secondFloorTwoTransform = translationMatrix(Eigen::Vector3f(40.0f, -21.5f, 139.0f));
	secondFloorTwoTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, secondFloorTwoModel, secondFloorTwoTexture.data, secondFloorTwoTexture.texWidth, secondFloorTwoTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, secondFloorTwoTransform, worldToCamera, projection, lights, width, height);


	Mesh secondFloorRailingModel = loadMeshFile("../models/SecondFloorRailing.obj");
	TextureData secondFloorRailingTexture;
	lodepng::decode(secondFloorRailingTexture.data, secondFloorRailingTexture.texWidth, secondFloorRailingTexture.texHeight, "../models/Floor.png");

	Eigen::Matrix4f secondFloorRailingOneTransform = translationMatrix(Eigen::Vector3f(40.0f, -21.5f, 139.0f));
	secondFloorRailingOneTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, secondFloorRailingModel, secondFloorRailingTexture.data, secondFloorRailingTexture.texWidth, secondFloorRailingTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, secondFloorRailingOneTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f secondFloorRailingTwoTransform = translationMatrix(Eigen::Vector3f(-54.5f, -21.5f, 139.0f));
	secondFloorRailingTwoTransform *= rotateXMatrix(180.0f);
	secondFloorRailingTwoTransform *= rotateYMatrix(180.0f);
	secondFloorRailingTwoTransform *= scaleMatrix(-1.0f);
	drawMesh(imageBuffer, zBuffer, secondFloorRailingModel, secondFloorRailingTexture.data, secondFloorRailingTexture.texWidth, secondFloorRailingTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, secondFloorRailingTwoTransform, worldToCamera, projection, lights, width, height);


	Mesh floorModel = loadMeshFile("../models/Floor.obj");
	TextureData floorTexture;
	lodepng::decode(floorTexture.data, floorTexture.texWidth, floorTexture.texHeight, "../models/Floor.png");

	Eigen::Matrix4f floorTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 165.0f));
	floorTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, floorModel, floorTexture.data, floorTexture.texWidth, floorTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, floorTransform, worldToCamera, projection, lights, width, height);


	Mesh lightModel = loadMeshFile("../models/Light.obj");
	TextureData lightTexture;
	lodepng::decode(lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, "../models/Light.png");

	Eigen::Matrix4f lightOneTransform = translationMatrix(Eigen::Vector3f(29.5f, -20.0f, 130.0f));
	lightOneTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightOneTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightTwoTransform = translationMatrix(Eigen::Vector3f(29.5f, -20.0f, 170.0f));
	lightTwoTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightTwoTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightThreeTransform = translationMatrix(Eigen::Vector3f(54.5f, -20.0f, 170.0f));
	lightThreeTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightThreeTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightFourTransform = translationMatrix(Eigen::Vector3f(54.5f, -20.0f, 130.0f));
	lightFourTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightFourTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightFiveTransform = translationMatrix(Eigen::Vector3f(82.5f, -20.0f, 130.0f));
	lightFiveTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightFiveTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightSixTransform = translationMatrix(Eigen::Vector3f(82.5f, -20.0f, 170.0f));
	lightSixTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightSixTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightSevenTransform = translationMatrix(Eigen::Vector3f(97.5f, -20.0f, 130.0f));
	lightSevenTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightSevenTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightEightTransform = translationMatrix(Eigen::Vector3f(97.5f, -20.0f, 170.0f));
	lightEightTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightEightTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightNineTransform = translationMatrix(Eigen::Vector3f(112.5f, -20.0f, 130.0f));
	lightNineTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightNineTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f lightTenTransform = translationMatrix(Eigen::Vector3f(112.5f, -20.0f, 170.0f));
	lightTenTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, lightModel, lightTexture.data, lightTexture.texWidth, lightTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, lightTenTransform, worldToCamera, projection, lights, width, height);


	Mesh tankOneModel = loadMeshFile("../models/TankOne.obj");
	TextureData tankOneTexture;
	lodepng::decode(tankOneTexture.data, tankOneTexture.texWidth, tankOneTexture.texHeight, "../models/TankOne.png");

	Eigen::Matrix4f tankOneTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.2f, 130.0f));
	tankOneTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, tankOneModel, tankOneTexture.data, tankOneTexture.texWidth, tankOneTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, tankOneTransform, worldToCamera, projection, lights, width, height);

	Eigen::Matrix4f tankTwoTransform = translationMatrix(Eigen::Vector3f(-52.0f, -20.2f, 210.0f));
	drawMesh(imageBuffer, zBuffer, tankOneModel, tankOneTexture.data, tankOneTexture.texWidth, tankOneTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, tankTwoTransform, worldToCamera, projection, lights, width, height);


	Mesh tankTwoModel = loadMeshFile("../models/TankTwo.obj");
	TextureData tankTwoTexture;
	lodepng::decode(tankTwoTexture.data, tankTwoTexture.texWidth, tankTwoTexture.texHeight, "../models/TankTwo.png");

	Eigen::Matrix4f tankThreeTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.2f, 130.0f));
	tankThreeTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, tankTwoModel, tankTwoTexture.data, tankTwoTexture.texWidth, tankTwoTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, tankThreeTransform, worldToCamera, projection, lights, width, height);


	Mesh boxModel = loadMeshFile("../models/Box.obj");
	TextureData boxOneTexture;
	lodepng::decode(boxOneTexture.data, boxOneTexture.texWidth, boxOneTexture.texHeight, "../models/BoxOne.png");

	Eigen::Matrix4f boxOneTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	boxOneTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, boxModel, boxOneTexture.data, boxOneTexture.texWidth, boxOneTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, boxOneTransform, worldToCamera, projection, lights, width, height);

	TextureData boxTwoTexture;
	lodepng::decode(boxTwoTexture.data, boxTwoTexture.texWidth, boxTwoTexture.texHeight, "../models/BoxTwo.png");

	Eigen::Matrix4f boxTwoTransform = translationMatrix(Eigen::Vector3f(36.25f, -14.25f, 126.0f));
	boxTwoTransform *= rotateYMatrix(190.0f);
	drawMesh(imageBuffer, zBuffer, boxModel, boxTwoTexture.data, boxTwoTexture.texWidth, boxTwoTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, boxTwoTransform, worldToCamera, projection, lights, width, height);

	Mesh rafterModel = loadMeshFile("../models/Rafter.obj");
	TextureData rafterTexture;
	lodepng::decode(rafterTexture.data, rafterTexture.texWidth, rafterTexture.texHeight, "../models/Rafter.png");

	Eigen::Matrix4f rafterTransform = translationMatrix(Eigen::Vector3f(39.5f, -19.5f, 130.0f));
	rafterTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, rafterModel, rafterTexture.data, rafterTexture.texWidth, rafterTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, rafterTransform, worldToCamera, projection, lights, width, height);

	Mesh cameraModel = loadMeshFile("../models/Camera.obj");
	TextureData cameraTexture;
	lodepng::decode(cameraTexture.data, cameraTexture.texWidth, cameraTexture.texHeight, "../models/Camera.png");

	Eigen::Matrix4f cameraTransform = translationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	cameraTransform *= rotateYMatrix(180.0f);
	drawMesh(imageBuffer, zBuffer, cameraModel, cameraTexture.data, cameraTexture.texWidth, cameraTexture.texHeight, Eigen::Vector3f::Ones() * 1.0f, specularExponent, mode, camWorldPos, cameraTransform, worldToCamera, projection, lights, width, height);

	#pragma endregion


	// For debug - draw point lights as colored circles so we can see where they are
	//drawPointLights(imageBuffer, width, height, lights);

	// Downsize the image to 1920 1080 for sampling
	superSample(imageBuffer, imageBufferOutput, realHeight, realWidth, sampleRate, width);

	// Save the image to png.
	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBufferOutput, realWidth, realHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	saveZBufferImage("zBuffer.png", zBuffer, width, height);

	return 0;
}

int main()
{
	drawScene("output_phong.png", ShadingMode::PHONG, 2.5f);
	drawScene("output_blinnphong.png", ShadingMode::BLINN_PHONG, 7.5f);

	return 0;
}
