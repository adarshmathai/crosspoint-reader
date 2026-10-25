#include "AppsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <utility>

#include "MappedInputManager.h"
#include "activities/apps/AppArt.h"
#include "activities/apps/CheckersActivity.h"
#include "activities/apps/ChessActivity.h"
#include "activities/apps/ClockActivity.h"
#include "activities/apps/GameUi.h"
#include "activities/apps/GoActivity.h"
#include "activities/apps/SudokuActivity.h"
#include "activities/apps/TicTacToeActivity.h"
#include "activities/apps/WordleActivity.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "fontIds.h"

namespace fui = freeink::ui;

namespace {

constexpr int kIconSize = 100;
constexpr uint8_t kCardRadius = 12;
constexpr int kLargeTileHeight = 219;

enum class AppId : uint8_t { Wordle, Sudoku, Chess, Checkers, Go, TicTacToe, Clock };
constexpr AppId kApps[] = {AppId::Wordle, AppId::Sudoku,    AppId::Chess, AppId::Checkers,
                           AppId::Go,     AppId::TicTacToe, AppId::Clock};

void drawWordleIcon(const GfxRenderer& renderer, const int x, const int y, const int size) {
  const int gap = std::max(2, size / 24);
  const int block = (size - 4 * gap) / 5;
  const int top = y + (size - 2 * block - gap) / 2;
  for (int column = 0; column < 5; ++column) {
    const int bx = x + column * (block + gap);
    const int tone = (column * 7 + 1) % 5;
    for (int row = 0; row < 2; ++row) {
      const int by = top + row * (block + gap);
      if ((column + row) % 4 == 0) {
        renderer.fillRoundedRect(bx, by, block, block, 5, Color::Black);
      } else if (tone == row) {
        renderer.fillRoundedRect(bx, by, block, block, 5, Color::LightGray);
        renderer.drawRoundedRect(bx, by, block, block, 2, 5, true);
      } else {
        renderer.drawRoundedRect(bx, by, block, block, 2, 5, true);
      }
    }
  }
}

void drawSudokuIcon(const GfxRenderer& renderer, const int x, const int y, const int size) {
  const int cell = size / 3;
  const int grid = cell * 3;
  renderer.drawRect(x, y, grid, grid, 3, true);
  for (int line = 1; line < 3; ++line) {
    const int weight = line == 1 ? 1 : 3;
    renderer.fillRect(x + line * cell - weight / 2, y, weight, grid, true);
    renderer.fillRect(x, y + line * cell - weight / 2, grid, weight, true);
  }
  gameui::bigDigit(renderer, 4, true, {x, y, cell, cell}, gameui::Ink::Black);
  gameui::bigDigit(renderer, 8, true, {x + cell, y + cell, cell, cell}, gameui::Ink::Black);
  gameui::bigDigit(renderer, 2, true, {x + 2 * cell, y + 2 * cell, cell, cell}, gameui::Ink::Black);
}

void drawChessIcon(const GfxRenderer& renderer, const int x, const int y, const int size) {
  const int square = size / 4;
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      if ((row + column) % 2 == 0) gameui::hatch(renderer, {x + column * square, y + row * square, square, square});
    }
  }
  renderer.drawRect(x - 2, y - 2, size + 4, size + 4, 2, true);
  const auto& knight = appart::kMenuKnight;
  gameui::stampOver(renderer, knight, appart::kMenuKnightUnder, x + (size - knight.width) / 2,
                    y + (size - knight.height) / 2);
}

void drawCheckersIcon(const GfxRenderer& renderer, const int x, const int y, const int size) {
  const int square = size / 4;
  const int radius = std::max(4, square / 3);
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      if ((row + column) % 2 == 0) renderer.fillRect(x + column * square, y + row * square, square, square, true);
    }
  }
  renderer.drawRect(x, y, square * 4, square * 4, 2, true);
  gameui::disc(renderer, x + square * 1 + square / 2, y + square * 2 + square / 2, radius, false);
  gameui::ring(renderer, x + square * 1 + square / 2, y + square * 2 + square / 2, radius, 2, true);
  gameui::disc(renderer, x + square * 2 + square / 2, y + square + square / 2, radius, true);
  gameui::ring(renderer, x + square * 2 + square / 2, y + square + square / 2, radius - 2, 1, false);
}

void drawTicTacToeIcon(const GfxRenderer& renderer, const int x, const int y, const int size) {
  const int cell = size / 3;
  for (int i = 1; i < 3; ++i) {
    renderer.fillRect(x + i * cell - 1, y + 2, 3, size - 4, true);
    renderer.fillRect(x + 2, y + i * cell - 1, size - 4, 3, true);
  }
  for (const int at : {0, 8}) {
    const int inset = std::max(4, cell / 5);
    const int left = x + (at % 3) * cell + inset;
    const int top = y + (at / 3) * cell + inset;
    renderer.drawLine(left, top, left + cell - 2 * inset, top + cell - 2 * inset, 4, true);
    renderer.drawLine(left, top + cell - 2 * inset, left + cell - 2 * inset, top, 4, true);
  }
  for (const int at : {2, 4}) {
    gameui::ring(renderer, x + (at % 3) * cell + cell / 2, y + (at / 3) * cell + cell / 2, std::max(6, cell / 3), 3,
                 true);
  }
}

void drawGoIcon(const GfxRenderer& renderer, const int x, const int y, const int size) {
  constexpr int lines = 5;
  const int step = size / lines;
  const int first = step / 2;
  for (int i = 0; i < lines; ++i) {
    renderer.fillRect(x + first, y + first + i * step, step * (lines - 1) + 1, i == 0 ? 3 : 2, true);
    renderer.fillRect(x + first + i * step, y + first, i == 0 ? 3 : 2, step * (lines - 1) + 1, true);
  }
  const auto stone = [&](const int column, const int row, const bool black) {
    const int cx = x + first + column * step;
    const int cy = y + first + row * step;
    const int radius = std::max(4, step / 2 - 1);
    gameui::disc(renderer, cx, cy, radius, black);
    gameui::ring(renderer, cx, cy, radius, black ? 1 : 2, true);
  };
  stone(1, 1, true);
  stone(2, 1, false);
  stone(2, 2, true);
  stone(3, 2, false);
  stone(1, 3, false);
}

void drawCalendarIcon(const GfxRenderer& renderer, const int x, const int y, const int size) {
  const int headerHeight = std::max(18, size / 3);
  renderer.drawRoundedRect(x, y, size, size, 3, 10, true);
  renderer.fillRoundedRect(x, y, size, headerHeight, 10, true, true, false, false, Color::Black);
  const int columnStep = size / 4;
  const int rowStep = (size - headerHeight - 8) / 3;
  for (int row = 0; row < 3; ++row) {
    for (int column = 0; column < 3; ++column) {
      const int cx = x + (column + 1) * columnStep;
      const int cy = y + headerHeight + row * rowStep + rowStep / 2;
      if (row == 1 && column == 1) {
        renderer.fillRect(cx - 3, cy - 3, 7, 7, true);
      } else {
        renderer.drawRect(cx - 3, cy - 3, 7, 7, 1, true);
      }
    }
  }
}

void drawAppIcon(const GfxRenderer& renderer, const AppId app, const int x, const int y, const int size) {
  switch (app) {
    case AppId::Wordle:
      drawWordleIcon(renderer, x, y, size);
      break;
    case AppId::Sudoku:
      drawSudokuIcon(renderer, x, y, size);
      break;
    case AppId::Chess:
      drawChessIcon(renderer, x, y, size);
      break;
    case AppId::Checkers:
      drawCheckersIcon(renderer, x, y, size);
      break;
    case AppId::Go:
      drawGoIcon(renderer, x, y, size);
      break;
    case AppId::TicTacToe:
      drawTicTacToeIcon(renderer, x, y, size);
      break;
    case AppId::Clock:
      drawCalendarIcon(renderer, x, y, size);
      break;
  }
}

}  // namespace

AppsActivity::AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("Apps", renderer, mappedInput) {}

const char* AppsActivity::headerTitle() const { return tr(STR_APPS); }

void AppsActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMarginFromScreen(
      fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), metrics.contentSidePadding,
                  static_cast<int16_t>(metrics.buttonHintsHeight), metrics.contentSidePadding});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  const int selected = nav.selected.load();
  for (int i = 0; i < listCount(); ++i) {
    tiles[i].label = nullptr;
    tiles[i].value = static_cast<int16_t>(i);
    tiles[i].state = i == selected ? fui::StateSelected : fui::StateNormal;
  }

  gridRect = screen.body();
  columns = renderer.getScreenWidth() > renderer.getScreenHeight() ? 3 : 2;
  const int rows = (listCount() + columns - 1) / columns;
  gap = gridRect.height >= rows * kLargeTileHeight + (rows - 1) * 16 ? 16 : 12;
  const int tileHeight = (gridRect.height - (rows - 1) * gap) / rows;

  fui::TileGridProps props;
  props.items = tiles;
  props.count = static_cast<uint16_t>(listCount());
  props.action = ACTION_ROW;
  props.columns = static_cast<uint8_t>(columns);
  props.gap = static_cast<int16_t>(gap);
  props.tileHeight = static_cast<int16_t>(tileHeight);
  props.inputMask = fui::InputTouch;
  props.radius = kCardRadius;
  props.styles.explicitlySet = true;
  props.styles.normal.background = fui::Paint::solid(fui::Color::White);
  props.styles.normal.foreground = fui::Paint::solid(fui::Color::Black);
  props.styles.normal.border = fui::Paint::solid(fui::Color::Black);
  props.styles.normal.borderWidth = 2;
  props.styles.normal.radius = kCardRadius;
  props.styles.selected = props.styles.normal;
  props.styles.selected.background = fui::Paint::solid(fui::Color::LightGray);
  props.styles.focused = props.styles.selected;
  props.styles.active = props.styles.selected;
  screen.tileGrid(props);
}

void AppsActivity::drawTiles() const {
  const char* const labels[] = {tr(STR_GAME_WORDLE), tr(STR_GAME_SUDOKU),    tr(STR_GAME_CHESS), tr(STR_GAME_CHECKERS),
                                tr(STR_GAME_GO),     tr(STR_GAME_TICTACTOE), tr(STR_APP_CLOCK)};
  const int rows = (listCount() + columns - 1) / columns;
  const int tileWidth = (gridRect.width - (columns - 1) * gap) / columns;
  const int tileHeight = (gridRect.height - (rows - 1) * gap) / rows;
  for (int i = 0; i < listCount(); ++i) {
    const int row = i / columns;
    const int column = i % columns;
    const fui::Rect tile{static_cast<int16_t>(gridRect.x + column * (tileWidth + gap)),
                         static_cast<int16_t>(gridRect.y + row * (tileHeight + gap)), static_cast<int16_t>(tileWidth),
                         static_cast<int16_t>(tileHeight)};
    const int iconSize = std::min(kIconSize, tileHeight - 38);
    if (iconSize <= 0) continue;
    const int iconY = tile.y + (tile.height - iconSize - 34) / 2;
    drawAppIcon(renderer, kApps[i], tile.x + (tile.width - iconSize) / 2, iconY, iconSize);
    gameui::fittedText(renderer, {tile.x + 3, iconY + iconSize + 4, tile.width - 6, 30}, labels[i]);
  }
}

void AppsActivity::render(RenderLock&&) {
  renderer.clearScreen();
  drawChrome();
  renderUi();
  drawTiles();
  drawFooter();
  renderer.displayBuffer();
}

void AppsActivity::activateIndex(const int index) {
  if (index < 0 || index >= listCount()) return;
  std::unique_ptr<Activity> activity;
  switch (kApps[index]) {
    case AppId::Wordle:
      activity = makeUniqueNoThrow<WordleActivity>(renderer, mappedInput);
      break;
    case AppId::Sudoku:
      activity = makeUniqueNoThrow<SudokuActivity>(renderer, mappedInput);
      break;
    case AppId::Chess:
      activity = makeUniqueNoThrow<ChessActivity>(renderer, mappedInput);
      break;
    case AppId::Checkers:
      activity = makeUniqueNoThrow<CheckersActivity>(renderer, mappedInput);
      break;
    case AppId::Go:
      activity = makeUniqueNoThrow<GoActivity>(renderer, mappedInput);
      break;
    case AppId::TicTacToe:
      activity = makeUniqueNoThrow<TicTacToeActivity>(renderer, mappedInput);
      break;
    case AppId::Clock:
      activity = makeUniqueNoThrow<ClockActivity>(renderer, mappedInput);
      break;
  }
  if (!activity) {
    LOG_ERR("APPS", "Could not allocate app activity");
    return;
  }
  app.clearTapFlash();
  startActivityForResult(std::move(activity), [this](const ActivityResult&) { requestUpdate(); });
}
