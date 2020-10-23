#pragma once

#include <Qt>

enum class eSceneTreeRoles
{
    InternalObjectRole = Qt::UserRole,
    RightAlignedDecorationRole,
    ForegroundAlphaRole,
    FilterDataRole
};

constexpr int ToItemRoleCast(const eSceneTreeRoles& role)
{
    return static_cast<int>(role);
}
