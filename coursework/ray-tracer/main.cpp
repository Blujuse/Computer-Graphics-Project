#include <Eigen/Dense>
#include <lodepng.h>
#include <json/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "BVHNode.hpp"
#include "Triangle.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PointLight.hpp"
#include "DirectionalLight.hpp"
#include "SpotLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "PhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "Model.hpp"
#include <fstream>

/// <summary>
/// Load a JSON config file using the nlohmann library.
/// </summary>
nlohmann::json loadConfig(const std::string& filename)
{
	std::ifstream configStream(filename);
	nlohmann::json config = nlohmann::json::parse(configStream);
	return config;
}

/// <summary>
/// Load an Eigen Vector3f from a config file.
/// Call as for example loadVec3FromConfig(config["myVector3"]);
/// </summary>
Eigen::Vector3f loadVec3FromConfig(const nlohmann::json& config)
{
	return Eigen::Vector3f(config[0], config[1], config[2]);
}

int main(int argc, char* argv[]) {

	// *** Load the config file ***
	auto config = loadConfig("../config/config.json");

	const int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];
	const int nChannels = 4;

	// *** Set up camera and output image ***
	Camera cam(
		loadVec3FromConfig(config["cameraPos"]),
		loadVec3FromConfig(config["cameraForward"]),
		loadVec3FromConfig(config["cameraUp"]),
		pixWidth, pixHeight,
		config["cameraFov"]);


	std::vector<uint8_t> outImage(pixHeight * pixWidth * nChannels);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);

	// *** Load shaders and textures ***
	struct TextureData 
	{
		std::vector<uint8_t> data;
		unsigned int width, height;
	};

	TextureData snakeTexture;
	lodepng::decode(snakeTexture.data, snakeTexture.width, snakeTexture.height, "../models/SnakeAtlas.png");

	TextureData GRUTexture;
	lodepng::decode(GRUTexture.data, GRUTexture.width, GRUTexture.height, "../models/AtlasGRU.png");

	TextureData socomTexture;
	lodepng::decode(socomTexture.data, socomTexture.width, socomTexture.height, "../models/socom.png");

	TextureData famasTexture;
	lodepng::decode(famasTexture.data, famasTexture.width, famasTexture.height, "../models/Famas.png");

	TextureData wallOneTexture;
	lodepng::decode(wallOneTexture.data, wallOneTexture.width, wallOneTexture.height, "../models/WallOne.png");

	TextureData hiddenWallTexture;
	lodepng::decode(hiddenWallTexture.data, hiddenWallTexture.width, hiddenWallTexture.height, "../models/FarWallDetail.png");

	TextureData wallTwoTexture;
	lodepng::decode(wallTwoTexture.data, wallTwoTexture.width, wallTwoTexture.height, "../models/WallTwo.png");

	TextureData wallThreeTexture;
	lodepng::decode(wallThreeTexture.data, wallThreeTexture.width, wallThreeTexture.height, "../models/WallThree.png");

	TextureData wallFourTexture;
	lodepng::decode(wallFourTexture.data, wallFourTexture.width, wallFourTexture.height, "../models/WallThree.png");

	TextureData farWallDetailTexture;
	lodepng::decode(farWallDetailTexture.data, farWallDetailTexture.width, farWallDetailTexture.height, "../models/FarWallDetail.png");

	TextureData secondFloorOne;
	lodepng::decode(secondFloorOne.data, secondFloorOne.width, secondFloorOne.height, "../models/SecondFloorOne.png");

	TextureData secondFloorTwo;
	lodepng::decode(secondFloorTwo.data, secondFloorTwo.width, secondFloorTwo.height, "../models/SecondFloorTwo.png");

	TextureData secondFloorRailing;
	lodepng::decode(secondFloorRailing.data, secondFloorRailing.width, secondFloorRailing.height, "../models/Floor.png");

	TextureData floorTexture;
	lodepng::decode(floorTexture.data, floorTexture.width, floorTexture.height, "../models/Floor.png");

	TextureData lightTexture;
	lodepng::decode(lightTexture.data, lightTexture.width, lightTexture.height, "../models/Light.png");

	TextureData tankTextureOne;
	lodepng::decode(tankTextureOne.data, tankTextureOne.width, tankTextureOne.height, "../models/TankOne.png");

	TextureData tankTextureTwo;
	lodepng::decode(tankTextureTwo.data, tankTextureTwo.width, tankTextureTwo.height, "../models/TankTwo.png");

	TextureData boxOneTexture;
	lodepng::decode(boxOneTexture.data, boxOneTexture.width, boxOneTexture.height, "../models/BoxOne.png");

	TextureData boxTwoTexture;
	lodepng::decode(boxTwoTexture.data, boxTwoTexture.width, boxTwoTexture.height, "../models/BoxTwo.png");

	TextureData rafterTexture;
	lodepng::decode(rafterTexture.data, rafterTexture.width, rafterTexture.height, "../models/Rafter.png");

	TextureData cameraTexture;
	lodepng::decode(cameraTexture.data, cameraTexture.width, cameraTexture.height, "../models/Camera.png");


	TexturedLambertianShader snakeShader(&snakeTexture.data, snakeTexture.width, snakeTexture.height);
	TexturedLambertianShader GRUShader(&GRUTexture.data, GRUTexture.width, GRUTexture.height);
	PhongShader socomShader(&socomTexture.data, socomTexture.width, socomTexture.height, Eigen::Vector3f(1.25f, 1.25f, 1.25f), 1.0f);
	PhongShader famasShader(&famasTexture.data, famasTexture.width, famasTexture.height, Eigen::Vector3f(0.75f, 0.75f, 0.75f), 0.5f);
	TexturedLambertianShader wallOneShader(&wallOneTexture.data, wallOneTexture.width, wallOneTexture.height);
	TexturedLambertianShader hiddenWallShader(&hiddenWallTexture.data, hiddenWallTexture.width, hiddenWallTexture.height);
	TexturedLambertianShader wallTwoShader(&wallTwoTexture.data, wallTwoTexture.width, wallTwoTexture.height);
	TexturedLambertianShader wallThreeShader(&wallThreeTexture.data, wallThreeTexture.width, wallThreeTexture.height);
	TexturedLambertianShader wallFourShader(&wallFourTexture.data, wallFourTexture.width, wallFourTexture.height);
	TexturedLambertianShader farWallDetailShader(&farWallDetailTexture.data, farWallDetailTexture.width, farWallDetailTexture.height);
	TexturedLambertianShader secondFloorOneShader(&secondFloorOne.data, secondFloorOne.width, secondFloorOne.height);
	TexturedLambertianShader secondFloorTwoShader(&secondFloorTwo.data, secondFloorTwo.width, secondFloorTwo.height);
	TexturedLambertianShader secondFloorRailingShader(&secondFloorRailing.data, secondFloorRailing.width, secondFloorRailing.height);
	TexturedLambertianShader floorShader(&floorTexture.data, floorTexture.width, floorTexture.height);
	TexturedLambertianShader lightShader(&lightTexture.data, lightTexture.width, lightTexture.height, false);
	TexturedLambertianShader tankShaderOne(&tankTextureOne.data, tankTextureOne.width, tankTextureOne.height);
	TexturedLambertianShader tankShaderTwo(&tankTextureTwo.data, tankTextureTwo.width, tankTextureTwo.height);
	TexturedLambertianShader boxOneShader(&boxOneTexture.data, boxOneTexture.width, boxOneTexture.height);
	TexturedLambertianShader boxTwoShader(&boxTwoTexture.data, boxTwoTexture.width, boxTwoTexture.height);
	TexturedLambertianShader rafterShader(&rafterTexture.data, rafterTexture.width, rafterTexture.height);
	TexturedLambertianShader cameraShader(&cameraTexture.data, cameraTexture.width, cameraTexture.height);


	// *** Set up scene ***
	Scene scene;
	
	// BVH loading
	Model snakeModel("../models/Snake.obj");
	Eigen::Matrix4f snakeTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	snakeTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(snakeModel, &snakeShader, 6, snakeTransform));

	Model GRUModel("../models/GRU.obj");
	Eigen::Matrix4f GRUOneTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	GRUOneTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(GRUModel, &GRUShader, 6, GRUOneTransform));

	Eigen::Matrix4f GRUTwoTransform = makeTranslationMatrix(Eigen::Vector3f(5.0f, -20.0f, 180.0f));
	GRUTwoTransform *= rotateY(90.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(GRUModel, &GRUShader, 6, GRUTwoTransform));

	Model socomModel("../models/Socom.obj");
	Eigen::Matrix4f socomTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	socomTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(socomModel, &socomShader, 4, socomTransform));

	Model famasModel("../models/Famas.obj");
	Eigen::Matrix4f famasOneTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -20.0f, 130.0f));
	famasOneTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(famasModel, &famasShader, 4, famasOneTransform));

	Eigen::Matrix4f famasTwoTransform = makeTranslationMatrix(Eigen::Vector3f(5.0f, -20.0f, 180.0f));
	famasTwoTransform *= rotateY(90.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(famasModel, &famasShader, 4, famasTwoTransform));

	Model wallOneModel("../models/WallOne.obj");
	Eigen::Matrix4f wallOneTransform = makeTranslationMatrix(Eigen::Vector3f(40.0f, -19.5f, 130.0f));
	wallOneTransform *= rotateY(180.0f);
	//wallOneTransform *= rotateZ(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(wallOneModel, &wallOneShader, 4, wallOneTransform));

	Model hiddenWallModel("../models/HiddenWall.obj");
	Eigen::Matrix4f hiddenWallTransform = makeTranslationMatrix(Eigen::Vector3f(40.0f, -19.5f, 145.0f));
	hiddenWallTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(hiddenWallModel, &hiddenWallShader, 4, hiddenWallTransform));

	Model wallTwoModel("../models/WallTwo.obj");
	Eigen::Matrix4f wallTwoTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -19.5f, 130.0f));
	wallTwoTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(wallTwoModel, &wallTwoShader, 4, wallTwoTransform));

	Model wallThreeModel("../models/WallThree.obj");
	Eigen::Matrix4f wallThreeTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -19.5f, 130.0f));
	wallThreeTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(wallThreeModel, &wallThreeShader, 4, wallThreeTransform));

	Model wallFourModel("../models/WallFour.obj");
	Eigen::Matrix4f wallFourTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -19.5f, 130.0f));
	wallFourTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(wallFourModel, &wallFourShader, 4, wallFourTransform));

	Model farWallDetail("../models/farWallDetail.obj");
	Eigen::Matrix4f farWallDetailTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -19.5f, 130.0f));
	farWallDetailTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(farWallDetail, &farWallDetailShader, 4, farWallDetailTransform));

	Model secondFloorOneModel("../models/SecondFloorOne.obj");
	Eigen::Matrix4f secondFloorOneTransform = makeTranslationMatrix(Eigen::Vector3f(40.0f, -21.5f, 139.0f));
	secondFloorOneTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(secondFloorOneModel, &secondFloorOneShader, 4, secondFloorOneTransform));

	Model secondFloorTwoModel("../models/SecondFloorTwo.obj");
	Eigen::Matrix4f secondFloorTwoTransform = makeTranslationMatrix(Eigen::Vector3f(40.0f, -21.5f, 139.0f));
	secondFloorTwoTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(secondFloorTwoModel, &secondFloorTwoShader, 4, secondFloorTwoTransform));

	Model secondFloorRailingModel("../models/SecondFloorRailing.obj");
	Eigen::Matrix4f secondFloorRailingTransformOne = makeTranslationMatrix(Eigen::Vector3f(40.0f, -21.5f, 139.0f));
	secondFloorRailingTransformOne *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(secondFloorRailingModel, &secondFloorRailingShader, 4, secondFloorRailingTransformOne));

	Eigen::Matrix4f secondFloorRailingTransformTwo = makeTranslationMatrix(Eigen::Vector3f(-54.5f, -21.5f, 139.0f));
	secondFloorRailingTransformTwo *= rotateX(180.0f);
	secondFloorRailingTransformTwo *= rotateY(180.0f);
	secondFloorRailingTransformTwo *= uniformScale(-1.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(secondFloorRailingModel, &secondFloorRailingShader, 4, secondFloorRailingTransformTwo, nullptr, false));

	Model floorModel("../models/Floor.obj");
	Eigen::Matrix4f floorTransform = makeTranslationMatrix(Eigen::Vector3f(-45.0f, -20.0f, 160.0f));
	scene.renderables.push_back(std::make_shared<BVHNode>(floorModel, &floorShader, 4, floorTransform));
	
	Model tankOneModel("../models/TankOne.obj");
	Eigen::Matrix4f tankOneTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -20.2f, 130.0f));
	tankOneTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(tankOneModel, &tankShaderOne, 4, tankOneTransform));

	Eigen::Matrix4f tankOneTwoTransform = makeTranslationMatrix(Eigen::Vector3f(-52.0f, -20.2f, 210.0f));
	scene.renderables.push_back(std::make_shared<BVHNode>(tankOneModel, &tankShaderOne, 4, tankOneTwoTransform));

	Model tankTwoModel("../models/TankTwo.obj");
	Eigen::Matrix4f tankTwoTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -20.2f, 130.0f));
	tankTwoTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(tankTwoModel, &tankShaderTwo, 4, tankTwoTransform));

	Model boxModel("../models/Box.obj");
	Eigen::Matrix4f boxOneTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -20.5f, 130.0f));
	boxOneTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(boxModel, &boxOneShader, 4, boxOneTransform));

	Eigen::Matrix4f boxTwoTransform = makeTranslationMatrix(Eigen::Vector3f(36.25f, -14.75f, 126.0f));
	boxTwoTransform *= rotateY(190.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(boxModel, &boxTwoShader, 4, boxTwoTransform));

	Model rafterModel("../models/Rafter.obj");
	Eigen::Matrix4f rafterTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -19.5f, 130.0f));
	rafterTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(rafterModel, &rafterShader, 4, rafterTransform));

	Model cameraModel("../models/Camera.obj");
	Eigen::Matrix4f cameraTransform = makeTranslationMatrix(Eigen::Vector3f(39.5f, -19.5f, 130.0f));
	cameraTransform *= rotateY(180.0f);
	scene.renderables.push_back(std::make_shared<BVHNode>(cameraModel, &cameraShader, 4, cameraTransform));

	#pragma region Light Models

	Model lightModel("../models/Light.obj");
	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformOne = makeTranslationMatrix(Eigen::Vector3f(55.5f, 62.0f, 200.0f));
	lightTransformOne *= rotateY(180.0f);
	lightTransformOne *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformOne);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformTwo = makeTranslationMatrix(Eigen::Vector3f(35.0f, 62.0f, 200.0f));
	lightTransformTwo *= rotateY(180.0f);
	lightTransformTwo *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformTwo);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformThree = makeTranslationMatrix(Eigen::Vector3f(55.5f, 62.0f, 250.0f));
	lightTransformThree *= rotateY(180.0f);
	lightTransformThree *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformThree);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformFour = makeTranslationMatrix(Eigen::Vector3f(35.0f, 62.0f, 250.0f));
	lightTransformFour *= rotateY(180.0f);
	lightTransformFour *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformFour);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformFive = makeTranslationMatrix(Eigen::Vector3f(75.0f, 62.0f, 200.0f));
	lightTransformFive *= rotateY(180.0f);
	lightTransformFive *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformFive);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformSix = makeTranslationMatrix(Eigen::Vector3f(95.0f, 62.0f, 200.0f));
	lightTransformSix *= rotateY(180.0f);
	lightTransformSix *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformSix);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformSeven = makeTranslationMatrix(Eigen::Vector3f(115.0f, 62.0f, 200.0f));
	lightTransformSeven *= rotateY(180.0f);
	lightTransformSeven *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformSeven);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformEight = makeTranslationMatrix(Eigen::Vector3f(75.0f, 62.0f, 250.0f));
	lightTransformEight *= rotateY(180.0f);
	lightTransformEight *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformEight);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformNine = makeTranslationMatrix(Eigen::Vector3f(95.0f, 62.0f, 250.0f));
	lightTransformNine *= rotateY(180.0f);
	lightTransformNine *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformNine);

	scene.renderables.push_back(std::make_shared<Mesh>(&lightShader, &lightModel, nullptr, false, true, VISIBLE_BITMASK));
	Eigen::Matrix4f lightTransformTen = makeTranslationMatrix(Eigen::Vector3f(115.0f, 62.0f, 250.0f));
	lightTransformTen *= rotateY(180.0f);
	lightTransformTen *= rotateX(180.0f);
	scene.renderables.back()->modelToWorld(lightTransformTen);

	#pragma endregion

	// *** Add lights to scene ***
	Eigen::Vector3f ambientLight(.6f, .6f, .6f);

	std::vector<std::unique_ptr<Light>> lightSources;
	
	Eigen::Vector3f spotlightPos = Eigen::Vector3f(-50.5f, 0.55f, 200.0f);
	Eigen::Vector3f targetPoint = Eigen::Vector3f(-50.5f, -20.0f, 200.0f);
	Eigen::Vector3f direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(-50.5f, 0.55f, 150.0f);
	targetPoint = Eigen::Vector3f(-50.5f, -20.0f, 150.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(-50.5f, 0.55f, 100.0f);
	targetPoint = Eigen::Vector3f(-50.5f, -20.0f, 100.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(-50.5f, 0.55f, 50.0f);
	targetPoint = Eigen::Vector3f(-50.5f, -20.0f, 50.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(-20.5f, 0.55f, 200.0f);
	targetPoint = Eigen::Vector3f(-20.5f, -20.0f, 200.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(-20.5f, 0.55f, 150.0f);
	targetPoint = Eigen::Vector3f(-20.5f, -20.0f, 150.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(-20.5f, 0.55f, 100.0f);
	targetPoint = Eigen::Vector3f(-20.5f, -20.0f, 100.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(-20.5f, 0.55f, 50.0f);
	targetPoint = Eigen::Vector3f(-20.5f, -20.0f, 50.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(10.5f, 0.55f, 200.0f);
	targetPoint = Eigen::Vector3f(10.5f, -20.0f, 200.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(10.5f, 0.55f, 150.0f);
	targetPoint = Eigen::Vector3f(10.5f, -20.0f, 150.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(10.5f, 0.55f, 100.0f);
	targetPoint = Eigen::Vector3f(10.5f, -20.0f, 100.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(10.5f, 0.55f, 50.0f);
	targetPoint = Eigen::Vector3f(10.5f, -20.0f, 50.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 5.0f));

	spotlightPos = Eigen::Vector3f(40.5f, 0.55f, 200.0f);
	targetPoint = Eigen::Vector3f(40.5f, -20.0f, 200.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 3.0f));

	spotlightPos = Eigen::Vector3f(40.5f, 0.55f, 150.0f);
	targetPoint = Eigen::Vector3f(40.5f, -20.0f, 150.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 3.0f));

	spotlightPos = Eigen::Vector3f(40.5f, 0.55f, 100.0f);
	targetPoint = Eigen::Vector3f(40.5f, -20.0f, 100.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 3.0f));

	spotlightPos = Eigen::Vector3f(40.5f, 0.55f, 50.0f);
	targetPoint = Eigen::Vector3f(40.5f, -20.0f, 50.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 3.0f));

	spotlightPos = Eigen::Vector3f(50.5f, 0.55f, 200.0f);
	targetPoint = Eigen::Vector3f(50.5f, -20.0f, 200.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 3.0f));

	spotlightPos = Eigen::Vector3f(50.5f, 0.55f, 150.0f);
	targetPoint = Eigen::Vector3f(50.5f, -20.0f, 150.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 3.0f));

	spotlightPos = Eigen::Vector3f(50.5f, 0.55f, 100.0f);
	targetPoint = Eigen::Vector3f(50.5f, -20.0f, 100.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 3.0f));

	spotlightPos = Eigen::Vector3f(50.5f, 0.55f, 50.0f);
	targetPoint = Eigen::Vector3f(50.5f, -20.0f, 50.0f);
	direction = (targetPoint - spotlightPos).normalized();
	lightSources.push_back(std::make_unique<Spotlight>(Eigen::Vector3f(250.0f, 250.0f, 250.0f), spotlightPos, direction, 3.0f));
	
	// *** Render the scene ***

	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}

	auto startTime = std::chrono::steady_clock::now();

	Ray ray = cam.getRay(531, 325);
	HitInfo hitInfo;
	scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK);
	float x = hitInfo.hitT;


	#pragma omp parallel for
	for (int y = 0; y < pixHeight; ++y) {
		for (int x = 0; x < pixWidth; ++x) {
			Ray ray = cam.getRay(x, scanlines[y]);
			HitInfo hitInfo;
			if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
				Eigen::Vector3f color = hitInfo.shader->getColor(
					hitInfo, &scene,
					lightSources, ambientLight,
					0, config["maxBounces"]);

				color.x() = std::min(color.x(), 1.f);
				color.y() = std::min(color.y(), 1.f);
				color.z() = std::min(color.z(), 1.f);


				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = color.x() * 255;
				outImage[(x + line * pixWidth) * nChannels + 1] = color.y() * 255;
				outImage[(x + line * pixWidth) * nChannels + 2] = color.z() * 255;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
			else {
				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = 0;
				outImage[(x + line * pixWidth) * nChannels + 1] = 0;
				outImage[(x + line * pixWidth) * nChannels + 2] = 0;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
		}
		if (omp_get_thread_num() == omp_get_num_threads()-1) {
			std::clog << "\rScanlines remaining: " << (pixHeight - y) << ' ' << std::flush;
		}

	}

	auto renderTime = std::chrono::steady_clock::now() - startTime;

	std::cout << "Render duration " << std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f << " seconds." << std::endl;

	// *** Save the output image ***
	int errorCode;
	errorCode = lodepng::encode(config["outputFilename"], outImage, pixWidth, pixHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
