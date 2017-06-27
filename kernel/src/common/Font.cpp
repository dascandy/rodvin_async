#include "Font.h"
#include "Xml.h"
#include "vfs.h"
#include "Image.h"
#include <path>

Font::glyph *readGlyph(XmlNode *node) {
  Font::glyph *g = new Font::glyph;
  g->x = strtol(node->attributes["x"].data(), NULL, 10);
  g->y = strtol(node->attributes["y"].data(), NULL, 10);
  g->w = strtol(node->attributes["width"].data(), NULL, 10);
  g->h = strtol(node->attributes["height"].data(), NULL, 10);
  g->xoff = strtol(node->attributes["xoffset"].data(), NULL, 10);
  g->yoff = strtol(node->attributes["yoffset"].data(), NULL, 10);
  g->xadv = strtol(node->attributes["xadvance"].data(), NULL, 10);
  return g;
}

future<Font*> Font::Create(const rodvin::path& filename) {
  return Vfs::lookup(filename).then([filename](future<DirEntry*> fde){
    DirEntry* de = fde.get();
    File* f = de->open();
    size_t length = f->filesize();
    uint8_t *data = new uint8_t[length];
    f->read(data, length);
    delete f;

    XmlNode *node = XmlRead((char *)data, length);
    XmlNode *chars = nullptr, *common = nullptr;
    rodvin::path imagePath;
    for (XmlNode *child : node->children) {
      if (child->type == "chars") 
        chars = child;
      else if (child->type == "pages") {
        XmlNode *first = child->children[0];
        rodvin::string textureName = first->attributes["file"];
        imagePath = filename.parent_path() / textureName;
      } else if (child->type == "common") {
        common = child;
      }
    }

    return Image::ConstructFromFile(imagePath).then([chars, common, node, data](future<Image*> fim) {
      Font *f = new Font(fim.get(), chars, common);
      delete node;
      delete [] data;
      return f;
    });
  });
}

Font::Font(Image* im, XmlNode* chars, XmlNode* common)
: image(im)
{
  height = strtol(common->attributes["lineHeight"].data(), NULL, 10);

  for (XmlNode *node : chars->children) {
    int id = strtol(node->attributes["id"].data(), NULL, 10);
    glyphs[id] = readGlyph(node);
  }
}

Font::~Font() {
  for (auto &p : glyphs) {
    delete p.second;
  }
}

size_t Font::getWidthFor(const rodvin::string& str) {
  // TODO: also use w/h
  // TODO: use some form of scaling
  size_t cx = 0;
  uint32_t lastChar = 0;
  for (uint32_t letter : str) {
    cx += getKerning(lastChar, letter);
    glyph *g = getGlyph(letter);
    lastChar = letter;
    if (!g) continue;
    cx += g->xadv;
  }
  return cx;
}


