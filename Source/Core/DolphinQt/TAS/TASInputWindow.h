// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <optional>
#include <string_view>
#include <utility>

#include <QDialog>
#include <QLayout>
#include <QLayoutItem>

#include "Common/Assert.h"
#include "Common/CommonTypes.h"

#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerInterface/CoreDevice.h"

class QBoxLayout;
class QCheckBox;
class QDialog;
class QEvent;
class QGroupBox;
class QSpinBox;
class QString;
class TASCheckBox;
class TASSpinBox;

class InputOverrider final
{
public:
  using OverrideFunction = std::function<std::optional<ControlState>(ControlState)>;

  void AddFunction(std::string_view group_name, std::string_view control_name,
                   OverrideFunction function);

  ControllerEmu::InputOverrideFunction GetInputOverrideFunction() const;

private:
  std::map<std::pair<std::string_view, std::string_view>, OverrideFunction> m_functions;
};

class AspectRatioLayout : public QLayout
{
public:
  AspectRatioLayout(float aspect_ratio, QWidget* parent = nullptr)
      : QLayout(parent), m_aspect_ratio(aspect_ratio)
  {
    ASSERT(aspect_ratio != 0);
    // setSizeConstraint(QLayout::SetMinAndMaxSize);
    // setContentsMargins(QMargins{});
  }
  void addItem(QLayoutItem* item) override { m_items.emplace_back(item); }
  int count() const override { return static_cast<int>(m_items.size()); }
  QLayoutItem* itemAt(int index) const override
  {
    if (index < 0 || index >= count())
      return nullptr;
    return m_items[index].get();
  }
  QLayoutItem* takeAt(int index) override
  {
    if (index < 0 || index >= count())
      return nullptr;
    QLayoutItem* const item = m_items[index].release();
    m_items.erase(m_items.begin() + index);
    return item;
  }
  void setMinimumSize(QSize minimum_size)
  {
    m_minimum_size = minimum_size;

    // Make sure the minimum size has the correct aspect ratio
    int minimum_width = minimum_size.height() * m_aspect_ratio;
    int minimum_height = minimum_size.width() / m_aspect_ratio;
    m_minimum_size = m_minimum_size.expandedTo(QSize(minimum_width, minimum_height));
  }
  void setDefaultSize(QSize default_size)
  {
    m_default_size = default_size;

    // Make sure the default size has the correct aspect ratio
    int default_width = default_size.height() * m_aspect_ratio;
    int default_height = default_size.width() / m_aspect_ratio;
    m_default_size = m_default_size.expandedTo(QSize(default_width, default_height));
  }
  void setAspectRatio(float aspect_ratio)
  {
    m_aspect_ratio = aspect_ratio;

    // Update the existing minimum and default sizes to respect the new aspect ratio
    setMinimumSize(m_minimum_size);
    setDefaultSize(m_default_size);
  }
  Qt::Orientations expandingDirections() const override
  {
    return {/*Qt::Orientation::Horizontal/*, Qt::Orientation::Vertical*/};
  }
  // bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int width) const override { return width / m_aspect_ratio; }
  void setGeometry(const QRect& rect) override
  {
    QLayout::setGeometry(rect);
    if (count() == 0)
      return;

    int width = std::min(rect.width(), static_cast<int>(rect.height() * m_aspect_ratio));
    width = std::max(width, minimumSize().width());
    int height = std::min(rect.height(), static_cast<int>(rect.width() / m_aspect_ratio));
    height = std::max(height, minimumSize().height());
    int x_offset = (rect.width() - width) / 2;
    int y_offset = (rect.height() - height) / 2;

    QLayoutItem* const item = itemAt(0);
    item->setGeometry(rect.adjusted(x_offset, y_offset, -x_offset, -y_offset));

    for (int i = 1; i < count(); ++i)
      itemAt(i)->setGeometry(QRect{});
  }
  QSize sizeHint() const override { return m_default_size; }
  QSize minimumSize() const override { return count() == 0 ? QSize{} : itemAt(0)->minimumSize(); }
  // QSize maximumSize() const override { return m_default_size; }

private:
  std::vector<std::unique_ptr<QLayoutItem>> m_items;
  QSize m_minimum_size{};
  QSize m_default_size{};
  float m_aspect_ratio = 1.0;
};

class TASInputWindow : public QDialog
{
  Q_OBJECT
public:
  explicit TASInputWindow(QWidget* parent);

  int GetTurboPressFrames() const;
  int GetTurboReleaseFrames() const;

protected:
  TASCheckBox* CreateButton(const QString& text, std::string_view group_name,
                            std::string_view control_name, InputOverrider* overrider);
  QGroupBox* CreateStickInputs(const QString& text, std::string_view group_name,
                               InputOverrider* overrider, int min_x, int min_y, int max_x,
                               int max_y, Qt::Key x_shortcut_key, Qt::Key y_shortcut_key);
  QBoxLayout* CreateSliderValuePairLayout(const QString& text, std::string_view group_name,
                                          std::string_view control_name, InputOverrider* overrider,
                                          int zero, int default_, int min, int max,
                                          Qt::Key shortcut_key, QWidget* shortcut_widget,
                                          std::optional<ControlState> scale = {});
  TASSpinBox* CreateSliderValuePair(std::string_view group_name, std::string_view control_name,
                                    InputOverrider* overrider, QBoxLayout* layout, int zero,
                                    int default_, int min, int max,
                                    QKeySequence shortcut_key_sequence, Qt::Orientation orientation,
                                    QWidget* shortcut_widget,
                                    std::optional<ControlState> scale = {});
  TASSpinBox* CreateSliderValuePair(QBoxLayout* layout, int default_, int max,
                                    QKeySequence shortcut_key_sequence, Qt::Orientation orientation,
                                    QWidget* shortcut_widget);

  void changeEvent(QEvent* event) override;

  QGroupBox* m_settings_box;
  QCheckBox* m_use_controller;
  QSpinBox* m_turbo_press_frames;
  QSpinBox* m_turbo_release_frames;

private:
  std::optional<ControlState> GetButton(TASCheckBox* checkbox, ControlState controller_state);
  std::optional<ControlState> GetSpinBox(TASSpinBox* spin, int zero, int min, int max,
                                         ControlState controller_state);
  std::optional<ControlState> GetSpinBox(TASSpinBox* spin, int zero, ControlState controller_state,
                                         ControlState scale);
};
