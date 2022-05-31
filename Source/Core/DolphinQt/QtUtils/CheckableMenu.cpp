// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/QtUtils/CheckableMenu.h"

#include <QAction>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>

#include "Common/Config/Config.h"

CheckableMenu::CheckableMenu(const QString& name, QMenu* const parent,
                             const QString& select_all_label, const QString& deselect_all_label)
    : QMenu(name, parent), m_select_all_action(addAction(select_all_label)),
      m_deselect_all_action(addAction(deselect_all_label)), m_any_action_was_triggered(false)
{
  addSeparator();
}

void CheckableMenu::AddConfigItem(const QString& label, const Config::Info<bool>* const config_item)
{
  const bool current_value = Config::Get(*config_item);
  const auto update_function = [config_item](const bool is_checked) {
    Config::SetBase(*config_item, is_checked);
  };
  AddItem(label, current_value, update_function);
}

void CheckableMenu::AddItem(const QString& label, const bool is_checked,
                            std::function<void(bool)> toggle_function)
{
  auto* const menu_item = addAction(label);
  menu_item->setCheckable(true);
  menu_item->setChecked(is_checked);

  const auto set_selection_state = [this, menu_item, toggle_function, label](bool is_selected) {
    menu_item->setChecked(is_selected);
    toggle_function(is_selected);
    emit ItemToggled(label, is_selected);
  };

  const auto set_selected = std::bind(set_selection_state, true);
  const auto set_deselected = std::bind(set_selection_state, false);

  connect(menu_item, &QAction::triggered, set_selection_state);
  connect(m_select_all_action, &QAction::triggered, menu_item, set_selected);
  connect(m_deselect_all_action, &QAction::triggered, menu_item, set_deselected);
}

static void HideMenuTree(QMenu* top_menu)
{
  QObject* next_parent = top_menu->parent();
  while (next_parent != nullptr)
  {
    QMenu* next_parent_qmenu = qobject_cast<QMenu*>(next_parent);
    if (next_parent_qmenu != nullptr)
    {
      top_menu = next_parent_qmenu;
    }
    next_parent = next_parent->parent();
  }
  top_menu->hide();
}

// When triggering an action with Enter or Return QMenu::keyPressEvent closes the menu tree. This
// handler triggers the action without closing the menu, and passes all other keypresses to QMenu.
void CheckableMenu::keyPressEvent(QKeyEvent* const event)
{
  switch (event->key())
  {
  case Qt::Key_Enter:
  case Qt::Key_Return:
  {
    QAction* const action = activeAction();
    if (action != nullptr)
    {
      action->trigger();
      m_any_action_was_triggered = true;
      return;
    }
  }
    [[fallthrough]];
  default:
    QMenu::keyPressEvent(event);
  }
}

// QMenu generates a leave event either when the user moves the mouse outside the menu (which starts
// a timer that closes the current menu ~half a second later) or when the user clicks a menu action
// (which immediately closes the menu, making the leaveEvent redundant).
//
// CheckableMenu behaves the same as QMenu if the user hasn't selected any actions. If they have
// selected an action we can't duplicate QMenu's behavior since the point of CheckableMenu is to not
// immediately close the menu after an action is selected. However, closing the menu immediately
// after moving the mouse out of the window is closer to QMenu's behavior in that case than waiting
// for the timeout or making the user click outside the menu.
void CheckableMenu::leaveEvent(QEvent* const event)
{
  if (m_any_action_was_triggered)
  {
    HideMenuTree(this);
    m_any_action_was_triggered = false;
  }
  else
  {
    QMenu::leaveEvent(event);
  }
}

// When triggering an action by releasing the mouse QMenu::mouseReleaseEvent closes the menu tree.
// This handler triggers the action without closing the menu and passes all other keypresses to
// QMenu.
void CheckableMenu::mouseReleaseEvent(QMouseEvent* const event)
{
  QAction* const action = qobject_cast<QAction*>(actionAt(event->position().toPoint()));
  if (action)
  {
    action->trigger();
    m_any_action_was_triggered = true;
  }
  else
  {
    QMenu::mouseReleaseEvent(event);
  }
}
