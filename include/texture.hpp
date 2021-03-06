/** @file Texture.h
 * @brief Defines a method for loading textures.
 *
 * This file gives facilities for easily loading and binding textures.
 */

#ifndef TEXTURE_H
#define TEXTURE_H

#include <common.hpp>

/**
 * When defined, DEBUG allows extra messages to be printed to the terminal for
 * debugging purposes.
 */
#define DEBUG

/**
 * Texture functions are given a namespace for better organization.
 */
namespace Texture {

	/**
	 * Loads a texture from the given file name, returning the GLuint used for
	 * later referencing of the texture.
	 */

	GLuint loadTexture(std::string fileName);
    GLuint genColor(Color c);

	void freeTextures(void);

	void initColorIndex();
	vec2 getIndex(Color c);
	vec2 imageDim(std::string fileName);
}

class SpriteLoader {
private:
	std::unordered_map<uint64_t, GLuint> sprites;
	std::unordered_map<std::string, uint64_t> spritesLoc;

	uint64_t freeID = 0;
	uint64_t increaseID() {
		uint64_t id_t = 0;
		while (1) {
			try {
				sprites.at(id_t);
			} catch (const std::out_of_range& oor) {
				freeID = id_t;
				return freeID;
			}
			id_t++;
		}
	}
public:
	uint64_t loadSprite(std::string s)	{
		uint64_t tex_e = 0;
		try {
			tex_e = spritesLoc.at(s);
		} catch (const std::out_of_range& oor) {
			sprites.emplace(increaseID(), Texture::loadTexture (s));
			spritesLoc.emplace(s, freeID);
			return freeID;
		}
		return tex_e;
	}

	GLuint getSprite(uint64_t id) {
		return sprites.at(id);
	}
};

/**
 * DRAFT texture iterator?
 */
class TextureIterator {
private:
	std::vector<std::pair<GLuint, std::string>> textures;
	std::vector<std::pair<GLuint, std::string>>::iterator position;
public:
	TextureIterator(void) {
		position = std::begin(textures);
	}
	~TextureIterator(void) {
		textures.clear();
	}
	TextureIterator(const std::vector<std::string> &l) {
		for (const auto &s : l)
			textures.emplace_back(Texture::loadTexture(s), s);

		position = std::begin(textures);
	}
	void operator++(int) noexcept {
		if (++position < std::end(textures))
			glBindTexture(GL_TEXTURE_2D, (*position).first);
		else
			position = std::end(textures) - 1;
	}
	void operator--(int) noexcept {
		if (--position >= std::begin(textures))
			glBindTexture(GL_TEXTURE_2D, (*position).first);
		else
			position = std::begin(textures);
	}
	void operator()(const int &index) {
		if (index < 0 || index > static_cast<int>(textures.size()))
			throw std::invalid_argument("texture index out of range");

		position = std::begin(textures) + index;
		glBindTexture(GL_TEXTURE_2D, (*position).first);
	}
	const std::string& getTexturePath(const int &index) {
		if (index < 0 || index > static_cast<int>(textures.size()))
			throw std::invalid_argument("texture index out of range");

		return textures[index].second;
	}
	const vec2 getTextureDim(void) {
		return Texture::imageDim((*position).second);
	}
};

/**
 * The Texturec class.
 *
 * This class can handle an array of textures and allows easy binding of those
 * textures.
 */

class Texturec{
private:
	unsigned int texState;

public:

	std::vector<GLuint> image;
	std::vector<std::string> texLoc;

	Texturec(uint amt, ...);
	Texturec(uint amt,const char **paths);
	Texturec(std::vector<std::string>vec);
	Texturec(std::initializer_list<std::string> l);

	~Texturec();

	void bindNext();
	void bindPrev();
	void bind(unsigned int);
};

#endif //TEXTURE_H
