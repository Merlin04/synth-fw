#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>
#include "imduino.h"

texture_alpha8_t fontAtlas;

#define SCREEN_X 240
#define SCREEN_Y 320

#define TFT_CS 10
#define TFT_RST -1
#define TFT_DC 9

#define RGBA(r, g, b, a) ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f)

#define UI_BG RGBA(102, 102, 102, 1)
#define UI_PRIMARY RGBA(255, 91, 15, 1)
#define UI_DARK_PRIMARY RGBA(209, 73, 0, 1) // use for stuff like hover/selections
#define UI_ACCENT RGBA(68, 255, 210) // only use in rare circumstances like selected text
#define UI_LIGHTBG RGBA(151, 151, 155, 1) // use for things like backgrounds of text input
#define UI_TEXT RGBA(255, 255, 255, 1)
#define UI_TEXT_DISABLED RGBA(255, 255, 255, 0.6)
#define UI_BLACK RGBA(23, 3, 18, 1) // use this rarely

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
texture_color16_t screen; // it's going to be 153600 bytes, so best keep it in external ram which we have plenty of

void screen_init() {
  tft.init(SCREEN_X, SCREEN_Y);
  tft.setFont(&FreeSans9pt7b);
  tft.fillScreen(ST77XX_BLACK);
}

void screen_draw() {
  tft.dmaWait(); // wait for previous operations to complete
  tft.startWrite();
  tft.setAddrWindow(0, 0, SCREEN_X, SCREEN_Y);
  tft.writePixels((uint16_t*) screen.pixels, SCREEN_X * SCREEN_Y, false);
  tft.endWrite();
}

unsigned long drawTime;
unsigned long renderTime;
unsigned long rasterTime;

ImGuiContext* context;

void setup() {
  // delay(5000);// time to open serial monitor
  Serial.begin(115200);
  Serial.println("Hello world!");
  context = ImGui::CreateContext();
  ImGui_ImplSoftraster_Init(&screen);

  ImGuiStyle& style = ImGui::GetStyle();
  style.AntiAliasedLines = false;
  style.AntiAliasedFill = false;
  // style.WindowRounding = 0.0f;
  style.Alpha = 1.0f;
  style.FrameRounding = 4.0f; // todo tweak

  auto& colors = style.Colors;
  colors[ImGuiCol_WindowBg] = UI_BG;
  colors[ImGuiCol_MenuBarBg] = UI_BG;

  // border
  colors[ImGuiCol_Border] = UI_PRIMARY;

  // text
  colors[ImGuiCol_Text] = UI_TEXT;
  colors[ImGuiCol_TextDisabled] = UI_TEXT_DISABLED;

  // headers
  colors[ImGuiCol_Header] = UI_PRIMARY;
  colors[ImGuiCol_HeaderHovered] = UI_DARK_PRIMARY;

  // buttons
  colors[ImGuiCol_Button] = UI_PRIMARY;
  colors[ImGuiCol_ButtonHovered] = UI_DARK_PRIMARY;
  colors[ImGuiCol_ButtonActive] = UI_DARK_PRIMARY;
  colors[ImGuiCol_CheckMark] = UI_TEXT;

  // popps
  colors[ImGuiCol_PopupBg] = UI_BG;

  // slider
  colors[ImGuiCol_SliderGrab] = UI_PRIMARY;
  colors[ImGuiCol_SliderGrabActive] = UI_DARK_PRIMARY;

  // frame bg
  colors[ImGuiCol_FrameBg] = UI_LIGHTBG;
  colors[ImGuiCol_FrameBgHovered] = UI_PRIMARY;
  colors[ImGuiCol_FrameBgActive] = UI_PRIMARY;

  // tabs - todo

  // title
  colors[ImGuiCol_TitleBg] = UI_PRIMARY;
  colors[ImGuiCol_TitleBgActive] = UI_PRIMARY;
  colors[ImGuiCol_TitleBgCollapsed] = UI_PRIMARY;

  // scrollbar
  colors[ImGuiCol_ScrollbarBg] = UI_BG;
  colors[ImGuiCol_ScrollbarGrab] = UI_PRIMARY;
  colors[ImGuiCol_ScrollbarGrabHovered] = UI_DARK_PRIMARY;
  colors[ImGuiCol_ScrollbarGrabActive] = UI_DARK_PRIMARY;

  // separator
  colors[ImGuiCol_Separator] = UI_PRIMARY;
  colors[ImGuiCol_SeparatorHovered] = UI_DARK_PRIMARY;
  colors[ImGuiCol_SeparatorActive] = UI_DARK_PRIMARY;

  // resize grip
  colors[ImGuiCol_ResizeGrip] = UI_PRIMARY;
  colors[ImGuiCol_ResizeGripHovered] = UI_DARK_PRIMARY;
  colors[ImGuiCol_ResizeGripActive] = UI_DARK_PRIMARY;


  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight | ImFontAtlasFlags_NoMouseCursors;

  uint8_t* pixels;
  int width, height;
  io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
  fontAtlas.init(width, height, (alpha8_t*) pixels);
  io.Fonts->TexID = &fontAtlas;

  screen_init();
  screen.pixels = nullptr;
  screen.needFree = false;
  screen.init(SCREEN_X, SCREEN_Y);
}

float f = 0.0f;
unsigned long t = 0;

void loop() {
  ImGuiIO& io = ImGui::GetIO();
  io.DeltaTime = 1.0f / 60.0f;

  ImGui_ImplSoftraster_NewFrame();
  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
  ImGui::SetNextWindowSize(ImVec2(SCREEN_X, SCREEN_Y));
  ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

  f += 0.05;
  if(f > 1.0f) f = 0.0f;

  unsigned int deltaTime = millis() - t;
  t += deltaTime;

  deltaTime -= (drawTime + renderTime + rasterTime);

  ImGui::Text("Hardware write time %d ms", drawTime);
  ImGui::Text("Render time %d ms", renderTime);
  ImGui::Text("Raster time %d ms", rasterTime);
  ImGui::Text("Remaining time %d ms", deltaTime);
  // long text demo
  ImGui::Text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec a diam lectus. Sed sit amet ipsum mauris. Maecenas congue ligula ac quam viverra nec consectetur ante hendrerit. Donec et mollis dolor.");
  ImGui::SliderFloat("SliderFloat", &f, 0.0f, 1.0f);
  float samples[100];
for (int n = 0; n < 100; n++)
    samples[n] = sinf(n * 0.2f + ImGui::GetTime() * 1.5f);
ImGui::PlotLines("Samples", samples, 100);

  ImGui::End();

  renderTime = millis();
  ImGui::Render();
  renderTime = millis() - renderTime;

  rasterTime = millis();
  auto d = ImGui::GetDrawData();
  ImGui_ImplSoftraster_RenderDrawData(d);
  rasterTime = millis() - rasterTime;

  drawTime = millis();
  screen_draw();
  drawTime = millis() - drawTime;
}