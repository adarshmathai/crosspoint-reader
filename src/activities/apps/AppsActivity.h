#pragma once

#include "activities/UiListActivity.h"

class AppsActivity final : public UiListActivity {
 public:
  AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void render(RenderLock&&) override;

 protected:
  int listCount() const override { return 7; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override;

 private:
  freeink::ui::TileGridItem tiles[7]{};
  freeink::ui::Rect gridRect{};
  int columns = 2;
  int gap = 12;

  void drawTiles() const;
};
