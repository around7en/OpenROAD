#pragma once

#include <memory>

#include "gui/gui.h"

namespace replace {

class InitialPlace;
class NesterovBase;
class PlacerBase;
class GCell;

// This class draws debugging graphics on the layout
class Graphics : public gui::Renderer
{
 public:
  // Debug InitialPlace
  Graphics(std::shared_ptr<PlacerBase> pb,
           InitialPlace* ip);

  // Debug NesterovPlace
  Graphics(std::shared_ptr<PlacerBase> pb,
           std::shared_ptr<NesterovBase> nb,
           bool draw_bins);

  // Draw the graphics; optionally pausing afterwards
  void cellPlot(bool pause = false);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) override;
  virtual gui::Selected select(odb::dbTechLayer* layer,
                               const odb::Point& point) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  InitialPlace* ip_;
  GCell* selected_;
  bool draw_bins_;

  void drawNesterov(gui::Painter& painter);
  void drawInitial(gui::Painter& painter);
  void drawBounds(gui::Painter& painter);
};

}  // namespace replace
