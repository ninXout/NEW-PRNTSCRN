#include "Screenshot.hpp"
#include <thread>

using namespace geode::prelude;

Screenshot::Screenshot(unsigned int width, unsigned int height, CCNode* node) : m_width(width), m_height(height), m_node(node), m_tex(RenderTexture(width, height)) {
    m_data = m_tex.capture(node);
}

CCTexture2D* Screenshot::intoTexture() {
    return m_tex.intoTexture();
}

#ifdef GEODE_IS_WINDOWS

void Screenshot::intoFile(const std::string& filename, bool jpeg) { // jpeg is unused because saveToFile handles automatically based on file extension
    std::thread([=, data = std::move(m_data)]() {
        GLubyte* newData = nullptr;
        newData = new GLubyte[m_width * m_width * 4];
        for (int i = 0; i < m_height; ++i){
            memcpy(&newData[i * m_width * 4], 
                    &data.get()[(m_height - i - 1) * m_width * 4], 
                    m_width * 4);
        }

        Loader::get()->queueInMainThread([=](){
            CCImage* image = new CCImage();
            if (image->initWithImageData(newData, m_width * m_height * 4, CCImage::EImageFormat::kFmtRawData, m_width, m_height, 8)) {
                image->autorelease();
                image->saveToFile(filename.c_str(), true);
            }
        });
    }).detach();
}

void Screenshot::intoClipboard() {
    std::thread([=, data = std::move(m_data)]() {
        auto bitmap = CreateBitmap(m_width, m_height, 1, 32, data.get());

        if (OpenClipboard(NULL)) {
            if (EmptyClipboard()) {
                SetClipboardData(CF_BITMAP, bitmap);
                CloseClipboard();
            }
        }
    }).detach();
}

#endif