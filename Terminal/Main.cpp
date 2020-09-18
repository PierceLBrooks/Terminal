

#include <cstdlib>
#include <cstring>
#include <string>
#include <cmath>
#include <sstream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#ifdef SFML_SYSTEM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#include <signal.h>
#else
#include <pty.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif

#include "imgui/imconfig.h"
#include "imgui/imconfig-SFML.h"
#include "imgui/imgui.h"
#include "imgui/imgui-SFML.h"
#include "imgui/imgui_freetype.h"
#define imgui ImGui

using namespace std;
using namespace sf;

#include "vterm/vterm.h"
#include "vterm/vterm_keycodes.h"

#include "OptionFile.hpp"
#include "Terminal.hpp"
#include "SystemFrontend.hpp"
#include "WslFrontend.hpp"

OptionFile option;
Terminal* term;

RenderWindow* win;

void runImGui(Terminal* term, RenderWindow* win);

Sprite coverAutoscale(Texture& texture, Vector2f asize) {
	Sprite sp(texture);
	Vector2f tsize(texture.getSize());
	if (tsize.x / tsize.y > asize.x / asize.y) {
		float scale = asize.y / tsize.y;
		sp.setScale(scale, scale);
		sp.setPosition(-(tsize.x * scale - asize.x) / 2.0f, 0);
	} else {
		float scale = asize.x / tsize.x;
		sp.setScale(scale, scale);
		sp.setPosition(0, -(tsize.y * scale - asize.y) / 2.0f);
	}
	return sp;
}

const double menuCharFactor = 0.9;

int main(int argc, char* argv[]) {

	option.loadFromFile("Terminal.ini");
	int rows, cols;
	Vector2i cellSize = Vector2i(atoi(option.get("cell_width").c_str()), atoi(option.get("cell_height").c_str()));
	int charSize = atoi(option.get("fontsize").c_str()), menuOffset = (int)(charSize * menuCharFactor) + 2;
	bool useBold = option.get("use_bold") == "true";
	VideoMode mode;
	bool fullscreen = (argc > 1 && strcmp(argv[1], "--fullscreen") == 0);
	if (fullscreen) {
		mode = VideoMode::getDesktopMode();
		cols = mode.width / cellSize.x;
		rows = ((mode.height - menuOffset) / cellSize.y);
		const string& rc = option.get("run_on_startup");
		if (!rc.empty())
			system(rc.c_str());
	} else {
		rows = atoi(option.get("rows").c_str());
		cols = atoi(option.get("cols").c_str());
		mode = VideoMode(cols * cellSize.x, rows * cellSize.y + menuOffset);
	}
	string bgFilename = option.get("background_image");
	Uint8 bgDarkness = atoi(option.get("bg_darkness").c_str());
	Texture bgTexture;
	Sprite bgSprite;
	bool useVbo = VertexBuffer::isAvailable();
	int scrollMaxLines = atoi(option.get("scrollback_max_lines").c_str());
	bool useWslExe = option.get("use_wsl_exe") == "true";
	int updateRate = atoi(option.get("update_rate").c_str());

	if (!bgFilename.empty()) {
		if (!bgTexture.loadFromFile(bgFilename))
			bgFilename.clear();
		else {
			bgTexture.setSmooth(true);
		}
	}
	if (bgFilename.empty())
		bgDarkness = 255;
	else
		bgSprite = coverAutoscale(bgTexture, Vector2f(mode.width, mode.height));

	Font font;
#ifdef SFML_SYSTEM_WINDOWS
	//font.loadFromFile("C:\\Windows\\Fonts\\ConsolasDengXianSemiBold.ttf");
	string fontFile = "C:\\Windows\\Fonts\\" + option.get("font");
	font.loadFromFile(fontFile);
#else
	string fontFile = "/usr/share/fonts/" + option.get("font");
	//font.loadFromFile("/mnt/c/Windows/Fonts/ConsolasDengXianSemiBold.ttf");
	if (!font.loadFromFile(fontFile))
		if (!font.loadFromFile("/mnt/Windows/Windows/Fonts/ConsolasDengXianSemiBold.ttf"))
			if (!font.loadFromFile("/mnt/c/Windows/Fonts/ConsolasDengXianSemiBold.ttf"))
				font.loadFromFile("/usr/share/fonts/truetype/unifont/unifont.ttf");
#endif

	if (fullscreen)
		win = new RenderWindow(mode, "Terminal", Style::None);
	else
		win = new RenderWindow(mode, "Terminal", Style::Titlebar | Style::Close | Style::Resize);
	win->clear();
	win->display();

	if (fullscreen) {
		win->setPosition(Vector2i(0, 0));
		win->setMouseCursorVisible(false);
	}
	win->setKeyRepeatEnabled(true);

	if (!updateRate)
		win->setVerticalSyncEnabled(true);
	else
		win->setFramerateLimit(updateRate);

	VertexBuffer buf(PrimitiveType::Triangles, VertexBuffer::Dynamic);
	if (useVbo)
		buf.create((size_t)2 * rows * cols * 4);
	VertexArray arrtop;
	arrtop.setPrimitiveType(PrimitiveType::Triangles);
	vector<Vertex> arr;

#ifdef SFML_SYSTEM_WINDOWS
	bool useWsl = option.get("use_wsl_frontend") == "true";
	if (useWsl)
		term = new Terminal(new WslFrontend(option.get("wsl_backend_file"), option.get("shell"), option.get("wsl_working_dir"), rows, cols, useWslExe),
			rows, cols, cellSize, charSize, useBold, scrollMaxLines);
	else
		term = new Terminal(new SystemFrontend(option.get("shell"), rows, cols), rows, cols, cellSize, charSize, useBold, scrollMaxLines);
#else
	term = new Terminal(new SystemFrontend(option.get("shell"), rows, cols), rows, cols, cellSize, charSize, useBold, scrollMaxLines);
#endif
	term->cbSetWindowSize = [&](int width, int height) {
		win->setSize(Vector2u(width, height));
		win->setView(View(FloatRect(0, 0, width, height)));
	};
	term->cbSetWindowTitle = [&](const string& title) {
		win->setTitle(String::fromUtf8(title.begin(), title.end()));
	};

	term->offsetY = menuOffset;

	term->invalidate();

	imgui::SFML::Init(*win, false);
	imgui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
	imgui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	imgui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
	imgui::PushStyleColor(ImGuiCol_WindowBg, Color(0,0,0,160));
	imgui::GetIO().Fonts->AddFontFromFileTTF(fontFile.c_str(), (int)(charSize * menuCharFactor), 0, imgui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
	ImGuiFreeType::BuildFontAtlas(imgui::GetIO().Fonts, ImGuiFreeType::ForceAutoHint /*| ImGuiFreeType::MonoHinting*/);
	imgui::SFML::UpdateFontTexture();

	Clock cl, delta;
	while (win->isOpen()) {

		cl.restart();

		imgui::SFML::Update(*win, delta.restart());
		term->update();


		bool redrawn = term->redrawIfRequired(font, arr, bgDarkness == 255 ? Color::Black : Color(0, 0, 0, bgDarkness));
		win->clear();

		if (bgDarkness != 255)
			win->draw(bgSprite);

		if (useVbo) {
			if (redrawn)
				buf.update(arr.data(), arr.size(), 0);
			win->draw(buf, 0, arr.size(), &font.getTexture(charSize));
		} else
			win->draw(arr.data(), arr.size(), PrimitiveType::Triangles, &font.getTexture(charSize));

		if (fullscreen) {
			arrtop.clear();
			const Vector2f pos(Mouse::getPosition(*win));
			const float sqrt2 = 1.414213562f;
			const Vector2f off1(0, 16), off2(8 * sqrt2, 8 * sqrt2);
			arrtop.append(Vertex(pos, Color::White));
			arrtop.append(Vertex(pos + off1, Color::White));
			arrtop.append(Vertex(pos + Vector2f(8, 8), Color::White));
			arrtop.append(Vertex(pos, Color::White));
			arrtop.append(Vertex(pos + off2, Color::White));
			arrtop.append(Vertex(pos + Vector2f(0, 8 * sqrt2), Color::White));
			win->draw(arrtop);
		}

		runImGui(term, win);
		imgui::EndFrame();
		imgui::SFML::Render(*win);

		Event e;
		e.type = Event::Count;
		while (win->pollEvent(e)) {
			imgui::SFML::ProcessEvent(e);
			if (e.type == Event::Closed)
				win->close();
			else if (e.type == Event::Resized) {
				win->setView(View(FloatRect(0, 0, e.size.width, e.size.height)));

				if (e.size.width / cellSize.x != cols || e.size.height / cellSize.y != rows) {
					cols = e.size.width / cellSize.x;
					rows = e.size.height / cellSize.y;
					buf.create((size_t)2 * rows * cols * 4);
				}

				bgSprite = coverAutoscale(bgTexture, Vector2f(e.size.width, e.size.height));
			}

			if (!(imgui::GetIO().WantCaptureKeyboard && (e.type == Event::TextEntered)) && !(imgui::GetIO().WantCaptureMouse && (e.type == Event::MouseButtonPressed || e.type == Event::MouseButtonReleased || e.type == Event::MouseMoved)))
				term->processEvent(*win, e);
		}


		if (!term->isRunning())
			win->close();


		win->display();
		if (updateRate)
			sleep(max(microseconds(0), seconds(1.0f / updateRate) - cl.getElapsedTime()));
	}

	//delete win;
	//delete term;

	return 0;
}

