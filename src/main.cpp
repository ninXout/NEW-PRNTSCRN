#include "RenderTexture.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// #include <CCGL.h>
#include <thread>

#ifdef GEODE_IS_WINDOWS

using uint = unsigned int;

void screenshot(std::unique_ptr<uint8_t[]> data, const CCSize& size, bool copy, const std::string& filename, uint x, uint y, uint a, uint b) {
    const auto src_width = static_cast<uint>(size.width);
    const auto src_height = static_cast<uint>(size.height);
    a = a ? a : src_width;
    b = b ? b : src_height;
	std::thread([=, data = std::move(data)]() {
		if (copy) {
			auto bitmap = CreateBitmap((int)size.width, (int)size.height, 1, 32, data.get());

			if (OpenClipboard(NULL)) {
				if (EmptyClipboard()) {
					SetClipboardData(CF_BITMAP, bitmap);
					CloseClipboard();
				}
			}
		} else {
			GLubyte* newData = nullptr;
			newData = new GLubyte[(int)size.width * (int)size.width * 4];
			for (int i = 0; i < (int)size.height; ++i){
				memcpy(&newData[i * (int)size.width * 4], 
						&data.get()[((int)size.height - i - 1) * (int)size.width * 4], 
						(int)size.width * 4);
			}

			Loader::get()->queueInMainThread([=](){
				CCImage* image = new CCImage();
				std::string filepath = (geode::Mod::get()->getConfigDir() / filename).string();
				image->initWithImageData(newData, (int)size.width * (int)size.height * 4, CCImage::EImageFormat::kFmtRawData, (int)size.width, (int)size.height, 8);
				image->saveToFile(filepath.c_str(), true);
			});
		}
	}).detach();
}

#endif

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat) {
		if (down && key == enumKeyCodes::KEY_F2) {
			auto director = CCDirector::sharedDirector();
			auto winSize = director->getWinSize();
			auto scene = director->getRunningScene();

			auto captureSize = CCSize(Mod::get()->getSettingValue<int64_t>("resolution-width"), Mod::get()->getSettingValue<int64_t>("resolution-height"));
			RenderTexture texture(captureSize.width, captureSize.height);
			PlayLayer* pl = PlayLayer::get();
			if (pl && Mod::get()->getSettingValue<bool>("hide-ui")) {
				pl->getChildByID("UILayer")->setVisible(false);
				pl->getChildByID("percentage-label")->setVisible(false);
				pl->getChildByID("progress-bar")->setVisible(false);
			}
			auto data = texture.capture(scene);
			if (pl && Mod::get()->getSettingValue<bool>("hide-ui")) {
				pl->getChildByID("UILayer")->setVisible(true);
				pl->getChildByID("percentage-label")->setVisible(true);
				pl->getChildByID("progress-bar")->setVisible(true);
			}

			auto* ctexture = texture.intoTexture();

			std::string extension = Mod::get()->getSettingValue<bool>("jpeg-mafia") ? ".jpg" : ".png";
			std::string name = "menu";

			if (PlayLayer::get()) {
				std::filesystem::path folder = Mod::get()->getConfigDir() / (std::to_string(PlayLayer::get()->m_level->m_levelID));
				
				if (!std::filesystem::exists(folder)) std::filesystem::create_directory(folder);

				int i = 1;
				while (std::filesystem::exists(folder / (std::to_string(i) + extension))) {
					i++;
				}
				name = (folder / (std::to_string(i))).string();
			}
			name += extension;

			screenshot(std::move(data), captureSize, false, name);
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat);
	}
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(PlayLayer) {
	struct Fields {
		bool screenshotted = false;
	};

	void resetLevel() {
		PlayLayer::resetLevel();
		m_fields->screenshotted = false;
	}

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);

		if (Mod::get()->getSettingValue<bool>("auto-screenshot") && getCurrentPercentInt() >= Mod::get()->getSettingValue<int64_t>("auto-percent") && !m_fields->screenshotted) {
			auto director = CCDirector::sharedDirector();
			auto winSize = director->getWinSize();
			auto scene = director->getRunningScene();

			auto captureSize = CCSize(Mod::get()->getSettingValue<int64_t>("resolution-width"), Mod::get()->getSettingValue<int64_t>("resolution-height"));
			RenderTexture texture(captureSize.width, captureSize.height);
			auto data = texture.capture(scene);

			auto* ctexture = texture.intoTexture();

			std::string extension = Mod::get()->getSettingValue<bool>("jpeg-mafia") ? ".jpg" : ".png";
			std::string name = "menu";

			std::filesystem::path folder = Mod::get()->getConfigDir() / (std::to_string(m_level->m_levelID));
				
			if (!std::filesystem::exists(folder)) std::filesystem::create_directory(folder);

			int i = 0;
			while (std::filesystem::exists(folder / (std::string("auto_") + std::to_string(Mod::get()->getSettingValue<int64_t>("auto-percent")) + "-" + std::to_string(i) + extension))) {
				i++;
			}
			name = (folder / (std::string("auto_") + std::to_string(Mod::get()->getSettingValue<int64_t>("auto-percent")) + "-" + std::to_string(i))).string();
			name += extension;

			screenshot(std::move(data), captureSize, Mod::get()->getSettingValue<bool>("copy-clipboard"), name);
			m_fields->screenshotted = true;
		}
	}
};

#include <Geode/modify/PauseLayer.hpp>
#include "ScreenshotPopup.hpp"
class $modify(NewPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();

		auto btn = CCMenuItemSpriteExtra::create(
			CircleButtonSprite::createWithSprite("screenshot.png"_spr),
			this,
			menu_selector(NewPauseLayer::onScreenshotPopup)
		);
		static_cast<CCMenu*>(getChildByID("left-button-menu"))->addChild(btn);
		static_cast<CCMenu*>(getChildByID("left-button-menu"))->updateLayout();
	}

	void onScreenshotPopup(CCObject*) {
		ScreenshotPopup::create()->show();
	}
};