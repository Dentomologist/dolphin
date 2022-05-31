// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include <QMenu>
#include <QObject>
#include <QString>

class QAction;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;

namespace Config
{
template <typename type>
class Info;
}

// Menu with non-exclusive selection that stays open after selecting or deselecting an item.
// Automatically adds select/deselect all actions to the top of the menu
class CheckableMenu final : public QMenu
{
  Q_OBJECT

public:
  explicit CheckableMenu(const QString& name, QMenu* parent = nullptr,
                         const QString& select_all_label = tr("Show All"),
                         const QString& deselect_all_label = tr("Hide All"));

  void AddConfigItem(const QString& label, const Config::Info<bool>* const config_item);
  void AddItem(const QString& label, const bool is_checked,
               std::function<void(bool)> toggle_function);

protected:
  virtual void keyPressEvent(QKeyEvent* event) override;
  virtual void leaveEvent(QEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

Q_SIGNALS:
  void ItemToggled(const QString& label, bool is_selected);

private:
  bool m_any_action_was_triggered;
  QAction* m_select_all_action;
  QAction* m_deselect_all_action;
};
