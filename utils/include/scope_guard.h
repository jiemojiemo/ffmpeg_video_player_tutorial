//
// Created by user on 1/19/23.
//

#ifndef FFMPEG_VIDEO_PLAYER_SCOPE_GUARD_H
#define FFMPEG_VIDEO_PLAYER_SCOPE_GUARD_H

#pragma once
#include <functional>
class ScopeGuard {
public:
  explicit ScopeGuard(std::function<void()> onExitScope)
      : onExitScope_(onExitScope) {}

  ~ScopeGuard() { onExitScope_(); }

private:
  std::function<void()> onExitScope_;

private: // noncopyable
  ScopeGuard(ScopeGuard const &) = delete;
  ScopeGuard &operator=(ScopeGuard const &) = delete;
};

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)
#define ON_SCOPE_EXIT(callback)                                                \
  ScopeGuard SCOPEGUARD_LINENAME(EXIT, __LINE__)(callback)

#endif // FFMPEG_VIDEO_PLAYER_SCOPE_GUARD_H
