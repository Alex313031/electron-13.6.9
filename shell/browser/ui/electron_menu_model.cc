// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/electron_menu_model.h"

#include <utility>

#include "base/stl_util.h"

namespace electron {

#if defined(OS_MAC)
ElectronMenuModel::SharingItem::SharingItem() = default;
ElectronMenuModel::SharingItem::SharingItem(SharingItem&&) = default;
ElectronMenuModel::SharingItem::~SharingItem() = default;
#endif

bool ElectronMenuModel::Delegate::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  return GetAcceleratorForCommandIdWithParams(command_id, false, accelerator);
}

ElectronMenuModel::ElectronMenuModel(Delegate* delegate)
    : ui::SimpleMenuModel(delegate), delegate_(delegate) {}

ElectronMenuModel::~ElectronMenuModel() = default;

void ElectronMenuModel::SetToolTip(int index, const std::u16string& toolTip) {
  int command_id = GetCommandIdAt(index);
  toolTips_[command_id] = toolTip;
}

std::u16string ElectronMenuModel::GetToolTipAt(int index) {
  const int command_id = GetCommandIdAt(index);
  const auto iter = toolTips_.find(command_id);
  return iter == std::end(toolTips_) ? std::u16string() : iter->second;
}

void ElectronMenuModel::SetRole(int index, const std::u16string& role) {
  int command_id = GetCommandIdAt(index);
  roles_[command_id] = role;
}

std::u16string ElectronMenuModel::GetRoleAt(int index) {
  const int command_id = GetCommandIdAt(index);
  const auto iter = roles_.find(command_id);
  return iter == std::end(roles_) ? std::u16string() : iter->second;
}

void ElectronMenuModel::SetSecondaryLabel(int index,
                                          const std::u16string& sublabel) {
  int command_id = GetCommandIdAt(index);
  sublabels_[command_id] = sublabel;
}

std::u16string ElectronMenuModel::GetSecondaryLabelAt(int index) const {
  int command_id = GetCommandIdAt(index);
  const auto iter = sublabels_.find(command_id);
  return iter == std::end(sublabels_) ? std::u16string() : iter->second;
}

bool ElectronMenuModel::GetAcceleratorAtWithParams(
    int index,
    bool use_default_accelerator,
    ui::Accelerator* accelerator) const {
  if (delegate_) {
    return delegate_->GetAcceleratorForCommandIdWithParams(
        GetCommandIdAt(index), use_default_accelerator, accelerator);
  }
  return false;
}

bool ElectronMenuModel::ShouldRegisterAcceleratorAt(int index) const {
  if (delegate_) {
    return delegate_->ShouldRegisterAcceleratorForCommandId(
        GetCommandIdAt(index));
  }
  return true;
}

bool ElectronMenuModel::WorksWhenHiddenAt(int index) const {
  if (delegate_) {
    return delegate_->ShouldCommandIdWorkWhenHidden(GetCommandIdAt(index));
  }
  return true;
}

#if defined(OS_MAC)
bool ElectronMenuModel::GetSharingItemAt(int index, SharingItem* item) const {
  if (delegate_)
    return delegate_->GetSharingItemForCommandId(GetCommandIdAt(index), item);
  return false;
}

void ElectronMenuModel::SetSharingItem(SharingItem item) {
  sharing_item_.emplace(std::move(item));
}

const base::Optional<ElectronMenuModel::SharingItem>&
ElectronMenuModel::GetSharingItem() const {
  return sharing_item_;
}
#endif

void ElectronMenuModel::MenuWillClose() {
  ui::SimpleMenuModel::MenuWillClose();
  for (Observer& observer : observers_) {
    observer.OnMenuWillClose();
  }
}

void ElectronMenuModel::MenuWillShow() {
  ui::SimpleMenuModel::MenuWillShow();
  for (Observer& observer : observers_) {
    observer.OnMenuWillShow();
  }
}

ElectronMenuModel* ElectronMenuModel::GetSubmenuModelAt(int index) {
  return static_cast<ElectronMenuModel*>(
      ui::SimpleMenuModel::GetSubmenuModelAt(index));
}

}  // namespace electron
