#pragma once

#include "GameUi.h"
#include "activities/Activity.h"
#include "games/WordleGame.h"

// Wordle uses the system text keyboard to enter each guess. A tap on a tile of
// the row being typed moves the marker there. Give up, in the header, has to
// be held. The round is saved, so leaving the screen never loses it.
class WordleActivity final : public Activity {
 public:
  WordleActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

  static constexpr char kSavePath[] = "/.crosspoint/games/wordle.bin";

 private:
  enum class Notice : uint8_t { None, TooShort, NotAWord, InvalidCharacters, KeyboardUnavailable, NoDictionary };

  void layout();
  void startRound();
  void enterGuess();
  void applyKeyboardText(const char* text);
  void submitGuess();
  const char* noticeText() const;
  void save();
  void lookUpAnswer();
  // activeRow: a tile of the row being typed; marked: the tile the next letter goes into.
  void drawTile(const gameui::Box& box, char letter, wordle::Mark mark, bool activeRow, bool marked) const;
  void drawResult() const;

  wordle::Game game;
  Notice notice = Notice::None;
  bool dirty = false;  // round changed since the last save

  gameui::Box tiles[wordle::kMaxGuesses][wordle::kWordLength];
  gameui::Box noticeBox;
  gameui::HoldButton giveUpHold;
  gameui::Box guessBox;
  gameui::Box panelBox;  // result area that replaces the keyboard
  gameui::Box lookUpBox;
  gameui::Box newGameBox;
  gameui::ButtonPointer pointer;  // VARAGH: apps on button readers
};
