//
// Created by user on 7/8/22.
//

// see this: https://github.com/google/sanitizers/issues/1548
// add suppressions to avoid error detections
extern "C" const char *__lsan_default_suppressions() {
  return "leak:cache_t\n"
         "leak:realizeClassWithoutSwift\n"
         "leak:*CFTSDGetTable";
}